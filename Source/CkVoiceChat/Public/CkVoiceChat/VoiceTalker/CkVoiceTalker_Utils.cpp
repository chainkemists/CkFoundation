#include "CkVoiceTalker_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkVoiceChat/Capture/CkVoiceChat_CaptureSource.h"
#include "CkVoiceChat/CkVoiceChat_Log.h"
#include "CkVoiceChat/Codec/CkVoiceChat_Codec.h"
#include "CkVoiceChat/VoiceListener/CkVoiceListener_Fragment.h"

#include <GameFramework/PlayerState.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoiceTalker_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_VoiceTalker_ParamsData& InParams)
    -> FCk_Handle_VoiceTalker
{
    ck::voice_chat::VeryVerbose(TEXT("Adding VoiceTalker feature to Entity [{}]"), InHandle);

    InHandle.Add<ck::FFragment_VoiceTalker_Params>(InParams);
    InHandle.Add<ck::FFragment_VoiceTalker_Current>();
    InHandle.Add<ck::FTag_VoiceTalker_NeedsSetup>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_VoiceTalker_UE, FCk_Handle_VoiceTalker,
    ck::FFragment_VoiceTalker_Params, ck::FFragment_VoiceTalker_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoiceTalker_UE::
    Request_StartTransmit(
        FCk_Handle_VoiceTalker& InVoiceTalker,
        const FCk_Request_VoiceTalker_StartTransmit& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VoiceTalker
{
    const auto Request = InRequest;

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InVoiceTalker.AddOrGet<ck::FFragment_VoiceTalker_Requests>()._Requests.Emplace(Request);

    return InVoiceTalker;
}

auto
    UCk_Utils_VoiceTalker_UE::
    Request_StopTransmit(
        FCk_Handle_VoiceTalker& InVoiceTalker,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VoiceTalker
{
    const auto Request = FCk_Request_VoiceTalker_StopTransmit{};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InVoiceTalker.AddOrGet<ck::FFragment_VoiceTalker_Requests>()._Requests.Emplace(Request);

    return InVoiceTalker;
}

auto
    UCk_Utils_VoiceTalker_UE::
    Request_SetTransmitMode(
        FCk_Handle_VoiceTalker& InVoiceTalker,
        ECk_VoiceChat_TransmitMode InTransmitMode,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VoiceTalker
{
    const auto Request = FCk_Request_VoiceTalker_SetTransmitMode{InTransmitMode};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InVoiceTalker.AddOrGet<ck::FFragment_VoiceTalker_Requests>()._Requests.Emplace(Request);

    return InVoiceTalker;
}

auto
    UCk_Utils_VoiceTalker_UE::
    Request_SetInputGain(
        FCk_Handle_VoiceTalker& InVoiceTalker,
        float InInputGain,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VoiceTalker
{
    const auto Request = FCk_Request_VoiceTalker_SetInputGain{InInputGain};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InVoiceTalker.AddOrGet<ck::FFragment_VoiceTalker_Requests>()._Requests.Emplace(Request);

    return InVoiceTalker;
}

auto
    UCk_Utils_VoiceTalker_UE::
    Request_SetSelfMute(
        FCk_Handle_VoiceTalker& InVoiceTalker,
        ECk_EnableDisable InSelfMute,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VoiceTalker
{
    const auto Request = FCk_Request_VoiceTalker_SetSelfMute{InSelfMute};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InVoiceTalker.AddOrGet<ck::FFragment_VoiceTalker_Requests>()._Requests.Emplace(Request);

    return InVoiceTalker;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoiceTalker_UE::
    Get_IsTransmitting(
        const FCk_Handle_VoiceTalker& InVoiceTalker)
    -> bool
{
    return InVoiceTalker.Has<ck::FTag_VoiceTalker_IsTransmitting>();
}

auto
    UCk_Utils_VoiceTalker_UE::
    Get_IsSpeaking(
        const FCk_Handle_VoiceTalker& InVoiceTalker)
    -> bool
{
    return InVoiceTalker.Has<ck::FTag_VoiceTalker_IsSpeaking>();
}

auto
    UCk_Utils_VoiceTalker_UE::
    Get_TransmitMode(
        const FCk_Handle_VoiceTalker& InVoiceTalker)
    -> ECk_VoiceChat_TransmitMode
{
    return InVoiceTalker.Get<ck::FFragment_VoiceTalker_Current>().Get_TransmitMode();
}

auto
    UCk_Utils_VoiceTalker_UE::
    Get_InputGain(
        const FCk_Handle_VoiceTalker& InVoiceTalker)
    -> float
{
    return InVoiceTalker.Get<ck::FFragment_VoiceTalker_Current>().Get_InputGain();
}

auto
    UCk_Utils_VoiceTalker_UE::
    Get_CurrentAmplitude(
        const FCk_Handle_VoiceTalker& InVoiceTalker)
    -> float
{
    return ck::voice_chat::codec::Dequantize_Amplitude(
        InVoiceTalker.Get<ck::FFragment_VoiceTalker_Current>().Get_AmplitudeQ8());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoiceTalker_UE::
    BindTo_OnTransmitStarted(
        FCk_Handle_VoiceTalker& InVoiceTalker,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_VoiceTalker& InDelegate)
    -> FCk_Handle_VoiceTalker
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnVoiceTalker_TransmitStarted, InVoiceTalker, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InVoiceTalker;
}

auto
    UCk_Utils_VoiceTalker_UE::
    UnbindFrom_OnTransmitStarted(
        FCk_Handle_VoiceTalker& InVoiceTalker,
        const FCk_Delegate_VoiceTalker& InDelegate)
    -> FCk_Handle_VoiceTalker
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnVoiceTalker_TransmitStarted, InVoiceTalker, InDelegate);
    return InVoiceTalker;
}

auto
    UCk_Utils_VoiceTalker_UE::
    BindTo_OnTransmitStopped(
        FCk_Handle_VoiceTalker& InVoiceTalker,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_VoiceTalker& InDelegate)
    -> FCk_Handle_VoiceTalker
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnVoiceTalker_TransmitStopped, InVoiceTalker, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InVoiceTalker;
}

auto
    UCk_Utils_VoiceTalker_UE::
    UnbindFrom_OnTransmitStopped(
        FCk_Handle_VoiceTalker& InVoiceTalker,
        const FCk_Delegate_VoiceTalker& InDelegate)
    -> FCk_Handle_VoiceTalker
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnVoiceTalker_TransmitStopped, InVoiceTalker, InDelegate);
    return InVoiceTalker;
}

auto
    UCk_Utils_VoiceTalker_UE::
    BindTo_OnSpeakingStateChanged(
        FCk_Handle_VoiceTalker& InVoiceTalker,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_VoiceTalker_SpeakingStateChanged& InDelegate)
    -> FCk_Handle_VoiceTalker
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnVoiceTalker_SpeakingStateChanged, InVoiceTalker, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InVoiceTalker;
}

auto
    UCk_Utils_VoiceTalker_UE::
    UnbindFrom_OnSpeakingStateChanged(
        FCk_Handle_VoiceTalker& InVoiceTalker,
        const FCk_Delegate_VoiceTalker_SpeakingStateChanged& InDelegate)
    -> FCk_Handle_VoiceTalker
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnVoiceTalker_SpeakingStateChanged, InVoiceTalker, InDelegate);
    return InVoiceTalker;
}

