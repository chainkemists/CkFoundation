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
    static UPARAM(DisplayName="Timer Handle") FCk_Handle
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Timer_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="AddOrReplace Timer")
    static UPARAM(DisplayName="Timer Handle") FCk_Handle
    AddOrReplace(
        FCk_Handle InTimerOwnerEntity,
        const FCk_Fragment_Timer_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Add Multiple New Timers")
    static TArray<FCk_Handle>
    AddMultiple(
        FCk_Handle InHandle,
        const FCk_Fragment_MultipleTimer_ParamsData& InParams);


    // Not providing a remove function by design - use UCk_Utils_EntityLifetime_UE::Request_DestroyEntity instead
    // Reason: We have no way of knowing how many other Fragments this Entity may have. We do not want to destroy
    // an Entity that may be more than a Timer. We also don't want to remove Fragments one by one as that is
    // more expensive when the Handle is always a Timer and nothing else.
    //UFUNCTION(BlueprintCallable,
    //          Category = "Ck|Utils|Timer",
    //          DisplayName="Remove Timer")
    //static void
    //Remove(
    //    FCk_Handle InTimerEntity);

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
        FCk_Handle InTimer);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="Get Timer Handle")
    static UPARAM(DisplayName="Timer Handle") FCk_Handle
    Get_Timer(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="Get Timer Name")
    static FGameplayTag
    Get_Name(
        FCk_Handle InTimerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="Get Timer Current State")
    static ECk_Timer_State
    Get_CurrentState(
        FCk_Handle InTimer);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="Get Timer Count Direction")
    static ECk_Timer_CountDirection
    Get_CountDirection(
        FCk_Handle InTimer);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="Get Timer Behavior")
    static ECk_Timer_Behavior
    Get_Behavior(
        FCk_Handle InTimer);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="Get Timer Current Value")
    static FCk_Chrono
    Get_CurrentTimerValue(
        FCk_Handle InTimer);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="For Each Timer",
              meta=(AutoCreateRefTerm="InDelegate"))
    static TArray<FCk_Handle>
    ForEach_Timer(
        FCk_Handle InTimerOwnerEntity,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_Timer(
        FCk_Handle InTimerOwnerEntity,
        const TFunction<void(FCk_Handle)>& InFunc) -> void;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Request Reset Timer")
    static void
    Request_Reset(
        FCk_Handle InTimer);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Request Complete Timer")
    static void
    Request_Complete(
        FCk_Handle InTimer);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Request Stop Timer")
    static void
    Request_Stop(
        FCk_Handle InTimer);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Request Pause Timer")
    static void
    Request_Pause(
        FCk_Handle InTimer);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Request Resume Timer")
    static void
    Request_Resume(
        FCk_Handle InTimer);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Request Jump Timer")
    static void
    Request_Jump(
        FCk_Handle InTimer,
        FCk_Request_Timer_Jump InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Request Consume Timer")
    static void
    Request_Consume(
        FCk_Handle InTimer,
        FCk_Request_Timer_Consume InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Request Change Count Direction")
    static void
    Request_ChangeCountDirection(
        FCk_Handle InTimer,
        ECk_Timer_CountDirection InCountDirection);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="Request Reverse Direction")
    static void
    Request_ReverseDirection(
        FCk_Handle InTimer);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Bind To Timer Reset")
    static void
    BindTo_OnTimerReset(
        FCk_Handle InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Bind To Timer Stop")
    static void
    BindTo_OnTimerStop(
        FCk_Handle InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);


    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Bind To Timer Pause")
    static void
    BindTo_OnTimerPause(
        FCk_Handle InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Bind To Timer Resume")
    static void
    BindTo_OnTimerResume(
        FCk_Handle InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Bind To Timer Done")
    static void
    BindTo_OnTimerDone(
        FCk_Handle InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Bind To Timer Depleted")
    static void
    BindTo_OnTimerDepleted(
        FCk_Handle InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Bind To Timer Update")
    static void
    BindTo_OnTimerUpdate(
        FCk_Handle InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Unbind From Timer Reset")
    static void
    UnbindFrom_OnTimerReset(
        FCk_Handle InTimerEntity,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Unbind From Timer Stop")
    static void
    UnbindFrom_OnTimerStop(
        FCk_Handle InTimerEntity,
        const FCk_Delegate_Timer& InDelegate);


    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Unbind From Timer Pause")
    static void
    UnbindFrom_OnTimerPause(
        FCk_Handle InTimerEntity,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Unbind From Timer Resume")
    static void
    UnbindFrom_OnTimerResume(
        FCk_Handle InTimerEntity,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Unbind From Timer Done")
    static void
    UnbindFrom_OnTimerDone(
        FCk_Handle InTimerEntity,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Unbind From Timer Depleted")
    static void
    UnbindFrom_OnTimerDepleted(
        FCk_Handle InTimerEntity,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "Unbind From Timer Update")
    static void
    UnbindFrom_OnTimerUpdate(
        FCk_Handle InTimerEntity,
        const FCk_Delegate_Timer& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------

