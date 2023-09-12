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
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Request Reset Timer")
    static void
    Request_Reset(
        FCk_Handle InHandle,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Request Stop Timer")
    static void
    Request_Stop(
        FCk_Handle InHandle,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Request Pause Timer")
    static void
    Request_Pause(
        FCk_Handle InHandle,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Request Resume Timer")
    static void
    Request_Resume(
        FCk_Handle InHandle,
        const FCk_Delegate_Timer& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Bind To Timer Reset")
    static void
    BindTo_OnTimerReset(
        FCk_Handle InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Bind To Timer Stop")
    static void
    BindTo_OnTimerStop(
        FCk_Handle InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);


    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Bind To Timer Pause")
    static void
    BindTo_OnTimerPause(
        FCk_Handle InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Bind To Timer Resume")
    static void
    BindTo_OnTimerResume(
        FCk_Handle InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Bind To Timer Done")
    static void
    BindTo_OnTimerDone(
        FCk_Handle InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Bind To Timer Update")
    static void
    BindTo_OnTimerUpdate(
        FCk_Handle InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Unbind From Timer Reset")
    static void
    UnbindFrom_OnTimerReset(
        FCk_Handle InHandle,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Unbind From Timer Stop")
    static void
    UnbindFrom_OnTimerStop(
        FCk_Handle InHandle,
        const FCk_Delegate_Timer& InDelegate);


    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Unbind From Timer Pause")
    static void
    UnbindFrom_OnTimerPause(
        FCk_Handle InHandle,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Unbind From Timer Resume")
    static void
    UnbindFrom_OnTimerResume(
        FCk_Handle InHandle,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Unbind From Timer Done")
    static void
    UnbindFrom_OnTimerDone(
        FCk_Handle InHandle,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Unbind From Timer Update")
    static void
    UnbindFrom_OnTimerUpdate(
        FCk_Handle InHandle,
        const FCk_Delegate_Timer& InDelegate);

public:
    static auto
    Request_OverrideTimer(
        FCk_Handle InHandle,
        const FCk_Chrono& InNewTimer) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