auto
    UCk_Utils_VoiceTalker_UE::
    BindTo_OnFramesCaptured(
        FCk_Handle_VoiceTalker& InVoiceTalker,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_VoiceTalker_FramesCaptured& InDelegate)
    -> FCk_Handle_VoiceTalker
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnVoiceTalker_FramesCaptured, InVoiceTalker, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InVoiceTalker;
}

auto
    UCk_Utils_VoiceTalker_UE::
    UnbindFrom_OnFramesCaptured(
        FCk_Handle_VoiceTalker& InVoiceTalker,
        const FCk_Delegate_VoiceTalker_FramesCaptured& InDelegate)
    -> FCk_Handle_VoiceTalker
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnVoiceTalker_FramesCaptured, InVoiceTalker, InDelegate);
    return InVoiceTalker;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoiceTalker_UE::
    Debug_InjectCaptureSource(
        FCk_Handle_VoiceTalker& InVoiceTalker,
        const TSharedPtr<ICk_VoiceChat_CaptureSource>& InCaptureSource)
    -> void
{
    InVoiceTalker.Get<ck::FFragment_VoiceTalker_Current>()._CaptureSource = InCaptureSource;
}

auto
    UCk_Utils_VoiceTalker_UE::
    Debug_Get_LoopbackDecodedPcm(
        const FCk_Handle_VoiceTalker& InVoiceTalker)
    -> const TArray<uint8>&
{
    return InVoiceTalker.Get<ck::FFragment_VoiceTalker_Current>()._LoopbackDecodedPcm;
}

