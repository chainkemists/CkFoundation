#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkHFSM/Transition/CkTransition_Fragment.h"

#include "CkTransition_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Transition_Setup;
    class FProcessor_Transition_Enter;
    class FProcessor_Transition_Exit;
    class FProcessor_Transition_Evaluate;
}

// --------------------------------------------------------------------------------------------------------------------

// NOT Blueprint-exposed - internal use only
UCLASS(NotBlueprintable)
class CKHFSM_API UCk_Utils_Transition_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Transition_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Transition);

public:
    friend class ck::FProcessor_Transition_Setup;
    friend class ck::FProcessor_Transition_Enter;
    friend class ck::FProcessor_Transition_Exit;
    friend class ck::FProcessor_Transition_Evaluate;

public:
    // Feature Management (C++ only)
    static FCk_Handle_Transition
    Create(
        FCk_Handle_State& InStateHandle,
        const FCk_Fragment_Transition_ParamsData& InParams);

    static bool
    Has(
        const FCk_Handle& InHandle);

    static FCk_Handle_Transition
    Cast(
        FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    static FCk_Handle_Transition
    CastChecked(
        FCk_Handle InHandle);

public:
    // Commands (C++ only)
    static FCk_Handle_Transition
    Request_StartEvaluating(
        FCk_Handle_Transition& InHandle);

    static FCk_Handle_Transition
    Request_StopEvaluating(
        FCk_Handle_Transition& InHandle);

public:
    // Query (C++ only)
    static FGameplayTag
    Get_Name(
        const FCk_Handle_Transition& InHandle);

    static ECk_Transition_Result
    Get_EvaluationResult(
        const FCk_Handle_Transition& InHandle);

    static FCk_Handle
    Get_TargetState(
        const FCk_Handle_Transition& InHandle);

    static FCk_Handle
    Get_TransitionCondition(
        const FCk_Handle_Transition& InHandle);

public:
    // Signals (C++ only)
    static FCk_Handle_Transition
    BindTo_OnEnter(
        FCk_Handle_Transition& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Transition& InDelegate);

    static FCk_Handle_Transition
    BindTo_OnExit(
        FCk_Handle_Transition& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Transition& InDelegate);

    static FCk_Handle_Transition
    BindTo_OnPassed(
        FCk_Handle_Transition& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Transition& InDelegate);

    static FCk_Handle_Transition
    BindTo_OnFailed(
        FCk_Handle_Transition& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Transition& InDelegate);

    static FCk_Handle_Transition
    UnbindFrom_OnEnter(
        FCk_Handle_Transition& InHandle,
        const FCk_Delegate_Transition& InDelegate);

    static FCk_Handle_Transition
    UnbindFrom_OnExit(
        FCk_Handle_Transition& InHandle,
        const FCk_Delegate_Transition& InDelegate);

    static FCk_Handle_Transition
    UnbindFrom_OnPassed(
        FCk_Handle_Transition& InHandle,
        const FCk_Delegate_Transition& InDelegate);

    static FCk_Handle_Transition
    UnbindFrom_OnFailed(
        FCk_Handle_Transition& InHandle,
        const FCk_Delegate_Transition& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------