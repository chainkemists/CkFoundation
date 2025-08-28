#pragma once

#include "CkAudioTrack_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkAudioTrack_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKAUDIO_API UCk_Utils_AudioTrack_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AudioTrack_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_AudioTrack);

public:
    static FCk_Handle_AudioTrack
    Create(
        FCk_Handle& InParentDirector,
        const FCk_Fragment_AudioTrack_ParamsData& InParams);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AudioTrack",
              DisplayName="[Ck][AudioTrack] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AudioTrack",
        DisplayName="[Ck][AudioTrack] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_AudioTrack
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|AudioTrack",
        DisplayName="[Ck][AudioTrack] Handle -> AudioTrack Handle",
        meta = (CompactNodeTitle = "<AsAudioTrack>", BlueprintAutocast))
    static FCk_Handle_AudioTrack
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid AudioTrack Handle",
        Category = "Ck|Utils|AudioTrack",
        meta = (CompactNodeTitle = "INVALID_AudioTrackHandle", Keywords = "make"))
    static FCk_Handle_AudioTrack
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AudioTrack",
              DisplayName="[Ck][AudioTrack] Get Track Name")
    static FGameplayTag
    Get_TrackName(
        const FCk_Handle_AudioTrack& InTrack);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AudioTrack",
              DisplayName="[Ck][AudioTrack] Get Priority")
    static int32
    Get_Priority(
        const FCk_Handle_AudioTrack& InTrack);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AudioTrack",
              DisplayName="[Ck][AudioTrack] Get State")
    static ECk_AudioTrack_State
    Get_State(
        const FCk_Handle_AudioTrack& InTrack);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AudioTrack",
              DisplayName="[Ck][AudioTrack] Get Is Virtualized")
    static bool
    Get_IsVirtualized(
        const FCk_Handle_AudioTrack& InTrack);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AudioTrack",
              DisplayName="[Ck][AudioTrack] Get Current Volume")
    static float
    Get_CurrentVolume(
        const FCk_Handle_AudioTrack& InTrack);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AudioTrack",
              DisplayName="[Ck][AudioTrack] Get Override Behavior")
    static ECk_AudioTrack_OverrideBehavior
    Get_OverrideBehavior(
        const FCk_Handle_AudioTrack& InTrack);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AudioTrack",
              DisplayName="[Ck][AudioTrack] Get IsSpatial")
    static bool
    Get_IsSpatial(
        const FCk_Handle_AudioTrack& InTrack);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AudioTrack",
              DisplayName="[Ck][AudioTrack] Get Playback Percent")
    static float
    Get_PlaybackPercent(
        const FCk_Handle_AudioTrack& InTrack);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName="[Ck][AudioTrack] Request Play")
    static FCk_Handle_AudioTrack
    Request_Play(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
        FCk_Time InFadeInTime);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName="[Ck][AudioTrack] Request Stop")
    static FCk_Handle_AudioTrack
    Request_Stop(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
        FCk_Time InFadeOutTime);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName="[Ck][AudioTrack] Request Set Volume")
    static FCk_Handle_AudioTrack
    Request_SetVolume(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
        float InTargetVolume,
        FCk_Time InFadeTime);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName = "[Ck][AudioTrack] Bind To OnPlaybackStarted")
    static FCk_Handle_AudioTrack
    BindTo_OnPlaybackStarted(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioTrack_Event& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName = "[Ck][AudioTrack] Bind To OnPlaybackFinished")
    static FCk_Handle_AudioTrack
    BindTo_OnPlaybackFinished(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioTrack_Event& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName = "[Ck][AudioTrack] Bind To OnFadeCompleted")
    static FCk_Handle_AudioTrack
    BindTo_OnFadeCompleted(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioTrack_Fade& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName = "[Ck][AudioTrack] Bind To OnPlayStateChanged")
    static FCk_Handle_AudioTrack
    BindTo_OnPlayStateChanged(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioTrack_PlayStateChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName = "[Ck][AudioTrack] Bind To OnVirtualizationChanged")
    static FCk_Handle_AudioTrack
    BindTo_OnVirtualizationChanged(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioTrack_VirtualizationChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName = "[Ck][AudioTrack] Bind To OnPlaybackPercent")
    static FCk_Handle_AudioTrack
    BindTo_OnPlaybackPercent(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioTrack_PlaybackPercent& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName = "[Ck][AudioTrack] Bind To OnSingleEnvelope")
    static FCk_Handle_AudioTrack
    BindTo_OnSingleEnvelope(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioTrack_SingleEnvelope& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName = "[Ck][AudioTrack] Bind To OnMultiEnvelope")
    static FCk_Handle_AudioTrack
    BindTo_OnMultiEnvelope(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioTrack_MultiEnvelope& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName = "[Ck][AudioTrack] Bind To OnAudioFinished")
    static FCk_Handle_AudioTrack
    BindTo_OnAudioFinished(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioTrack_AudioFinished& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName = "[Ck][AudioTrack] Unbind From OnPlaybackStarted")
    static FCk_Handle_AudioTrack
    UnbindFrom_OnPlaybackStarted(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
        const FCk_Delegate_AudioTrack_Event& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName = "[Ck][AudioTrack] Unbind From OnPlaybackFinished")
    static FCk_Handle_AudioTrack
    UnbindFrom_OnPlaybackFinished(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
        const FCk_Delegate_AudioTrack_Event& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName = "[Ck][AudioTrack] Unbind From OnFadeCompleted")
    static FCk_Handle_AudioTrack
    UnbindFrom_OnFadeCompleted(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
        const FCk_Delegate_AudioTrack_Fade& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName = "[Ck][AudioTrack] Unbind From OnPlayStateChanged")
    static FCk_Handle_AudioTrack
    UnbindFrom_OnPlayStateChanged(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
        const FCk_Delegate_AudioTrack_PlayStateChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName = "[Ck][AudioTrack] Unbind From OnVirtualizationChanged")
    static FCk_Handle_AudioTrack
    UnbindFrom_OnVirtualizationChanged(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
        const FCk_Delegate_AudioTrack_VirtualizationChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName = "[Ck][AudioTrack] Unbind From OnPlaybackPercent")
    static FCk_Handle_AudioTrack
    UnbindFrom_OnPlaybackPercent(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
        const FCk_Delegate_AudioTrack_PlaybackPercent& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName = "[Ck][AudioTrack] Unbind From OnSingleEnvelope")
    static FCk_Handle_AudioTrack
    UnbindFrom_OnSingleEnvelope(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
        const FCk_Delegate_AudioTrack_SingleEnvelope& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName = "[Ck][AudioTrack] Unbind From OnMultiEnvelope")
    static FCk_Handle_AudioTrack
    UnbindFrom_OnMultiEnvelope(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
        const FCk_Delegate_AudioTrack_MultiEnvelope& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName = "[Ck][AudioTrack] Unbind From OnAudioFinished")
    static FCk_Handle_AudioTrack
    UnbindFrom_OnAudioFinished(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
        const FCk_Delegate_AudioTrack_AudioFinished& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName="[Ck][AudioTrack] Enable Debug Draw")
    static FCk_Handle_AudioTrack
    Request_EnableDebugDraw(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack",
              DisplayName="[Ck][AudioTrack] Disable Debug Draw")
    static FCk_Handle_AudioTrack
    Request_DisableDebugDraw(
        UPARAM(ref) FCk_Handle_AudioTrack& InTrack);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AudioTrack",
              DisplayName="[Ck][AudioTrack] Is Debug Draw Enabled")
    static bool
    Get_IsDebugDrawEnabled(
        const FCk_Handle_AudioTrack& InTrack);

private:
    friend class UCk_Utils_AudioDirector_UE;
};

// --------------------------------------------------------------------------------------------------------------------
