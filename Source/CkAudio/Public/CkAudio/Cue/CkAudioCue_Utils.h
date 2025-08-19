#pragma once

#include "CkAudioCue_Fragment_Data.h"
#include "CkAudioCue_EntityScript.h"

#include "CkAudio/AudioDirector/CkAudioDirector_Fragment_Data.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkAudioCue_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKAUDIO_API UCk_Utils_AudioCue_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AudioCue_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_AudioCue);

public:
    // Internal function for AudioCue EntityScript - AudioCue = AudioDirector + AudioCue fragments
    static FCk_Handle_AudioCue
    Add(
        FCk_Handle& InHandle,
        const UCk_AudioCue_EntityScript& InAudioCueScript);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AudioCue",
              DisplayName="[Ck][AudioCue] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AudioCue",
        DisplayName="[Ck][AudioCue] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_AudioCue
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|AudioCue",
        DisplayName="[Ck][AudioCue] Handle -> AudioCue Handle",
        meta = (CompactNodeTitle = "<AsAudioCue>", BlueprintAutocast))
    static FCk_Handle_AudioCue
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid AudioCue Handle",
        Category = "Ck|Utils|AudioCue",
        meta = (CompactNodeTitle = "INVALID_AudioCueHandle", Keywords = "make"))
    static FCk_Handle_AudioCue
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioCue",
              DisplayName="[Ck][AudioCue] Request Play")
    static FCk_Handle_AudioCue
    Request_Play(
        UPARAM(ref) FCk_Handle_AudioCue& InAudioCue,
        TOptional<int32> InOverridePriority,
        FCk_Time InFadeInTime);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioCue",
              DisplayName="[Ck][AudioCue] Request Stop")
    static FCk_Handle_AudioCue
    Request_Stop(
        UPARAM(ref) FCk_Handle_AudioCue& InAudioCue,
        FCk_Time InFadeOutTime);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioCue",
              DisplayName="[Ck][AudioCue] Request Stop All")
    static FCk_Handle_AudioCue
    Request_StopAll(
        UPARAM(ref) FCk_Handle_AudioCue& InAudioCue,
        FCk_Time InFadeOutTime);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioCue",
              DisplayName = "[Ck][AudioCue] Bind To OnTrackStarted")
    static FCk_Handle_AudioCue
    BindTo_OnTrackStarted(
        UPARAM(ref) FCk_Handle_AudioCue& InAudioCue,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioCue_Event& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioCue",
              DisplayName = "[Ck][AudioCue] Bind To OnTrackStopped")
    static FCk_Handle_AudioCue
    BindTo_OnTrackStopped(
        UPARAM(ref) FCk_Handle_AudioCue& InAudioCue,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioCue_Event& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioCue",
              DisplayName = "[Ck][AudioCue] Unbind From OnTrackStarted")
    static FCk_Handle_AudioCue
    UnbindFrom_OnTrackStarted(
        UPARAM(ref) FCk_Handle_AudioCue& InAudioCue,
        const FCk_Delegate_AudioCue_Event& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioCue",
              DisplayName = "[Ck][AudioCue] Unbind From OnTrackStopped")
    static FCk_Handle_AudioCue
    UnbindFrom_OnTrackStopped(
        UPARAM(ref) FCk_Handle_AudioCue& InAudioCue,
        const FCk_Delegate_AudioCue_Event& InDelegate);

private:
    friend class UCk_AudioCue_EntityScript;
};

// --------------------------------------------------------------------------------------------------------------------
