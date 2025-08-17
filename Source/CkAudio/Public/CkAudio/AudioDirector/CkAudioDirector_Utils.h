#pragma once

#include "CkAudioDirector_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkAudioDirector_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKAUDIO_API UCk_Utils_AudioDirector_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AudioDirector_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_AudioDirector);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioDirector",
              DisplayName="[Ck][AudioDirector] Add Feature")
    static FCk_Handle_AudioDirector
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_AudioDirector_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AudioDirector",
              DisplayName="[Ck][AudioDirector] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AudioDirector",
        DisplayName="[Ck][AudioDirector] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_AudioDirector
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|AudioDirector",
        DisplayName="[Ck][AudioDirector] Handle -> AudioDirector Handle",
        meta = (CompactNodeTitle = "<AsAudioDirector>", BlueprintAutocast))
    static FCk_Handle_AudioDirector
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid AudioDirector Handle",
        Category = "Ck|Utils|AudioDirector",
        meta = (CompactNodeTitle = "INVALID_AudioDirectorHandle", Keywords = "make"))
    static FCk_Handle_AudioDirector
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AudioDirector",
              DisplayName="[Ck][AudioDirector] Get Current Highest Priority")
    static int32
    Get_CurrentHighestPriority(
        const FCk_Handle_AudioDirector& InDirector);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AudioDirector",
              DisplayName="[Ck][AudioDirector] Get Track By Name")
    static FCk_Handle_AudioTrack
    Get_TrackByName(
        const FCk_Handle_AudioDirector& InDirector,
        UPARAM(meta = (Categories = "Audio.Track")) FGameplayTag InTrackName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AudioDirector",
              DisplayName="[Ck][AudioDirector] Get All Tracks")
    static TArray<FCk_Handle_AudioTrack>
    Get_AllTracks(
        const FCk_Handle_AudioDirector& InDirector);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioDirector",
              DisplayName="[Ck][AudioDirector] Request Add Track")
    static FCk_Handle_AudioDirector
    Request_AddTrack(
        UPARAM(ref) FCk_Handle_AudioDirector& InDirector,
        const FCk_Fragment_AudioTrack_ParamsData& InTrackParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioDirector",
              DisplayName="[Ck][AudioDirector] Request Start Track")
    static FCk_Handle_AudioDirector
    Request_StartTrack(
        UPARAM(ref) FCk_Handle_AudioDirector& InDirector,
        UPARAM(meta = (Categories = "Audio.Track")) FGameplayTag InTrackName,
        TOptional<int32> InOverridePriority,
        FCk_Time InFadeInTime);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioDirector",
              DisplayName="[Ck][AudioDirector] Request Stop Track")
    static FCk_Handle_AudioDirector
    Request_StopTrack(
        UPARAM(ref) FCk_Handle_AudioDirector& InDirector,
        UPARAM(meta = (Categories = "Audio.Track")) FGameplayTag InTrackName,
        FCk_Time InFadeOutTime);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioDirector",
              DisplayName="[Ck][AudioDirector] Request Stop All Tracks")
    static FCk_Handle_AudioDirector
    Request_StopAllTracks(
        UPARAM(ref) FCk_Handle_AudioDirector& InDirector,
        FCk_Time InFadeOutTime);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioDirector",
              DisplayName = "[Ck][AudioDirector] Bind To OnTrackStarted")
    static FCk_Handle_AudioDirector
    BindTo_OnTrackStarted(
        UPARAM(ref) FCk_Handle_AudioDirector& InDirector,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioDirector_Track& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioDirector",
              DisplayName = "[Ck][AudioDirector] Bind To OnTrackStopped")
    static FCk_Handle_AudioDirector
    BindTo_OnTrackStopped(
        UPARAM(ref) FCk_Handle_AudioDirector& InDirector,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioDirector_Track& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioDirector",
              DisplayName = "[Ck][AudioDirector] Bind To OnTrackAdded")
    static FCk_Handle_AudioDirector
    BindTo_OnTrackAdded(
        UPARAM(ref) FCk_Handle_AudioDirector& InDirector,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioDirector_Track& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioDirector",
              DisplayName = "[Ck][AudioDirector] Unbind From OnTrackStarted")
    static FCk_Handle_AudioDirector
    UnbindFrom_OnTrackStarted(
        UPARAM(ref) FCk_Handle_AudioDirector& InDirector,
        const FCk_Delegate_AudioDirector_Track& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioDirector",
              DisplayName = "[Ck][AudioDirector] Unbind From OnTrackStopped")
    static FCk_Handle_AudioDirector
    UnbindFrom_OnTrackStopped(
        UPARAM(ref) FCk_Handle_AudioDirector& InDirector,
        const FCk_Delegate_AudioDirector_Track& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioDirector",
              DisplayName = "[Ck][AudioDirector] Unbind From OnTrackAdded")
    static FCk_Handle_AudioDirector
    UnbindFrom_OnTrackAdded(
        UPARAM(ref) FCk_Handle_AudioDirector& InDirector,
        const FCk_Delegate_AudioDirector_Track& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
