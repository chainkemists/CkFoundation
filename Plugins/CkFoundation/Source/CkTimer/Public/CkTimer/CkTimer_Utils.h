#pragma once
#include "CkMacros/CkMacros.h"

#include "CkNet/CkNet_Utils.h"

#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkTimer/CkTimer_Fragment_Data.h"

#include "CkTimer_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKTIMER_API UCk_Utils_Timer_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Timer_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Add Timer")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Timer_ParamsData& InData);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="Has Timer")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="Ensure Has Timer")
    static bool
    Ensure(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer")
    static void
    Request_Reset(
        FCk_Handle InHandle,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer")
    static void
    Request_Stop(
        FCk_Handle InHandle,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer")
    static void
    Request_Pause(
        FCk_Handle InHandle,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer")
    static void
    Request_Resume(
        FCk_Handle InHandle,
        const FCk_Delegate_Timer& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName = "Bind To Marker Enable Disable")
    static void
    BindTo_OnTimerReset(
        FCk_Handle InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName = "Bind To Marker Enable Disable")
    static void
    BindTo_OnTimerStop(
        FCk_Handle InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);


    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName = "Bind To Marker Enable Disable")
    static void
    BindTo_OnTimerPause(
        FCk_Handle InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName = "Bind To Marker Enable Disable")
    static void
    BindTo_OnTimerResume(
        FCk_Handle InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName = "Bind To Marker Enable Disable")
    static void
    BindTo_OnTimerDone(
        FCk_Handle InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName = "Bind To Marker Enable Disable")
    static void
    BindTo_OnTimerUpdate(
        FCk_Handle InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName = "Bind To Marker Enable Disable")
    static void
    UnbindTo_OnTimerReset(
        FCk_Handle InHandle,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName = "Bind To Marker Enable Disable")
    static void
    UnbindTo_OnTimerStop(
        FCk_Handle InHandle,
        const FCk_Delegate_Timer& InDelegate);


    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName = "Bind To Marker Enable Disable")
    static void
    UnbindTo_OnTimerPause(
        FCk_Handle InHandle,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName = "Bind To Marker Enable Disable")
    static void
    UnbindTo_OnTimerResume(
        FCk_Handle InHandle,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName = "Bind To Marker Enable Disable")
    static void
    UnbindTo_OnTimerDone(
        FCk_Handle InHandle,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName = "Bind To Marker Enable Disable")
    static void
    UnbindTo_OnTimerUpdate(
        FCk_Handle InHandle,
        const FCk_Delegate_Timer& InDelegate);
};
