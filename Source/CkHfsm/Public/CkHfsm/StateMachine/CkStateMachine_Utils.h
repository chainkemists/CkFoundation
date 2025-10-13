#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkHfsm/StateMachine/CkStateMachine_Fragment.h"
#include "CkHfsm/State/CkState_Fragment_Data.h"

#include "CkStateMachine_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_StateMachine_Setup;
    class FProcessor_StateMachine_Enter;
    class FProcessor_StateMachine_Exit;
    class FProcessor_StateMachine_Transition;
    class FProcessor_StateMachine_Evaluate;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_StateMachine"))
class CKHFSM_API UCk_Utils_StateMachine_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_StateMachine_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_StateMachine);

public:
    friend class ck::FProcessor_StateMachine_Setup;
    friend class ck::FProcessor_StateMachine_Enter;
    friend class ck::FProcessor_StateMachine_Exit;
    friend class ck::FProcessor_StateMachine_Transition;
    friend class ck::FProcessor_StateMachine_Evaluate;

public:
    // Feature Management
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|HFSM|StateMachine",
        DisplayName="[Ck][StateMachine] Add Feature")
    static FCk_Handle_StateMachine
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_StateMachine_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|HFSM|StateMachine",
        DisplayName="[Ck][StateMachine] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|HFSM|StateMachine",
        DisplayName="[Ck][StateMachine] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_StateMachine
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|HFSM|StateMachine",
        DisplayName="[Ck][StateMachine] Handle -> StateMachine Handle",
        meta = (CompactNodeTitle = "<AsStateMachine>", BlueprintAutocast))
    static FCk_Handle_StateMachine
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid StateMachine Handle",
        Category = "Ck|Utils|HFSM|StateMachine",
        meta = (CompactNodeTitle = "INVALID_StateMachineHandle", Keywords = "make"))
    static FCk_Handle_StateMachine
    Get_InvalidHandle() { return {}; }

public:
    // State Management (C++ only)
    static FCk_Handle_State
    AddState(
        FCk_Handle_StateMachine& InStateMachineHandle,
        const FCk_Fragment_State_ParamsData& InStateParams);

    static TArray<FCk_Handle_State>
    GetAllStates(
        const FCk_Handle_StateMachine& InStateMachineHandle);

public:
    // Commands (C++ only)
    static FCk_Handle_StateMachine
    Request_Start(
        FCk_Handle_StateMachine& InHandle);

    static FCk_Handle_StateMachine
    Request_Stop(
        FCk_Handle_StateMachine& InHandle);

    static FCk_Handle_StateMachine
    Request_Transition(
        FCk_Handle_StateMachine& InHandle);

public:
    // Query (C++ only)
    static FCk_Handle
    Get_CurrentState(
        const FCk_Handle_StateMachine& InHandle);

    static FCk_Handle
    Get_PreviousState(
        const FCk_Handle_StateMachine& InHandle);

public:
    // Signals (C++ only)
    static FCk_Handle_StateMachine
    BindTo_OnStart(
        FCk_Handle_StateMachine& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_StateMachine& InDelegate);

    static FCk_Handle_StateMachine
    BindTo_OnStop(
        FCk_Handle_StateMachine& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_StateMachine& InDelegate);

    static FCk_Handle_StateMachine
    BindTo_OnTransition(
        FCk_Handle_StateMachine& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_StateMachine_Transition& InDelegate);

    static FCk_Handle_StateMachine
    UnbindFrom_OnStart(
        FCk_Handle_StateMachine& InHandle,
        const FCk_Delegate_StateMachine& InDelegate);

    static FCk_Handle_StateMachine
    UnbindFrom_OnStop(
        FCk_Handle_StateMachine& InHandle,
        const FCk_Delegate_StateMachine& InDelegate);

    static FCk_Handle_StateMachine
    UnbindFrom_OnTransition(
        FCk_Handle_StateMachine& InHandle,
        const FCk_Delegate_StateMachine_Transition& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------