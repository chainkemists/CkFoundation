#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Fragment.h"
#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Fragment_Data.h"

#include "CkVoiceTalker_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class ICk_VoiceChat_CaptureSource;
class APlayerState;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_VoiceTalker"))
class CKVOICECHAT_API UCk_Utils_VoiceTalker_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VoiceTalker_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_VoiceTalker);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Add Feature")
    static FCk_Handle_VoiceTalker
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_VoiceTalker_ParamsData& InParams);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|VoiceChat|Talker",
        DisplayName="[Ck][VoiceTalker] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_VoiceTalker
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|VoiceChat|Talker",
        DisplayName="[Ck][VoiceTalker] Handle -> VoiceTalker Handle",
        meta = (CompactNodeTitle = "<AsVoiceTalker>", BlueprintAutocast))
    static FCk_Handle_VoiceTalker
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid VoiceTalker Handle",
        Category = "Ck|Utils|VoiceChat|Talker",
        meta = (CompactNodeTitle = "INVALID_VoiceTalkerHandle", Keywords = "make"))
    static FCk_Handle_VoiceTalker
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Request Start Transmit",
              meta = (AutoCreateRefTerm = "InRequest,InDelegate"))
    static FCk_Handle_VoiceTalker
    Request_StartTransmit(
        UPARAM(ref) FCk_Handle_VoiceTalker& InVoiceTalker,
        const FCk_Request_VoiceTalker_StartTransmit& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Request Stop Transmit",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_VoiceTalker
    Request_StopTransmit(
        UPARAM(ref) FCk_Handle_VoiceTalker& InVoiceTalker,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Request Set Transmit Mode",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_VoiceTalker
    Request_SetTransmitMode(
        UPARAM(ref) FCk_Handle_VoiceTalker& InVoiceTalker,
        ECk_VoiceChat_TransmitMode InTransmitMode,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Request Set Input Gain",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_VoiceTalker
    Request_SetInputGain(
        UPARAM(ref) FCk_Handle_VoiceTalker& InVoiceTalker,
        float InInputGain,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Request Set Self Mute",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_VoiceTalker
    Request_SetSelfMute(
        UPARAM(ref) FCk_Handle_VoiceTalker& InVoiceTalker,
        ECk_EnableDisable InSelfMute,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Get Is Transmitting")
    static bool
    Get_IsTransmitting(
        const FCk_Handle_VoiceTalker& InVoiceTalker);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Get Is Speaking")
    static bool
    Get_IsSpeaking(
        const FCk_Handle_VoiceTalker& InVoiceTalker);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Get Transmit Mode")
    static ECk_VoiceChat_TransmitMode
    Get_TransmitMode(
        const FCk_Handle_VoiceTalker& InVoiceTalker);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Get Input Gain")
    static float
    Get_InputGain(
        const FCk_Handle_VoiceTalker& InVoiceTalker);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Get Current Amplitude")
    static float
    Get_CurrentAmplitude(
        const FCk_Handle_VoiceTalker& InVoiceTalker);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Bind To OnTransmitStarted")
    static FCk_Handle_VoiceTalker
    BindTo_OnTransmitStarted(
        UPARAM(ref) FCk_Handle_VoiceTalker& InVoiceTalker,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_VoiceTalker& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Unbind From OnTransmitStarted")
    static FCk_Handle_VoiceTalker
    UnbindFrom_OnTransmitStarted(
        UPARAM(ref) FCk_Handle_VoiceTalker& InVoiceTalker,
        const FCk_Delegate_VoiceTalker& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Bind To OnTransmitStopped")
    static FCk_Handle_VoiceTalker
    BindTo_OnTransmitStopped(
        UPARAM(ref) FCk_Handle_VoiceTalker& InVoiceTalker,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_VoiceTalker& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Unbind From OnTransmitStopped")
    static FCk_Handle_VoiceTalker
    UnbindFrom_OnTransmitStopped(
        UPARAM(ref) FCk_Handle_VoiceTalker& InVoiceTalker,
        const FCk_Delegate_VoiceTalker& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Bind To OnSpeakingStateChanged")
    static FCk_Handle_VoiceTalker
    BindTo_OnSpeakingStateChanged(
        UPARAM(ref) FCk_Handle_VoiceTalker& InVoiceTalker,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_VoiceTalker_SpeakingStateChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Unbind From OnSpeakingStateChanged")
    static FCk_Handle_VoiceTalker
    UnbindFrom_OnSpeakingStateChanged(
        UPARAM(ref) FCk_Handle_VoiceTalker& InVoiceTalker,
        const FCk_Delegate_VoiceTalker_SpeakingStateChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Bind To OnFramesCaptured")
    static FCk_Handle_VoiceTalker
    BindTo_OnFramesCaptured(
        UPARAM(ref) FCk_Handle_VoiceTalker& InVoiceTalker,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_VoiceTalker_FramesCaptured& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Unbind From OnFramesCaptured")
    static FCk_Handle_VoiceTalker
    UnbindFrom_OnFramesCaptured(
        UPARAM(ref) FCk_Handle_VoiceTalker& InVoiceTalker,
        const FCk_Delegate_VoiceTalker_FramesCaptured& InDelegate);

public:
    // Test seams (C++-only). InjectCaptureSource replaces the engine microphone with a scripted
    // source; call BEFORE Request_StartTransmit. The loopback PCM accessors read/clear what the
    // full gate->encode->jitter->decode loop produced.
    static auto
    Debug_InjectCaptureSource(
        FCk_Handle_VoiceTalker& InVoiceTalker,
        const TSharedPtr<ICk_VoiceChat_CaptureSource>& InCaptureSource) -> void;

    static auto
    Debug_Get_LoopbackDecodedPcm(
        const FCk_Handle_VoiceTalker& InVoiceTalker) -> const TArray<uint8>&;

    static auto
    Debug_Reset_LoopbackDecodedPcm(
        FCk_Handle_VoiceTalker& InVoiceTalker) -> void;

    // Routing test seams (C++-only): inject a pre-packed bundle into the server-side routing
    // inbox (bypassing the RPC boundary), and read how many forwarded bundles await the client
    // playback processor.
    static auto
    Debug_InjectInboundBundle(
        FCk_Handle_VoiceTalker& InVoiceTalker,
        const TArray<uint8>& InPackedBundle,
        APlayerState* InSender) -> void;

    static auto
    Debug_Get_ReceiveInboxNum(
        const FCk_Handle_VoiceTalker& InVoiceTalker) -> int32;

    // Lifetime totals at the client RPC boundary (counted before the inbox capacity check) -
    // the S5 drain-budget instrumentation reads the connection's actual delivery rate off these.
    static auto
    Debug_Get_ReceiveArrivedBundles(
        const FCk_Handle_VoiceTalker& InVoiceTalker) -> uint64;

    // P4 playback-config seams: which channel the receive path selected and HybridRadio's
    // near/far render state - net specs assert the selection and mode; the audible render is
    // audition-only.
    static auto
    Debug_Get_PlaybackConfigChannel(
        const FCk_Handle_VoiceTalker& InVoiceTalker) -> FCk_Handle_VoiceChannel;

    static auto
    Debug_Get_HybridRenderNear(
        const FCk_Handle_VoiceTalker& InVoiceTalker) -> TOptional<bool>;

    // P4 prune seam: TOTAL talker entries across the world-scoped authority maps (ServeHistory +
    // ListenerMuteMatrix). Takes any LIVE handle in the world as the anchor - a destroyed
    // talker's own handle is a tombstone and cannot anchor the lookup, and matching map keys
    // against a tombstone would false-pass. In a spec's controlled world where every entry names
    // the destroyed talker, total == 0 after its destroy IS the prune proof.
    static auto
    Debug_Get_WorldMapTalkerEntryCount(
        const FCk_Handle& InAnyWorldHandle) -> int32;

    static auto
    Debug_Get_ReceiveArrivedBytes(
        const FCk_Handle_VoiceTalker& InVoiceTalker) -> uint64;
};

// --------------------------------------------------------------------------------------------------------------------
