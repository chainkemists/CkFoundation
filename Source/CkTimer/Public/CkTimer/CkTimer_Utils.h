#pragma once
#include "CkEcsBasics/CkEcsBasics_Utils.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkNet/CkNet_Utils.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkSignal/CkSignal_Fragment_Data.h"
#include "CkTimer/CkTimer_Fragment.h"

#include "CkTimer/CkTimer_Fragment_Data.h"

#include "CkTimer_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKTIMER_API UCk_Utils_Timer_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Timer_UE);

private:
    struct RecordOfTimers_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfTimers> {};

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Add New Timer")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Timer_ParamsData& InParams,
        ECk_Net_ReplicationType InReplicationType = ECk_Net_ReplicationType::All);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Replace Timer")
    static void
    Replace(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        const FCk_Fragment_Timer_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Add Multiple New Timers")
    static void
    AddMultiple(
        FCk_Handle InHandle,
        const FCk_Fragment_MultipleTimer_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="Has Timer")
    static bool
    Has(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="Has Any Timer")
    static bool
    Has_Any(
        FCk_Handle InTimerOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="Ensure Has Timer")
    static bool
    Ensure(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="Ensure Has Any Timer")
    static bool
    Ensure_Any(
        FCk_Handle InTimerOwnerEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="Get All Timers")
    static TArray<FGameplayTag>
    Get_All(
        FCk_Handle InTimerOwnerEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="Get Timer Current State")
    static ECk_Timer_State
    Get_CurrentState(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="Get Timer Count Direction")
    static ECk_Timer_CountDirection
    Get_CountDirection(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="Get Timer Behavior")
    static ECk_Timer_Behavior
    Get_Behavior(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="Get Timer Current Value")
    static FCk_Chrono
    Get_CurrentTimerValue(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Request Reset Timer")
    static void
    Request_Reset(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Request Stop Timer")
    static void
    Request_Stop(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Request Pause Timer")
    static void
    Request_Pause(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Request Resume Timer")
    static void
    Request_Resume(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Request Jump Timer")
    static void
    Request_Jump(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        FCk_Request_Timer_Jump InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Request Consume Timer")
    static void
    Request_Consume(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        FCk_Request_Timer_Consume InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Request Change Count Direction")
    static void
    Request_ChangeCountDirection(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        ECk_Timer_CountDirection InCountDirection);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Request Reverse Direction")
    static void
    Request_ReverseDirection(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Bind To Timer Reset")
    static void
    BindTo_OnTimerReset(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Bind To Timer Stop")
    static void
    BindTo_OnTimerStop(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);


    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Bind To Timer Pause")
    static void
    BindTo_OnTimerPause(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Bind To Timer Resume")
    static void
    BindTo_OnTimerResume(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Bind To Timer Done")
    static void
    BindTo_OnTimerDone(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Bind To Timer Update")
    static void
    BindTo_OnTimerUpdate(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Unbind From Timer Reset")
    static void
    UnbindFrom_OnTimerReset(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Unbind From Timer Stop")
    static void
    UnbindFrom_OnTimerStop(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        const FCk_Delegate_Timer& InDelegate);


    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Unbind From Timer Pause")
    static void
    UnbindFrom_OnTimerPause(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Unbind From Timer Resume")
    static void
    UnbindFrom_OnTimerResume(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Unbind From Timer Done")
    static void
    UnbindFrom_OnTimerDone(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Unbind From Timer Update")
    static void
    UnbindFrom_OnTimerUpdate(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        const FCk_Delegate_Timer& InDelegate);

public:
    static auto
    Request_OverrideTimer(
        FCk_Handle InHandle,
        const FCk_Chrono& InNewTimer) -> void;

private:
    static auto
    Has(
        FCk_Handle InHandle) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

