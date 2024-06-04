#pragma once
#include "CkEcsExt/CkEcsExt_Utils.h"
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
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Timer);

private:
    struct RecordOfTimers_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfTimers> {};

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Timer] Add New Timer")
    static FCk_Handle_Timer
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Timer_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Timer] Add Or Replace Timer")
    static FCk_Handle_Timer
    AddOrReplace(
        UPARAM(ref) FCk_Handle& InTimerOwnerEntity,
        const FCk_Fragment_Timer_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="[Ck][Timer] Add Multiple New Timers")
    static TArray<FCk_Handle_Timer>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InHandle,
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

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Timer",
        DisplayName="[Ck][Timer] Has Any Timer")
    static bool
    Has_Any(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Timer",
        DisplayName="[Ck][Timer] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Timer
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Timer",
        DisplayName="[Ck][Timer] Handle -> Timer Handle",
        meta = (CompactNodeTitle = "<AsTimer>", BlueprintAutocast))
    static FCk_Handle_Timer
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="[Ck][Timer] Try Get Timer")
    static FCk_Handle_Timer
    TryGet_Timer(
        const FCk_Handle& InTimerOwnerEntity,
        UPARAM(meta = (Categories = "Timer")) FGameplayTag InTimerName);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="[Ck][Timer] Get Name")
    static FGameplayTag
    Get_Name(
        const FCk_Handle_Timer& InTimerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="[Ck][Timer] Get Current State")
    static ECk_Timer_State
    Get_CurrentState(
        const FCk_Handle_Timer & InTimer);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="[Ck][Timer] Get Count Direction")
    static ECk_Timer_CountDirection
    Get_CountDirection(
        const FCk_Handle_Timer& InTimer);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="[Ck][Timer] Get Behavior")
    static ECk_Timer_Behavior
    Get_Behavior(
        const FCk_Handle_Timer& InTimer);

    UFUNCTION(BlueprintPure,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Timer] Get Current Value")
    static FCk_Chrono
    Get_CurrentTimerValue(
        const FCk_Handle_Timer& InTimer);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="[Ck][Timer] For Each",
              meta=(AutoCreateRefTerm="InDelegate, InOptionalPayload"))
    static TArray<FCk_Handle_Timer>
    ForEach_Timer(
        UPARAM(ref) FCk_Handle& InTimerOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_Timer(
        FCk_Handle& InTimerOwnerEntity,
        const TFunction<void(FCk_Handle_Timer)>& InFunc) -> void;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="[Ck][Timer] Request Reset")
    static FCk_Handle_Timer
    Request_Reset(
        UPARAM(ref) FCk_Handle_Timer& InTimer);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="[Ck][Timer] Request Complete")
    static FCk_Handle_Timer
    Request_Complete(
        UPARAM(ref) FCk_Handle_Timer& InTimer);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="[Ck][Timer] Request Stop")
    static FCk_Handle_Timer
    Request_Stop(
        UPARAM(ref) FCk_Handle_Timer& InTimer);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="[Ck][Timer] Request Pause")
    static FCk_Handle_Timer
    Request_Pause(
        UPARAM(ref) FCk_Handle_Timer& InTimer);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="[Ck][Timer] Request Resume")
    static FCk_Handle_Timer
    Request_Resume(
        UPARAM(ref) FCk_Handle_Timer& InTimer);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Timer] Request Jump")
    static FCk_Handle_Timer
    Request_Jump(
        UPARAM(ref) FCk_Handle_Timer& InTimer,
        FCk_Request_Timer_Jump InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Timer] Request Consume")
    static FCk_Handle_Timer
    Request_Consume(
        UPARAM(ref) FCk_Handle_Timer& InTimer,
        FCk_Request_Timer_Consume InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="[Ck][Timer] Request Change Count Direction")
    static FCk_Handle_Timer
    Request_ChangeCountDirection(
        UPARAM(ref) FCk_Handle_Timer& InTimer,
        ECk_Timer_CountDirection InCountDirection);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName="[Ck][Timer] Request Reverse Direction")
    static FCk_Handle_Timer
    Request_ReverseDirection(
        UPARAM(ref) FCk_Handle_Timer& InTimer);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "[Ck][Timer] Bind To OnReset")
    static FCk_Handle_Timer
    BindTo_OnReset(
        UPARAM(ref) FCk_Handle_Timer& InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "[Ck][Timer] Bind To OnStop")
    static FCk_Handle_Timer
    BindTo_OnStop(
        UPARAM(ref) FCk_Handle_Timer& InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);


    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "[Ck][Timer] Bind To OnPause")
    static FCk_Handle_Timer
    BindTo_OnPause(
        UPARAM(ref) FCk_Handle_Timer& InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "[Ck][Timer] Bind To OnResume")
    static FCk_Handle_Timer
    BindTo_OnResume(
        UPARAM(ref) FCk_Handle_Timer& InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "[Ck][Timer] Bind To OnDone")
    static FCk_Handle_Timer
    BindTo_OnDone(
        UPARAM(ref) FCk_Handle_Timer& InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "[Ck][Timer] Bind To OnDepleted")
    static FCk_Handle_Timer
    BindTo_OnDepleted(
        UPARAM(ref) FCk_Handle_Timer& InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "[Ck][Timer] Bind To OnUpdate")
    static FCk_Handle_Timer
    BindTo_OnUpdate(
        UPARAM(ref) FCk_Handle_Timer& InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "[Ck][Timer] Unbind From OnReset")
    static FCk_Handle_Timer
    UnbindFrom_OnReset(
        UPARAM(ref) FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "[Ck][Timer] Unbind From OnStop")
    static FCk_Handle_Timer
    UnbindFrom_OnStop(
        UPARAM(ref) FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate);


    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "[Ck][Timer] Unbind From OnPause")
    static FCk_Handle_Timer
    UnbindFrom_OnPause(
        UPARAM(ref) FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "[Ck][Timer] Unbind From OnResume")
    static FCk_Handle_Timer
    UnbindFrom_OnResume(
        UPARAM(ref) FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "[Ck][Timer] Unbind From OnDone")
    static FCk_Handle_Timer
    UnbindFrom_OnDone(
        UPARAM(ref) FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "[Ck][Timer] Unbind From OnDepleted")
    static FCk_Handle_Timer
    UnbindFrom_OnDepleted(
        UPARAM(ref) FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "[Ck][Timer] Unbind From OnUpdate")
    static FCk_Handle_Timer
    UnbindFrom_OnUpdate(
        UPARAM(ref) FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------

