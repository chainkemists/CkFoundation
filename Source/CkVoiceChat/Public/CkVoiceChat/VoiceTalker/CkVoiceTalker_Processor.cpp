#include "CkVoiceTalker_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkVoiceChat/Capture/CkVoiceChat_CaptureSource.h"
#include "CkVoiceChat/CkVoiceChat_Log.h"
#include "CkVoiceChat/CkVoiceChat_Stats.h"
#include "CkVoiceChat/Settings/CkVoiceChat_Settings.h"

#include "Interfaces/VoiceCodec.h"
#include "VoiceModule.h"

DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Frames Captured"),  STAT_CkVoiceChat_FramesCaptured,  STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Frames Encoded"),   STAT_CkVoiceChat_FramesEncoded,   STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Frames Concealed"), STAT_CkVoiceChat_FramesConcealed, STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Frames Decoded"),   STAT_CkVoiceChat_FramesDecoded,   STATGROUP_CkVoiceChat);

CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceTalker_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceTalker_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceTalker_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceTalker_Capture);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceTalker_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voice_talker_processor
{
    // FVoiceDecoderOpus refuses to decode unless the output buffer holds MAX_OPUS_FRAMES (6)
    // whole frames - VoiceCodecOpus.cpp:20 + the UncompressedBufferAvail gate in Decode.
    constexpr auto EngineDecodeCapacityFrames = 6;

    // Test-readable loopback PCM is bounded so long-running talkers don't grow it forever.
    constexpr auto MaxLoopbackDecodedBytes = 48000 * 2 * 2;   // 2 s at 48 kHz mono 16-bit

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
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_VoiceTalker_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity,
            const FFragment_VoiceTalker_Params& InParams,
            FFragment_VoiceTalker_Current& InCurrent)
        -> void
    {
        InVoiceTalkerEntity.Remove<MarkedDirtyBy>();

        InCurrent._TransmitMode = InParams.Get_TransmitMode();
        InCurrent._InputGain = InParams.Get_InputGain();

        voice_chat::VeryVerbose(TEXT("Setup complete for VoiceTalker [{}]"), InVoiceTalkerEntity);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoiceTalker_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity,
            const FFragment_VoiceTalker_Params& InParams,
            FFragment_VoiceTalker_Current& InCurrent,
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

            if (DoHandleRequest(InVoiceTalkerEntity, InParams, InCurrent, InRequest))
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
            FFragment_VoiceTalker_Current& InCurrent,
            const FCk_Request_VoiceTalker_StartTransmit& InRequest)
        -> bool
    {
        if (InHandle.Has<FTag_VoiceTalker_IsTransmitting>())
        { return true; }

        const auto TransmitIsAllowed = InCurrent.Get_TransmitMode() != ECk_VoiceChat_TransmitMode::Disabled;
        CK_ENSURE_IF_NOT(TransmitIsAllowed,
            TEXT("StartTransmit on VoiceTalker [{}] whose transmit mode is Disabled"), InHandle)
        {}
        if (NOT TransmitIsAllowed)
        { return false; }

        const auto SampleRate = UCk_Utils_VoiceChat_Settings_UE::Get_SampleRate();

        if (NOT InCurrent._CaptureSource.IsValid())
        {
            InCurrent._CaptureSource = MakeShared<FCk_VoiceChat_CaptureSource_Engine>(SampleRate);
        }

        if (NOT InCurrent._Encoder.IsValid())
        {
            constexpr auto NumChannels = 1;
            InCurrent._Encoder = FVoiceModule::Get().CreateVoiceEncoder(
                SampleRate, NumChannels, EAudioEncodeHint::VoiceEncode_Voice);

            const auto EncoderCreated = InCurrent._Encoder.IsValid();
            CK_ENSURE_IF_NOT(EncoderCreated,
                TEXT("Opus encoder could not be created for VoiceTalker [{}] - the engine Voice module "
                     "is disabled ([Voice] bEnabled=true missing in the host project's DefaultEngine.ini?)"),
                InHandle)
            {}
            if (NOT EncoderCreated)
            { return false; }

            InCurrent._Encoder->SetBitrate(UCk_Utils_VoiceChat_Settings_UE::Get_BitrateBps());
        }

        if (InParams.Get_Loopback() == ECk_EnableDisable::Enable && NOT InCurrent._LoopbackDecoder.IsValid())
        {
            constexpr auto NumChannels = 1;
            InCurrent._LoopbackDecoder = FVoiceModule::Get().CreateVoiceDecoder(SampleRate, NumChannels);
        }

        if (InParams.Get_Loopback() == ECk_EnableDisable::Enable && NOT InCurrent._LoopbackSynth.IsValid())
        {
            auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

            if (ck::IsValid(World) && World->GetAudioDevice().IsValid())
            {
                auto* Synth = NewObject<UCk_VoiceChatSynthComponent_UE>(World);
                Synth->bAutoActivate = false;
                Synth->RegisterComponentWithWorld(World);
                Synth->Start();

                InCurrent._LoopbackSynth = TStrongObjectPtr{Synth};
            }
        }

        const auto CaptureStarted = InCurrent._CaptureSource->Start();
        if (NOT CaptureStarted)
        { return false; }

        InHandle.Add<FTag_VoiceTalker_IsTransmitting>();
        UUtils_Signal_OnVoiceTalker_TransmitStarted::Broadcast(InHandle, MakePayload(InHandle));

        return true;
    }

    auto
        FProcessor_VoiceTalker_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_VoiceTalker_Params& InParams,
            FFragment_VoiceTalker_Current& InCurrent,
            const FCk_Request_VoiceTalker_StopTransmit& InRequest)
        -> bool
    {
        if (NOT InHandle.Has<FTag_VoiceTalker_IsTransmitting>())
        { return true; }

        if (InCurrent._CaptureSource.IsValid())
        {
            InCurrent._CaptureSource->Stop();
        }
        InCurrent._PendingPcm.Reset();

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
            FFragment_VoiceTalker_Current& InCurrent,
            const FCk_Request_VoiceTalker_SetTransmitMode& InRequest)
        -> bool
    {
        InCurrent._TransmitMode = InRequest.Get_TransmitMode();
        return true;
    }

    auto
        FProcessor_VoiceTalker_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_VoiceTalker_Params& InParams,
            FFragment_VoiceTalker_Current& InCurrent,
            const FCk_Request_VoiceTalker_SetInputGain& InRequest)
        -> bool
    {
        InCurrent._InputGain = InRequest.Get_InputGain();
        return true;
    }

    auto
        FProcessor_VoiceTalker_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_VoiceTalker_Params& InParams,
            FFragment_VoiceTalker_Current& InCurrent,
            const FCk_Request_VoiceTalker_SetSelfMute& InRequest)
        -> bool
    {
        InCurrent._SelfMute = InRequest.Get_SelfMute();
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
            FFragment_VoiceTalker_Current& InCurrent)
        -> void
    {
        using namespace ck_voice_talker_processor;

        if (NOT InCurrent._CaptureSource.IsValid())
        { return; }

        InCurrent._CaptureClockSeconds += InDeltaT.Get_Seconds();
        InCurrent._CaptureSource->Tick(InDeltaT.Get_Seconds());

        const auto DrainStartBytes = InCurrent._PendingPcm.Num();
        InCurrent._CaptureSource->DrainPcm(InCurrent._PendingPcm);

        const auto SampleRate = UCk_Utils_VoiceChat_Settings_UE::Get_SampleRate();

        if (const auto DrainedBytes = InCurrent._PendingPcm.Num() - DrainStartBytes;
            DrainedBytes > 0)
        {
            auto RawTap = TArray<uint8>{};
            RawTap.Append(InCurrent._PendingPcm.GetData() + DrainStartBytes, DrainedBytes);
            UUtils_Signal_OnVoiceTalker_FramesCaptured::Broadcast(InVoiceTalkerEntity,
                MakePayload(InVoiceTalkerEntity, RawTap, SampleRate));
        }

        const auto FrameBytes = Get_FrameBytes();
        const auto FrameDurationSeconds = Get_FrameDurationSeconds();

        const auto VadParams = FCk_VoiceChat_VadParams{}.Set_Threshold(InParams.Get_VadThreshold());
        const auto IsMuted = InCurrent.Get_SelfMute() == ECk_EnableDisable::Enable;
        const auto ModeAllowsFrames = InCurrent.Get_TransmitMode() != ECk_VoiceChat_TransmitMode::Disabled;

        auto MaxRmsThisTick = 0.0f;
        auto VadIsOpen = InVoiceTalkerEntity.Has<FTag_VoiceTalker_IsSpeaking>();

        while (InCurrent._PendingPcm.Num() >= FrameBytes)
        {
            auto FramePcm = TArray<uint8>{};
            FramePcm.Append(InCurrent._PendingPcm.GetData(), FrameBytes);
            InCurrent._PendingPcm.RemoveAt(0, FrameBytes);

            INC_DWORD_STAT(STAT_CkVoiceChat_FramesCaptured);

            const auto Samples = TArrayView<int16>{
                reinterpret_cast<int16*>(FramePcm.GetData()), FrameBytes / 2};

            Apply_Gain(Samples, InCurrent.Get_InputGain());

            const auto Rms = voice_chat::codec::Compute_Rms(Samples);
            MaxRmsThisTick = FMath::Max(MaxRmsThisTick, Rms);

            VadIsOpen = InCurrent._Vad.Update(Rms, FCk_Time{FrameDurationSeconds}, VadParams);

            if (NOT VadIsOpen || IsMuted || NOT ModeAllowsFrames || NOT InCurrent._Encoder.IsValid())
            { continue; }

            auto Encoded = TArray<uint8>{};
            Encoded.SetNumUninitialized(2048);
            auto EncodedSize = static_cast<uint32>(Encoded.Num());
            InCurrent._Encoder->Encode(FramePcm.GetData(), FrameBytes, Encoded.GetData(), EncodedSize);

            if (EncodedSize == 0)
            { continue; }

            Encoded.SetNum(static_cast<int32>(EncodedSize));

            INC_DWORD_STAT(STAT_CkVoiceChat_FramesEncoded);

            const auto FrameSeq = InCurrent._Seq++;

            if (InParams.Get_Loopback() == ECk_EnableDisable::Enable)
            {
                InCurrent._LoopbackJitter.Push(FrameSeq, MoveTemp(Encoded),
                    FCk_Time{InCurrent._CaptureClockSeconds}, FCk_VoiceChat_JitterParams{});
            }
        }

        InCurrent._AmplitudeQ8 = voice_chat::codec::Quantize_Amplitude(MaxRmsThisTick);

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

        if (InParams.Get_Loopback() != ECk_EnableDisable::Enable || NOT InCurrent._LoopbackDecoder.IsValid())
        { return; }

        InCurrent._LoopbackPopAccumulatorSeconds += InDeltaT.Get_Seconds();

        const auto JitterParams = FCk_VoiceChat_JitterParams{};
        const auto DecodeCapacityBytes = EngineDecodeCapacityFrames * FrameBytes;

        while (InCurrent._LoopbackPopAccumulatorSeconds >= FrameDurationSeconds)
        {
            InCurrent._LoopbackPopAccumulatorSeconds -= FrameDurationSeconds;

            const auto PopResult = InCurrent._LoopbackJitter.Pop(JitterParams);

            if (PopResult.Get_Type() == ECk_VoiceChat_JitterPop::Wait)
            { continue; }

            if (PopResult.Get_Type() == ECk_VoiceChat_JitterPop::Conceal)
            {
                INC_DWORD_STAT(STAT_CkVoiceChat_FramesConcealed);

                InCurrent._LoopbackDecodedPcm.AddZeroed(FrameBytes);

                if (InCurrent._LoopbackSynth.IsValid())
                {
                    auto SilentFrame = TArray<int16>{};
                    SilentFrame.SetNumZeroed(FrameBytes / 2);
                    InCurrent._LoopbackSynth->Enqueue_DecodedPcm(SilentFrame);
                }
                continue;
            }

            auto Decoded = TArray<uint8>{};
            Decoded.SetNumUninitialized(DecodeCapacityBytes);
            auto DecodedSize = static_cast<uint32>(Decoded.Num());
            InCurrent._LoopbackDecoder->Decode(PopResult.Get_Frame().GetData(),
                PopResult.Get_Frame().Num(), Decoded.GetData(), DecodedSize);

            if (DecodedSize > 0)
            {
                INC_DWORD_STAT(STAT_CkVoiceChat_FramesDecoded);

                InCurrent._LoopbackDecodedPcm.Append(Decoded.GetData(), static_cast<int32>(DecodedSize));

                if (InCurrent._LoopbackSynth.IsValid())
                {
                    InCurrent._LoopbackSynth->Enqueue_DecodedPcm(TArrayView<const int16>{
                        reinterpret_cast<const int16*>(Decoded.GetData()),
                        static_cast<int32>(DecodedSize) / 2});
                }
            }
        }

        if (InCurrent._LoopbackDecodedPcm.Num() > MaxLoopbackDecodedBytes)
        {
            InCurrent._LoopbackDecodedPcm.RemoveAt(0,
                InCurrent._LoopbackDecodedPcm.Num() - MaxLoopbackDecodedBytes);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoiceTalker_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity,
            FFragment_VoiceTalker_Current& InCurrent)
        -> void
    {
        voice_chat::Verbose(TEXT("Tearing down VoiceTalker [{}]"), InVoiceTalkerEntity);

        if (InCurrent._CaptureSource.IsValid())
        {
            InCurrent._CaptureSource->Stop();
        }

        if (auto* Synth = InCurrent._LoopbackSynth.Get();
            ck::IsValid(Synth))
        {
            Synth->Stop();
            Synth->DestroyComponent();
        }
        InCurrent._LoopbackSynth.Reset();

        InCurrent._CaptureSource.Reset();
        InCurrent._Encoder.Reset();
        InCurrent._LoopbackDecoder.Reset();
    }
}

// --------------------------------------------------------------------------------------------------------------------