auto
    UCk_Utils_VoiceTalker_UE::
    Debug_Reset_LoopbackDecodedPcm(
        FCk_Handle_VoiceTalker& InVoiceTalker)
    -> void
{
    InVoiceTalker.Get<ck::FFragment_VoiceTalker_Current>()._LoopbackDecodedPcm.Reset();
}

auto
    UCk_Utils_VoiceTalker_UE::
    Debug_InjectInboundBundle(
        FCk_Handle_VoiceTalker& InVoiceTalker,
        const TArray<uint8>& InPackedBundle,
        APlayerState* InSender)
    -> void
{
    InVoiceTalker.AddOrGet<ck::FFragment_VoiceTalker_ServerInbox>().Get_Bundles().Emplace(
        ck::FCk_VoiceChat_InboundBundle{InPackedBundle, MakeWeakObjectPtr(InSender)});
}

auto
    UCk_Utils_VoiceTalker_UE::
    Debug_Get_ReceiveInboxNum(
        const FCk_Handle_VoiceTalker& InVoiceTalker)
    -> int32
{
    if (NOT InVoiceTalker.Has<ck::FFragment_VoiceTalker_ReceiveInbox>())
    { return 0; }

    return InVoiceTalker.Get<ck::FFragment_VoiceTalker_ReceiveInbox>().Get_PackedBundles().Num();
}

auto
    UCk_Utils_VoiceTalker_UE::
    Debug_Get_ReceiveArrivedBundles(
        const FCk_Handle_VoiceTalker& InVoiceTalker)
    -> uint64
{
    if (NOT InVoiceTalker.Has<ck::FFragment_VoiceTalker_ReceiveInbox>())
    { return 0; }

    return InVoiceTalker.Get<ck::FFragment_VoiceTalker_ReceiveInbox>().Get_TotalArrivedBundles();
}

auto
    UCk_Utils_VoiceTalker_UE::
    Debug_Get_ReceiveArrivedBytes(
        const FCk_Handle_VoiceTalker& InVoiceTalker)
    -> uint64
{
    if (NOT InVoiceTalker.Has<ck::FFragment_VoiceTalker_ReceiveInbox>())
    { return 0; }

    return InVoiceTalker.Get<ck::FFragment_VoiceTalker_ReceiveInbox>().Get_TotalArrivedBytes();
}

auto
    UCk_Utils_VoiceTalker_UE::
    Debug_Get_PlaybackConfigChannel(
        const FCk_Handle_VoiceTalker& InVoiceTalker)
    -> FCk_Handle_VoiceChannel
{
    if (NOT InVoiceTalker.Has<ck::FFragment_VoiceTalker_Current>())
    { return {}; }

    return InVoiceTalker.Get<ck::FFragment_VoiceTalker_Current>().Get_PlaybackConfigChannel();
}

auto
    UCk_Utils_VoiceTalker_UE::
    Debug_Get_HybridRenderNear(
        const FCk_Handle_VoiceTalker& InVoiceTalker)
    -> TOptional<bool>
{
    if (NOT InVoiceTalker.Has<ck::FFragment_VoiceTalker_Current>())
    { return {}; }

    return InVoiceTalker.Get<ck::FFragment_VoiceTalker_Current>().Get_HybridRenderNear();
}

auto
    UCk_Utils_VoiceTalker_UE::
    Debug_Get_WorldMapTalkerEntryCount(
        const FCk_Handle& InAnyWorldHandle)
    -> int32
{
    auto TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InAnyWorldHandle);

    auto Entries = 0;

    if (TransientEntity.Has<ck::FFragment_VoiceChat_ServeHistory>())
    {
        for (const auto& [Player, PerTalker] : TransientEntity.Get<ck::FFragment_VoiceChat_ServeHistory>().Get_LastServedFrame())
        { Entries += PerTalker.Num(); }
    }

    if (TransientEntity.Has<ck::FFragment_VoiceChat_ListenerMuteMatrix>())
    {
        for (const auto& [Player, MutedTalkers] : TransientEntity.Get<ck::FFragment_VoiceChat_ListenerMuteMatrix>().Get_MutedByPlayer())
        { Entries += MutedTalkers.Num(); }
    }

    return Entries;
}

// --------------------------------------------------------------------------------------------------------------------
