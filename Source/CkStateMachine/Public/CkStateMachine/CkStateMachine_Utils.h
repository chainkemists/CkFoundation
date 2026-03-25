#pragma once

#include "CkStateMachine_Request_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include <StructUtils/InstancedStruct.h>

#include "CkStateMachine_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_SmState_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_StateMachine"))
class CKSTATEMACHINE_API UCk_Utils_StateMachine_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_StateMachine_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_StateMachine);

public:
    // ================================================================================================================
    // CREATION
    // ================================================================================================================

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Add StateMachine")
    static FCk_Handle_StateMachine
    Add(
        UPARAM(ref) FCk_Handle& InOwner,
        TSubclassOf<UCk_SmState_EntityScript> InInitialStateClass,
        ECk_SmAutoStart InAutoStart = ECk_SmAutoStart::OnSetup);

    // ================================================================================================================
    // CONTROL
    // ================================================================================================================

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Request Start")
    static FCk_Handle_StateMachine
    Request_Start(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Request Stop")
    static FCk_Handle_StateMachine
    Request_Stop(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Request Pause")
    static FCk_Handle_StateMachine
    Request_Pause(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Request Resume")
    static FCk_Handle_StateMachine
    Request_Resume(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Request Transition")
    static FCk_Handle_StateMachine
    Request_Transition(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass);

    // ================================================================================================================
    // GETTERS
    // ================================================================================================================

    UFUNCTION(BlueprintPure,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Get Run Status")
    static ECk_SmRunStatus
    Get_RunStatus(
        const FCk_Handle_StateMachine& InStateMachine);

    UFUNCTION(BlueprintPure,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Get Current State Class")
    static TSubclassOf<UCk_SmState_EntityScript>
    Get_CurrentStateClass(
        const FCk_Handle_StateMachine& InStateMachine);

    UFUNCTION(BlueprintPure,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Get Current State Handle")
    static FCk_Handle_SmState
    Get_CurrentStateHandle(
        const FCk_Handle_StateMachine& InStateMachine);

    UFUNCTION(BlueprintPure,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Is In State")
    static bool
    IsInState(
        const FCk_Handle_StateMachine& InStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InStateClass);

    // ================================================================================================================
    // PAYLOAD (Dynamic Fragment Wrappers)
    // ================================================================================================================

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Add Payload")
    static FCk_Handle
    AddPayload(
        UPARAM(ref) FCk_Handle& InEntity,
        const FInstancedStruct& InPayload);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Get Payload")
    static UPARAM(ref) FInstancedStruct&
    GetPayload(
        const FCk_Handle& InEntity,
        const UScriptStruct* InType);

    // ================================================================================================================
    // SIGNAL BINDING
    // ================================================================================================================

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Bind To OnStateChanged")
    static FCk_Handle_StateMachine
    BindTo_OnStateChanged(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Sm_OnStateChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Unbind From OnStateChanged")
    static FCk_Handle_StateMachine
    UnbindFrom_OnStateChanged(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Sm_OnStateChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Bind To OnStarted")
    static FCk_Handle_StateMachine
    BindTo_OnStarted(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Sm_OnStarted& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Unbind From OnStarted")
    static FCk_Handle_StateMachine
    UnbindFrom_OnStarted(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Sm_OnStarted& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Bind To OnStopped")
    static FCk_Handle_StateMachine
    BindTo_OnStopped(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Sm_OnStopped& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Unbind From OnStopped")
    static FCk_Handle_StateMachine
    UnbindFrom_OnStopped(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Sm_OnStopped& InDelegate);

    // ================================================================================================================
    // CAST
    // ================================================================================================================

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_StateMachine
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Handle -> SM Handle",
        meta = (CompactNodeTitle = "<AsSM>", BlueprintAutocast))
    static FCk_Handle_StateMachine
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid SM Handle",
        Category = "Ck|StateMachine",
        meta = (CompactNodeTitle = "INVALID_SmHandle", Keywords = "make"))
    static FCk_Handle_StateMachine
    Get_InvalidHandle() { return {}; }

    // ================================================================================================================
    // INTERNALS
    // ================================================================================================================

private:
    static auto
    DoAddRequest(
        FCk_Handle_StateMachine& InSm,
        const auto& InRequest) -> FCk_Handle_StateMachine;
};

// --------------------------------------------------------------------------------------------------------------------
