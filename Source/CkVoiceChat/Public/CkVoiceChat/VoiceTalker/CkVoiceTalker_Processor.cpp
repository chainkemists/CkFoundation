#include "CkVoiceTalker_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkVoiceChat/Capture/CkVoiceChat_CaptureSource.h"
#include "CkVoiceChat/CkVoiceChat_Log.h"
#include "CkVoiceChat/CkVoiceChat_Stats.h"
#include "CkVoiceChat/Net/CkVoiceChatRelay_Actor.h"
#include "CkVoiceChat/Net/CkVoiceChatRelay_Subsystem.h"
#include "CkVoiceChat/Settings/CkVoiceChat_Settings.h"
#include "CkVoiceChat/VoiceChannel/CkVoiceChannel_Utils.h"
#include "CkVoiceChat/VoiceListener/CkVoiceListener_Fragment.h"

#include "Interfaces/VoiceCodec.h"
#include "VoiceModule.h"

#include <GameFramework/PlayerController.h>
#include <GameFramework/PlayerState.h>
#include <Sound/SoundAttenuation.h>
#include <Sound/SoundEffectSource.h>

DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Frames Captured"),  STAT_CkVoiceChat_FramesCaptured,  STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Frames Encoded"),   STAT_CkVoiceChat_FramesEncoded,   STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Frames Concealed"), STAT_CkVoiceChat_FramesConcealed, STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Frames Decoded"),   STAT_CkVoiceChat_FramesDecoded,   STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Frames Sent"),      STAT_CkVoiceChat_FramesSent,      STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Frames Dropped (Stale)"), STAT_CkVoiceChat_FramesDroppedStale, STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Bundles Dropped (Inbox Full)"), STAT_CkVoiceChat_BundlesDroppedInboxFull_Talker, STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Receive Dropped (Malformed)"), STAT_CkVoiceChat_ReceiveDroppedMalformed, STATGROUP_CkVoiceChat);

CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceTalker_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceTalker_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceTalker_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceTalker_Capture);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceTalker_ReceivePlayback);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceTalker_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voice_talker_processor
{
    // FVoiceDecoderOpus refuses to decode unless the output buffer holds MAX_OPUS_FRAMES (6)
    // whole frames - VoiceCodecOpus.cpp:20 + the UncompressedBufferAvail gate in Decode.
    constexpr auto EngineDecodeCapacityFrames = 6;

    // Test-readable loopback PCM is bounded so long-running talkers don't grow it forever.
    constexpr auto MaxLoopbackDecodedBytes = 48000 * 2 * 2;   // 2 s at 48 kHz mono 16-bit

    // Send-side freshness bound: a frame older than this has already
    // blown the mouth-to-ear budget - dropping it beats delivering it late, and it keeps the
    // outbound queue bounded when no channel/relay is reachable yet.
    const auto MaxOutboundFrameAge = FCk_Time{0.15};

    // Headroom under the 256 B server->client unreliable split threshold, leaving
    // room for the RPC envelope (talker handle + array overhead) around the packed bundle.
    constexpr auto MaxPackedBundleBytes = 240;

    auto
    Get_FrameBytes() -> int32
    {
        const auto SampleRate = UCk_Utils_VoiceChat_Settings_UE::Get_SampleRate();
        const auto FrameSizeMs = UCk_Utils_VoiceChat_Settings_UE::Get_FrameSizeMs();
        return (SampleRate * FrameSizeMs / 1000) * 2;
    }

    auto
    Get_FrameDurationSeconds() -> double
    {
        return static_cast<double>(UCk_Utils_VoiceChat_Settings_UE::Get_FrameSizeMs()) / 1000.0;
    }

    auto
    Apply_Gain(
        TArrayView<int16> InOutSamples,
        float InGain) -> void
    {
        if (FMath::IsNearlyEqual(InGain, 1.0f))
        { return; }

        for (auto& Sample : InOutSamples)
        {
            Sample = static_cast<int16>(FMath::Clamp(
                FMath::RoundToInt32(static_cast<float>(Sample) * InGain), -32768, 32767));
        }
    }

    auto
    Get_LocalPlayerState(
        UWorld* InWorld) -> APlayerState*
    {
        if (ck::Is_NOT_Valid(InWorld, ck::IsValid_Policy_NullptrOnly{}))
        { return nullptr; }

        const auto* LocalController = InWorld->GetFirstPlayerController();

        if (ck::Is_NOT_Valid(LocalController, ck::IsValid_Policy_NullptrOnly{}))
        { return nullptr; }

        return LocalController->PlayerState;
    }

    auto
    ResolveRelayChannel_ForPlayer(
        UWorld* InWorld,
        APlayerState* InPlayerState) -> ACk_VoiceChatRelay_UE*
    {
        if (ck::Is_NOT_Valid(InWorld, ck::IsValid_Policy_NullptrOnly{}) || ck::Is_NOT_Valid(InPlayerState))
        { return nullptr; }

        auto* Subsystem = InWorld->GetSubsystem<UCk_VoiceChatRelay_Subsystem_UE>();

        if (ck::Is_NOT_Valid(Subsystem, ck::IsValid_Policy_NullptrOnly{}))
        { return nullptr; }

        auto Pending = Subsystem->Request_AcquireChannel_ForPlayer(InPlayerState);
        const auto Result = Subsystem->Try_ResolvePending(Pending);

        return ::Cast<ACk_VoiceChatRelay_UE>(Result.Get_ChannelActor().Get());
    }

    // Drains the outbound queue: staleness first, then oldest-first under the byte budget, then
    // bundles of <=FramesPerRpc frames (bounded under the unreliable split threshold) packed once
    // per eligible channel. Unsendable frames (no relay yet, no channel yet) stay queued - the
    // staleness bound keeps that wait finite.
    auto
    Send_OutboundFrames(
        FCk_Handle_VoiceTalker InTalker,
        uint8 InAmplitudeQ8,
        FCk_Time InCaptureClock,
        TArray<ck::FCk_VoiceChat_OutboundFrame>& InOutFrames) -> void
    {
        if (InOutFrames.IsEmpty())
        { return; }

        auto Entries = TArray<FCk_VoiceChat_PacingEntry>{};
        Entries.Reserve(InOutFrames.Num());
        for (const auto& Frame : InOutFrames)
        {
            Entries.Emplace(FCk_VoiceChat_PacingEntry{
                static_cast<int32>(Frame.Get_Seq()),
                Frame.Get_Encoded().Num(),
                InCaptureClock - Frame.Get_CaptureTime()});
        }

        const auto Pacing = ck::voice_chat::codec::Select_BundlesToSend(Entries,
            UCk_Utils_VoiceChat_Settings_UE::Get_MaxVoiceBytesPerConnectionPerTick(),
            MaxOutboundFrameAge);

        if (NOT Pacing.Get_StaleDropIds().IsEmpty())
        {
            const auto StaleIds = TSet<int32>{Pacing.Get_StaleDropIds()};
            InOutFrames.RemoveAll([&](const ck::FCk_VoiceChat_OutboundFrame& InFrame)
            { return StaleIds.Contains(static_cast<int32>(InFrame.Get_Seq())); });

            INC_DWORD_STAT_BY(STAT_CkVoiceChat_FramesDroppedStale, StaleIds.Num());
        }

        if (Pacing.Get_SendIds().IsEmpty())
        { return; }

        auto TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InTalker);

        if (NOT TransientEntity.Has<ck::FFragment_VoiceChat_ChannelRegistry>())
        { return; }

        auto EligibleChannelIdxs = TArray<uint8>{};
        for (const auto& Entry : TransientEntity.Get<ck::FFragment_VoiceChat_ChannelRegistry>().Get_Entries())
        {
            const auto& Channel = Entry.Get_Channel();

            if (ck::Is_NOT_Valid(Channel))
            { continue; }

            if (NOT UCk_Utils_VoiceChannel_UE::Get_IsMember(Channel, InTalker))
            { continue; }

            if (UCk_Utils_VoiceChannel_UE::Get_MemberFlags(Channel, InTalker).Get_CanTalk() != ECk_EnableDisable::Enable)
            { continue; }

            EligibleChannelIdxs.Emplace(Entry.Get_ChannelIdx());
        }

        if (EligibleChannelIdxs.IsEmpty())
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InTalker);
        const auto IsHost = UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InTalker);

        auto* Relay = static_cast<ACk_VoiceChatRelay_UE*>(nullptr);
        auto* HostSender = static_cast<APlayerState*>(nullptr);

        if (IsHost)
        {
            // No self-RPC on a listen server: the host's frames go straight into its own routing
            // inbox, stamped with the host player.
            HostSender = Get_LocalPlayerState(World);
        }
        else
        {
            Relay = ResolveRelayChannel_ForPlayer(World, Get_LocalPlayerState(World));

            if (ck::Is_NOT_Valid(Relay))
            { return; }
        }

        const auto FramesPerBundle = FMath::Clamp(UCk_Utils_VoiceChat_Settings_UE::Get_FramesPerRpc(), 1, 3);
        const auto SentIds = TSet<int32>{Pacing.Get_SendIds()};

        auto BundleFrames = TArray<TArray<uint8>>{};
        auto BundleFirstSeq = uint16{0};
        auto BundleBytes = ck::voice_chat::codec::BundleHeaderBytes;

        const auto FlushBundle = [&]() -> void
        {
            if (BundleFrames.IsEmpty())
            { return; }

            const auto NumFramesInBundle = BundleFrames.Num();

            for (const auto ChannelIdx : EligibleChannelIdxs)
            {
                const auto Header = FCk_VoiceChat_BundleHeader{BundleFirstSeq, ChannelIdx, InAmplitudeQ8};
                auto Packed = ck::voice_chat::codec::Pack_Bundle(Header, BundleFrames);

                if (Packed.IsEmpty())
                { continue; }

                if (IsHost)
                {
                    auto& Inbox = InTalker.AddOrGet<ck::FFragment_VoiceTalker_ServerInbox>().Get_Bundles();

                    if (Inbox.Num() >= ck::VoiceChat_MaxInboxBundles)
                    {
                        INC_DWORD_STAT(STAT_CkVoiceChat_BundlesDroppedInboxFull_Talker);
                        continue;
                    }

                    Inbox.Emplace(ck::FCk_VoiceChat_InboundBundle{MoveTemp(Packed), MakeWeakObjectPtr(HostSender)});
                }
                else
                {
                    Relay->Server_SendVoiceBundle(InTalker, Packed);
                }
            }

            INC_DWORD_STAT_BY(STAT_CkVoiceChat_FramesSent, NumFramesInBundle);

            BundleFrames.Reset();
            BundleBytes = ck::voice_chat::codec::BundleHeaderBytes;
        };

        for (const auto& Frame : InOutFrames)
        {
            if (NOT SentIds.Contains(static_cast<int32>(Frame.Get_Seq())))
            { continue; }

            const auto FrameWireBytes = ck::voice_chat::codec::FrameSizePrefixBytes + Frame.Get_Encoded().Num();

            if (NOT BundleFrames.IsEmpty()
                && (BundleFrames.Num() >= FramesPerBundle || BundleBytes + FrameWireBytes > MaxPackedBundleBytes))
            { FlushBundle(); }

            if (BundleFrames.IsEmpty())
            { BundleFirstSeq = Frame.Get_Seq(); }

            BundleFrames.Emplace(Frame.Get_Encoded());
            BundleBytes += FrameWireBytes;
        }
        FlushBundle();

        InOutFrames.RemoveAll([&](const ck::FCk_VoiceChat_OutboundFrame& InFrame)
        { return SentIds.Contains(static_cast<int32>(InFrame.Get_Seq())); });
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_VoiceTalker_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity,
            const FFragment_VoiceTalker_Params& InParams)
        -> void
    {
        InVoiceTalkerEntity.Remove<MarkedDirtyBy>();

        voice_chat::VeryVerbose(TEXT("Setup complete for VoiceTalker [{}]"), InVoiceTalkerEntity);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoiceTalker_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity,
            const FFragment_VoiceTalker_Params& InParams,
            FFragment_VoiceTalker_Tunables& InTunables,
            FFragment_VoiceTalker_Capture& InCapture,
            FFragment_VoiceTalker_Playout& InPlayout,
            FFragment_VoiceTalker_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InVoiceTalkerEntity, Result);

            if (DoHandleRequest(InVoiceTalkerEntity, InParams, InTunables, InCapture, InPlayout, InRequest))
            {
                Result = ECk_Request_OperationResult::Succeeded;
            }
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        {
            InVoiceTalkerEntity.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_VoiceTalker_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_VoiceTalker_Params& InParams,
            FFragment_VoiceTalker_Tunables& InTunables,
            FFragment_VoiceTalker_Capture& InCapture,
            FFragment_VoiceTalker_Playout& InPlayout,
            const FCk_Request_VoiceTalker_StartTransmit& InRequest)
        -> bool
    {
        if (InHandle.Has<FTag_VoiceTalker_IsTransmitting>())
        { return true; }

        const auto TransmitIsAllowed = InTunables.Get_TransmitMode() != ECk_VoiceChat_TransmitMode::Disabled;
        CK_ENSURE_IF_NOT(TransmitIsAllowed,
            TEXT("StartTransmit on VoiceTalker [{}] whose transmit mode is Disabled"), InHandle)
        {}
        if (NOT TransmitIsAllowed)
        { return false; }

        const auto SampleRate = UCk_Utils_VoiceChat_Settings_UE::Get_SampleRate();

        if (NOT InCapture._CaptureSource.IsValid())
        {
            InCapture._CaptureSource = MakeShared<FCk_VoiceChat_CaptureSource_Engine>(SampleRate);
        }

        if (NOT InCapture._Encoder.IsValid())
        {
            constexpr auto NumChannels = 1;
            InCapture._Encoder = FVoiceModule::Get().CreateVoiceEncoder(
                SampleRate, NumChannels, EAudioEncodeHint::VoiceEncode_Voice);

            const auto EncoderCreated = InCapture._Encoder.IsValid();
            CK_ENSURE_IF_NOT(EncoderCreated,
                TEXT("Opus encoder could not be created for VoiceTalker [{}] - the engine Voice module "
                     "is disabled ([Voice] bEnabled=true missing in the host project's DefaultEngine.ini?)"),
                InHandle)
            {}
            if (NOT EncoderCreated)
            { return false; }

            InCapture._Encoder->SetBitrate(UCk_Utils_VoiceChat_Settings_UE::Get_BitrateBps());
        }

        if (InParams.Get_Loopback() == ECk_EnableDisable::Enable && NOT InPlayout._LoopbackDecoder.IsValid())
        {
            constexpr auto NumChannels = 1;
            InPlayout._LoopbackDecoder = FVoiceModule::Get().CreateVoiceDecoder(SampleRate, NumChannels);
        }

        const auto CaptureStarted = InCapture._CaptureSource->Start();
        if (NOT CaptureStarted)
        { return false; }

        // The synth is created only AFTER capture is known to be running - creating it earlier
        // left a registered, silently-running component behind when capture failed to start.
        if (InParams.Get_Loopback() == ECk_EnableDisable::Enable)
        {
            FProcessor_VoiceTalker_ReceivePlayback::TryCreate_PlaybackSynth(InHandle, InPlayout);
        }

        InHandle.Add<FTag_VoiceTalker_IsTransmitting>();
        UUtils_Signal_OnVoiceTalker_TransmitStarted::Broadcast(InHandle, MakePayload(InHandle));

        return true;
    }

    auto
        FProcessor_VoiceTalker_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_VoiceTalker_Params& InParams,
            FFragment_VoiceTalker_Tunables& InTunables,
            FFragment_VoiceTalker_Capture& InCapture,
            FFragment_VoiceTalker_Playout& InPlayout,
            const FCk_Request_VoiceTalker_StopTransmit& InRequest)
        -> bool
    {
        if (NOT InHandle.Has<FTag_VoiceTalker_IsTransmitting>())
        { return true; }

        if (InCapture._CaptureSource.IsValid())
        {
            InCapture._CaptureSource->Stop();
        }
        InCapture._PendingPcm.Reset();
        InCapture._OutboundFrames.Reset();

        InHandle.Remove<FTag_VoiceTalker_IsTransmitting>();

        if (InHandle.Has<FTag_VoiceTalker_IsSpeaking>())
        {
            InHandle.Remove<FTag_VoiceTalker_IsSpeaking>();
            UUtils_Signal_OnVoiceTalker_SpeakingStateChanged::Broadcast(InHandle, MakePayload(InHandle, false));
        }

        UUtils_Signal_OnVoiceTalker_TransmitStopped::Broadcast(InHandle, MakePayload(InHandle));

        return true;
    }

    auto
        FProcessor_VoiceTalker_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_VoiceTalker_Params& InParams,
            FFragment_VoiceTalker_Tunables& InTunables,
            FFragment_VoiceTalker_Capture& InCapture,
            FFragment_VoiceTalker_Playout& InPlayout,
            const FCk_Request_VoiceTalker_SetTransmitMode& InRequest)
        -> bool
    {
        InTunables._TransmitMode = InRequest.Get_TransmitMode();
        return true;
    }

    auto
        FProcessor_VoiceTalker_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_VoiceTalker_Params& InParams,
            FFragment_VoiceTalker_Tunables& InTunables,
            FFragment_VoiceTalker_Capture& InCapture,
            FFragment_VoiceTalker_Playout& InPlayout,
            const FCk_Request_VoiceTalker_SetInputGain& InRequest)
        -> bool
    {
        InTunables._InputGain = InRequest.Get_InputGain();
        return true;
    }

    auto
        FProcessor_VoiceTalker_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_VoiceTalker_Params& InParams,
            FFragment_VoiceTalker_Tunables& InTunables,
            FFragment_VoiceTalker_Capture& InCapture,
            FFragment_VoiceTalker_Playout& InPlayout,
            const FCk_Request_VoiceTalker_SetSelfMute& InRequest)
        -> bool
    {
        InTunables._SelfMute = InRequest.Get_SelfMute();
        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoiceTalker_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity,
            const FFragment_VoiceTalker_Requests& InRequests)
        -> void
    {
        ck::request::FireCancelledForPending(InVoiceTalkerEntity, InRequests.Get_Requests());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoiceTalker_Capture::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity,
            const FFragment_VoiceTalker_Params& InParams,
            const FFragment_VoiceTalker_Tunables& InTunables,
            FFragment_VoiceTalker& InVoiceTalkerComp,
            FFragment_VoiceTalker_Capture& InCapture,
            FFragment_VoiceTalker_Playout& InPlayout)
        -> void
    {
        using namespace ck_voice_talker_processor;

        if (NOT InCapture._CaptureSource.IsValid())
        { return; }

        InCapture._CaptureClock = InCapture._CaptureClock + InDeltaT;
        InCapture._CaptureSource->Tick(InDeltaT);

        const auto DrainStartBytes = InCapture._PendingPcm.Num();
        InCapture._CaptureSource->DrainPcm(InCapture._PendingPcm);

        const auto SampleRate = UCk_Utils_VoiceChat_Settings_UE::Get_SampleRate();

        if (const auto DrainedBytes = InCapture._PendingPcm.Num() - DrainStartBytes;
            DrainedBytes > 0)
        {
            auto RawTap = TArray<uint8>{};
            RawTap.Append(InCapture._PendingPcm.GetData() + DrainStartBytes, DrainedBytes);
            UUtils_Signal_OnVoiceTalker_FramesCaptured::Broadcast(InVoiceTalkerEntity,
                MakePayload(InVoiceTalkerEntity, RawTap, SampleRate));
        }

        const auto FrameBytes = Get_FrameBytes();
        const auto FrameDurationSeconds = Get_FrameDurationSeconds();

        const auto VadParams = FCk_VoiceChat_VadParams{}.Set_Threshold(InParams.Get_VadThreshold());
        const auto IsMuted = InTunables.Get_SelfMute() == ECk_EnableDisable::Enable;
        const auto ModeAllowsFrames = InTunables.Get_TransmitMode() != ECk_VoiceChat_TransmitMode::Disabled;

        auto MaxRmsThisTick = 0.0f;
        auto FramesMeasuredThisTick = 0;
        auto VadIsOpen = InVoiceTalkerEntity.Has<FTag_VoiceTalker_IsSpeaking>();

        while (InCapture._PendingPcm.Num() >= FrameBytes)
        {
            auto FramePcm = TArray<uint8>{};
            FramePcm.Append(InCapture._PendingPcm.GetData(), FrameBytes);
            InCapture._PendingPcm.RemoveAt(0, FrameBytes);

            INC_DWORD_STAT(STAT_CkVoiceChat_FramesCaptured);

            const auto Samples = TArrayView<int16>{
                reinterpret_cast<int16*>(FramePcm.GetData()), FrameBytes / 2};

            Apply_Gain(Samples, InTunables.Get_InputGain());

            const auto Rms = voice_chat::codec::Compute_Rms(Samples);
            MaxRmsThisTick = FMath::Max(MaxRmsThisTick, Rms);
            ++FramesMeasuredThisTick;

            VadIsOpen = InCapture._Vad.Update(Rms, FCk_Time{FrameDurationSeconds}, VadParams);

            if (NOT VadIsOpen || IsMuted || NOT ModeAllowsFrames || NOT InCapture._Encoder.IsValid())
            { continue; }

            auto Encoded = TArray<uint8>{};
            Encoded.SetNumUninitialized(2048);
            auto EncodedSize = static_cast<uint32>(Encoded.Num());
            InCapture._Encoder->Encode(FramePcm.GetData(), FrameBytes, Encoded.GetData(), EncodedSize);

            if (EncodedSize == 0)
            { continue; }

            Encoded.SetNum(static_cast<int32>(EncodedSize));

            INC_DWORD_STAT(STAT_CkVoiceChat_FramesEncoded);

            const auto FrameSeq = InCapture._Seq++;

            InCapture._OutboundFrames.Emplace(FCk_VoiceChat_OutboundFrame{
                FrameSeq, Encoded, InCapture._CaptureClock});

            if (InParams.Get_Loopback() == ECk_EnableDisable::Enable)
            {
                InPlayout._LoopbackJitter.Push(FrameSeq, MoveTemp(Encoded),
                    InCapture._CaptureClock, FCk_VoiceChat_JitterParams{});
            }
        }

        // Amplitude holds its last measured value on frameless ticks (capture cadence 20 ms vs
        // ~8-16 ms tick) - zeroing here made Get_CurrentAmplitude flicker mid-speech, and the
        // wire header plus the routing fairness clamp both read this value.
        if (FramesMeasuredThisTick > 0)
        {
            InVoiceTalkerComp._AmplitudeQ8 = voice_chat::codec::Quantize_Amplitude(MaxRmsThisTick);
        }

        const auto IsSpeakingNow = VadIsOpen && NOT IsMuted && ModeAllowsFrames;
        const auto WasSpeaking = InVoiceTalkerEntity.Has<FTag_VoiceTalker_IsSpeaking>();

        if (IsSpeakingNow != WasSpeaking)
        {
            if (IsSpeakingNow)
            { InVoiceTalkerEntity.Add<FTag_VoiceTalker_IsSpeaking>(); }
            else
            { InVoiceTalkerEntity.Remove<FTag_VoiceTalker_IsSpeaking>(); }

            UUtils_Signal_OnVoiceTalker_SpeakingStateChanged::Broadcast(InVoiceTalkerEntity,
                MakePayload(InVoiceTalkerEntity, IsSpeakingNow));
        }

        Send_OutboundFrames(InVoiceTalkerEntity, InVoiceTalkerComp.Get_AmplitudeQ8(),
            InCapture._CaptureClock, InCapture._OutboundFrames);

        if (InParams.Get_Loopback() != ECk_EnableDisable::Enable)
        { return; }

        FProcessor_VoiceTalker_ReceivePlayback::Drain_Playout(InPlayout, InDeltaT);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoiceTalker_ReceivePlayback::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity,
            FFragment_VoiceTalker& InVoiceTalkerComp,
            FFragment_VoiceTalker_Playout& InPlayout,
            FFragment_VoiceTalker_ReceiveInbox& InInbox)
        -> void
    {
        using namespace ck_voice_talker_processor;

        InPlayout._ReceiveClock = InPlayout._ReceiveClock + InDeltaT;

        auto BundlesCopy = InInbox.Get_PackedBundles();
        InInbox.Get_PackedBundles().Reset();

        // Defense in depth for the mute race: the server-side exclusion is the privacy property,
        // but a bundle already in flight when the mute landed still arrives - the local ears
        // drop it here so mute is instant on this machine. The playout drain below still runs,
        // flushing any pre-mute jitter tail.
        if (NOT BundlesCopy.IsEmpty())
        {
            auto TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InVoiceTalkerEntity);

            if (TransientEntity.Has<FFragment_VoiceChat_LocalListener>())
            {
                const auto& LocalListener = TransientEntity.Get<FFragment_VoiceChat_LocalListener>().Get_Listener();

                if (ck::IsValid(LocalListener)
                    && LocalListener.Get<FFragment_VoiceListener_Current>().Get_MutedTalkers().Contains(InVoiceTalkerEntity))
                { BundlesCopy.Reset(); }
            }
        }

        auto BestDeliveringChannel = FCk_Handle_VoiceChannel{};

        for (const auto& Packed : BundlesCopy)
        {
            const auto Unpacked = voice_chat::codec::Unpack_Bundle(Packed);

            if (NOT Unpacked.IsSet())
            {
                // The route validated this bundle before forwarding - a malformed arrival here
                // is wire corruption, not a caller error.
                INC_DWORD_STAT(STAT_CkVoiceChat_ReceiveDroppedMalformed);
                continue;
            }

            // The synth renders with ONE channel's audio config; when bundles interleave across
            // channels, the highest-Priority delivering channel wins (the single-playback
            // dedupe). An unresolvable idx just means the control plane hasn't composed that
            // channel here yet - the audio still plays, flat. A channel still LOADING its audio
            // assets is skipped the same way: latching it would apply null config and nothing
            // re-applies on load completion - the next drain after the load finishes selects it.
            if (const auto DeliveringChannel = UCk_Utils_VoiceChannel_UE::TryGet_ChannelByIdx(
                    InVoiceTalkerEntity, Unpacked->Get_Header().Get_ChannelIdx());
                ck::IsValid(DeliveringChannel) &&
                NOT DeliveringChannel.Has<FTag_VoiceChannel_PendingAssetLoad>() &&
                (ck::Is_NOT_Valid(BestDeliveringChannel) ||
                 UCk_Utils_VoiceChannel_UE::Get_Priority(DeliveringChannel) >
                 UCk_Utils_VoiceChannel_UE::Get_Priority(BestDeliveringChannel)))
            {
                BestDeliveringChannel = DeliveringChannel;
            }

            if (NOT InPlayout._LoopbackDecoder.IsValid())
            {
                constexpr auto NumChannels = 1;
                InPlayout._LoopbackDecoder = FVoiceModule::Get().CreateVoiceDecoder(
                    UCk_Utils_VoiceChat_Settings_UE::Get_SampleRate(), NumChannels);
            }

            // Remote amplitude rides the header (the server never decodes) - mirror it so
            // Get_CurrentAmplitude works for remote talkers exactly like local ones.
            InVoiceTalkerComp._AmplitudeQ8 = Unpacked->Get_Header().Get_AmplitudeQ8();

            // Send-side invariant: a bundle's frames are one contiguous seq run (staleness drops
            // a prefix, the budget cuts a suffix), so frame i is header seq + i.
            const auto FirstSeq = Unpacked->Get_Header().Get_Seq();

            auto FrameOffset = uint16{0};
            for (const auto& Frame : Unpacked->Get_Frames())
            {
                InPlayout._LoopbackJitter.Push(static_cast<uint16>(FirstSeq + FrameOffset), Frame,
                    InPlayout._ReceiveClock, FCk_VoiceChat_JitterParams{});
                ++FrameOffset;
            }
        }

        if (ck::IsValid(BestDeliveringChannel) && BestDeliveringChannel != InPlayout._PlaybackConfigChannel)
        {
            InPlayout._PlaybackConfigChannel = BestDeliveringChannel;

            if (InPlayout._LoopbackSynth.IsValid())
            { Apply_SynthChannelConfig(InVoiceTalkerEntity, InPlayout); }
        }

        // Remote amplitude parity, release half: the header mirror only ever WRITES on arrival,
        // so a sender whose VAD closed leaves the last (loud) value frozen - unlike the local
        // side, whose capture keeps measuring ambient RMS. Once the stream has been idle past
        // the sender's own stale-drop age, the spurt is over: release to zero. The arrival-clock
        // guard keeps this off capture-loopback talkers (they never receive).
        if (NOT BundlesCopy.IsEmpty())
        {
            InPlayout._LastBundleArrivalClock = InPlayout._ReceiveClock;
        }
        else if (InPlayout._LastBundleArrivalClock > FCk_Time{0.0} &&
                 InVoiceTalkerComp._AmplitudeQ8 > 0 &&
                 InPlayout._ReceiveClock - InPlayout._LastBundleArrivalClock > MaxOutboundFrameAge)
        {
            InVoiceTalkerComp._AmplitudeQ8 = 0;
        }

        if (NOT BundlesCopy.IsEmpty())
        {
            TryCreate_PlaybackSynth(InVoiceTalkerEntity, InPlayout);
            Evaluate_HybridRenderMode(InVoiceTalkerEntity, InPlayout);
        }

        Drain_Playout(InPlayout, InDeltaT);
    }

    auto
        FProcessor_VoiceTalker_ReceivePlayback::
        Drain_Playout(
            FFragment_VoiceTalker_Playout& InPlayout,
            FCk_Time InDeltaT)
        -> void
    {
        using namespace ck_voice_talker_processor;

        if (NOT InPlayout._LoopbackDecoder.IsValid())
        {
            InPlayout._LoopbackPopAccumulator = FCk_Time{0.0};
            return;
        }

        const auto FrameBytes = Get_FrameBytes();
        const auto FrameDuration = FCk_Time{Get_FrameDurationSeconds()};

        InPlayout._LoopbackPopAccumulator = InPlayout._LoopbackPopAccumulator + InDeltaT;

        const auto JitterParams = FCk_VoiceChat_JitterParams{};
        const auto DecodeCapacityBytes = EngineDecodeCapacityFrames * FrameBytes;

        while (InPlayout._LoopbackPopAccumulator >= FrameDuration)
        {
            InPlayout._LoopbackPopAccumulator = InPlayout._LoopbackPopAccumulator - FrameDuration;

            const auto PopResult = InPlayout._LoopbackJitter.Pop(JitterParams);

            if (PopResult.Get_Type() == ECk_VoiceChat_JitterPop::Wait)
            { continue; }

            if (PopResult.Get_Type() == ECk_VoiceChat_JitterPop::Conceal)
            {
                INC_DWORD_STAT(STAT_CkVoiceChat_FramesConcealed);

                InPlayout._LoopbackDecodedPcm.AddZeroed(FrameBytes);

                if (InPlayout._LoopbackSynth.IsValid())
                {
                    auto SilentFrame = TArray<int16>{};
                    SilentFrame.SetNumZeroed(FrameBytes / 2);
                    InPlayout._LoopbackSynth->Enqueue_DecodedPcm(SilentFrame);
                }
                continue;
            }

            auto Decoded = TArray<uint8>{};
            Decoded.SetNumUninitialized(DecodeCapacityBytes);
            auto DecodedSize = static_cast<uint32>(Decoded.Num());
            InPlayout._LoopbackDecoder->Decode(PopResult.Get_Frame().GetData(),
                PopResult.Get_Frame().Num(), Decoded.GetData(), DecodedSize);

            if (DecodedSize > 0)
            {
                INC_DWORD_STAT(STAT_CkVoiceChat_FramesDecoded);

                InPlayout._LoopbackDecodedPcm.Append(Decoded.GetData(), static_cast<int32>(DecodedSize));

                if (InPlayout._LoopbackSynth.IsValid())
                {
                    InPlayout._LoopbackSynth->Enqueue_DecodedPcm(TArrayView<const int16>{
                        reinterpret_cast<const int16*>(Decoded.GetData()),
                        static_cast<int32>(DecodedSize) / 2});
                }
            }
        }

        if (InPlayout._LoopbackDecodedPcm.Num() > MaxLoopbackDecodedBytes)
        {
            InPlayout._LoopbackDecodedPcm.RemoveAt(0,
                InPlayout._LoopbackDecodedPcm.Num() - MaxLoopbackDecodedBytes);
        }
    }

    auto
        FProcessor_VoiceTalker_ReceivePlayback::
        TryCreate_PlaybackSynth(
            HandleType InVoiceTalkerEntity,
            FFragment_VoiceTalker_Playout& InPlayout)
        -> void
    {
        if (InPlayout._LoopbackSynth.IsValid())
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVoiceTalkerEntity);

        if (ck::IsValid(World) && World->GetAudioDevice().IsValid())
        {
            auto* Synth = NewObject<UCk_VoiceChatSynthComponent_UE>(World);
            Synth->bAutoActivate = false;
            Synth->RegisterComponentWithWorld(World);

            if (auto* OwningActor = UCk_Utils_OwningActor_UE::TryGet_EntityOwningActor(InVoiceTalkerEntity);
                ck::IsValid(OwningActor))
            {
                if (auto* AttachTarget = OwningActor->GetRootComponent();
                    ck::IsValid(AttachTarget))
                {
                    const auto& SocketName = InVoiceTalkerEntity.Get<FFragment_VoiceTalker_Params>().Get_PlaybackAttachSocketName();
                    Synth->AttachToComponent(AttachTarget,
                        FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
                }
            }

            InPlayout._LoopbackSynth = TStrongObjectPtr{Synth};

            Apply_SynthChannelConfig(InVoiceTalkerEntity, InPlayout);
        }
    }

    auto
        FProcessor_VoiceTalker_ReceivePlayback::
        Apply_SynthChannelConfig(
            HandleType InVoiceTalkerEntity,
            FFragment_VoiceTalker_Playout& InPlayout)
        -> void
    {
        auto* Synth = InPlayout._LoopbackSynth.Get();

        if (ck::Is_NOT_Valid(Synth))
        { return; }

        auto Policy = ECk_VoiceChat_SpatializationPolicy::Global2D;
        auto Attenuation = static_cast<USoundAttenuation*>(nullptr);
        auto SourceEffectChain = static_cast<USoundEffectSourcePresetChain*>(nullptr);

        if (const auto& ConfigChannel = InPlayout._PlaybackConfigChannel;
            ck::IsValid(ConfigChannel))
        {
            Policy = UCk_Utils_VoiceChannel_UE::Get_SpatializationPolicy(ConfigChannel);
            Attenuation = UCk_Utils_VoiceChannel_UE::Get_ResolvedAttenuation(ConfigChannel);
            SourceEffectChain = UCk_Utils_VoiceChannel_UE::Get_ResolvedSourceEffectChain(ConfigChannel);
        }

        // HybridRadio near = plain proximity speech (spatialized, NO radio filter); far = flat
        // radio through the channel's chain. Positional3D keeps its authored chain in 3D. An
        // unattached synth has no world position, so it cannot RENDER near regardless of the
        // computed state - it falls back to radio (flat + chain), never to bare flat.
        const auto CanSpatialize = ck::IsValid(Synth->GetAttachParent());

        const auto RenderNear =
            Policy == ECk_VoiceChat_SpatializationPolicy::HybridRadio &&
            InPlayout._HybridRenderNear.Get(false) &&
            CanSpatialize;

        const auto Spatialize =
            (Policy == ECk_VoiceChat_SpatializationPolicy::Positional3D || RenderNear) &&
            CanSpatialize;

        const auto ApplyEffectChain =
            Policy != ECk_VoiceChat_SpatializationPolicy::HybridRadio || NOT RenderNear;

        Synth->Stop();
        Synth->bAllowSpatialization = Spatialize;
        Synth->AttenuationSettings = Spatialize ? Attenuation : nullptr;
        Synth->SourceEffectChain = ApplyEffectChain ? SourceEffectChain : nullptr;
        Synth->Start();
    }

    auto
        FProcessor_VoiceTalker_ReceivePlayback::
        Evaluate_HybridRenderMode(
            HandleType InVoiceTalkerEntity,
            FFragment_VoiceTalker_Playout& InPlayout)
        -> void
    {
        const auto& ConfigChannel = InPlayout._PlaybackConfigChannel;

        if (ck::Is_NOT_Valid(ConfigChannel) ||
            UCk_Utils_VoiceChannel_UE::Get_SpatializationPolicy(ConfigChannel) !=
                ECk_VoiceChat_SpatializationPolicy::HybridRadio)
        {
            InPlayout._HybridRenderNear.Reset();
            return;
        }

        // The talker's position comes from its owning actor - the synth is attached to that
        // actor's root anyway, and the near/far STATE is a distance decision, not a render one,
        // so it must compute even where no audio device exists (headless: the state is what the
        // specs pin; Apply's attach guard still gates the actual render independently).
        auto* OwningActor = UCk_Utils_OwningActor_UE::TryGet_EntityOwningActor(InVoiceTalkerEntity);

        if (ck::Is_NOT_Valid(OwningActor))
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVoiceTalkerEntity);

        if (ck::Is_NOT_Valid(World))
        { return; }

        auto* LocalPlayerController = World->GetFirstPlayerController();

        if (ck::Is_NOT_Valid(LocalPlayerController))
        { return; }

        auto ListenerLocation = FVector{};
        auto ListenerFront = FVector{};
        auto ListenerRight = FVector{};
        LocalPlayerController->GetAudioListenerPosition(ListenerLocation, ListenerFront, ListenerRight);

        const auto DistSq = FVector::DistSquared(ListenerLocation, OwningActor->GetActorLocation());
        const auto RangeCm = UCk_Utils_VoiceChannel_UE::Get_AudibleRange(ConfigChannel);
        const auto OuterCm = RangeCm + UCk_Utils_VoiceChat_Settings_UE::Get_ProximityHysteresisMarginCm();

        // The Route asymmetry, mirrored: become near INSIDE the range, stay near until
        // range + margin - a speaker hovering at the boundary never flips per drain.
        const auto WasNear = InPlayout._HybridRenderNear.Get(false);
        const auto ThresholdCm = WasNear ? OuterCm : RangeCm;
        const auto IsNear = DistSq <= ThresholdCm * ThresholdCm;

        if (InPlayout._HybridRenderNear.IsSet() && InPlayout._HybridRenderNear.GetValue() == IsNear)
        { return; }

        InPlayout._HybridRenderNear = IsNear;

        Apply_SynthChannelConfig(InVoiceTalkerEntity, InPlayout);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoiceTalker_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity,
            FFragment_VoiceTalker_Capture& InCapture,
            FFragment_VoiceTalker_Playout& InPlayout)
        -> void
    {
        voice_chat::Verbose(TEXT("Tearing down VoiceTalker [{}]"), InVoiceTalkerEntity);

        // Churn-coupled hygiene for the world-scoped authority maps: sweep this talker's entries
        // and drop any stale player keys met on the way. Both fragments exist only where the
        // authority wrote them; every talker destroy prunes, so long-lived servers with churn
        // never accumulate dead handles (a departed player's own key falls to the NEXT sweep
        // after its PlayerState goes stale).
        auto TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InVoiceTalkerEntity);

        if (TransientEntity.Has<FFragment_VoiceChat_ServeHistory>())
        {
            auto& History = TransientEntity.Get<FFragment_VoiceChat_ServeHistory>().Get_LastServedFrame();

            for (auto It = History.CreateIterator(); It; ++It)
            {
                if (NOT It->Key.IsValid())
                {
                    It.RemoveCurrent();
                    continue;
                }

                It->Value.Remove(InVoiceTalkerEntity);
            }
        }

        if (TransientEntity.Has<FFragment_VoiceChat_ListenerMuteMatrix>())
        {
            auto& MutedByPlayer = TransientEntity.Get<FFragment_VoiceChat_ListenerMuteMatrix>().Get_MutedByPlayer();

            for (auto It = MutedByPlayer.CreateIterator(); It; ++It)
            {
                if (NOT It->Key.IsValid())
                {
                    It.RemoveCurrent();
                    continue;
                }

                It->Value.Remove(InVoiceTalkerEntity);
            }
        }

        if (InCapture._CaptureSource.IsValid())
        {
            InCapture._CaptureSource->Stop();
        }

        if (auto* Synth = InPlayout._LoopbackSynth.Get();
            ck::IsValid(Synth))
        {
            Synth->Stop();
            Synth->DestroyComponent();
        }
        InPlayout._LoopbackSynth.Reset();

        InCapture._CaptureSource.Reset();
        InCapture._Encoder.Reset();
        InPlayout._LoopbackDecoder.Reset();
    }
}

// --------------------------------------------------------------------------------------------------------------------
