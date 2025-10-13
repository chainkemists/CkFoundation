#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkHFSM/Condition/CkCondition_Fragment.h"

#include "CkCondition_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Condition_Setup;
    class FProcessor_Condition_Enter;
    class FProcessor_Condition_Exit;
    class FProcessor_Condition_Evaluate;
}

// --------------------------------------------------------------------------------------------------------------------

// NOT Blueprint-exposed - internal use only
UCLASS(NotBlueprintable)
class CKHFSM_API UCk_Utils_Condition_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Condition_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Condition);

public:
    friend class ck::FProcessor_Condition_Setup;
    friend class ck::FProcessor_Condition_Enter;
    friend class ck::FProcessor_Condition_Exit;
    friend class ck::FProcessor_Condition_Evaluate;

public:
    // Feature Management (C++ only)
    static FCk_Handle_Condition
    Create(
        FCk_Handle_Transition& InTransitionHandle,
        const FCk_Fragment_Condition_ParamsData& InParams);

    static bool
    Has(
        const FCk_Handle& InHandle);

    static FCk_Handle_Condition
    Cast(
        FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    static FCk_Handle_Condition
    CastChecked(
        FCk_Handle InHandle);

public:
    // Commands (C++ only)
    static FCk_Handle_Condition
    Request_StartOrResumeEvaluating(
        FCk_Handle_Condition& InHandle);

    static FCk_Handle_Condition
    Request_PauseEvaluation(
        FCk_Handle_Condition& InHandle);

    static FCk_Handle_Condition
    Request_StopEvaluating(
        FCk_Handle_Condition& InHandle);

    static FCk_Handle_Condition
    Request_MarkResult(
        FCk_Handle_Condition& InHandle,
        ECk_Condition_MarkResult InResult);

public:
    // Query (C++ only)
    static ECk_Condition_Result
    Get_EvaluationResult(
        const FCk_Handle_Condition& InHandle);

    static bool
    Get_IsResultNegated(
        const FCk_Handle_Condition& InHandle);

    static bool
    Get_IsEventDriven(
        const FCk_Handle_Condition& InHandle);

    static bool
    Get_IsNotEventDriven(
        const FCk_Handle_Condition& InHandle);

public:
    // Signals (C++ only)
    static FCk_Handle_Condition
    BindTo_OnEnter(
        FCk_Handle_Condition& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Condition& InDelegate);

    static FCk_Handle_Condition
    BindTo_OnExit(
        FCk_Handle_Condition& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Condition& InDelegate);

    static FCk_Handle_Condition
    BindTo_OnPassed(
        FCk_Handle_Condition& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Condition& InDelegate);

    static FCk_Handle_Condition
    BindTo_OnFailed(
        FCk_Handle_Condition& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Condition& InDelegate);

    static FCk_Handle_Condition
    UnbindFrom_OnEnter(
        FCk_Handle_Condition& InHandle,
        const FCk_Delegate_Condition& InDelegate);

    static FCk_Handle_Condition
    UnbindFrom_OnExit(
        FCk_Handle_Condition& InHandle,
        const FCk_Delegate_Condition& InDelegate);

    static FCk_Handle_Condition
    UnbindFrom_OnPassed(
        FCk_Handle_Condition& InHandle,
        const FCk_Delegate_Condition& InDelegate);

    static FCk_Handle_Condition
    UnbindFrom_OnFailed(
        FCk_Handle_Condition& InHandle,
        const FCk_Delegate_Condition& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------