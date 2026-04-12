#include "CkSmCondition_Utils.h"

#include "CkStateMachine/Transition/CkSmTransition_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmCondition_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return ck::IsValid(InHandle) && InHandle.Has<ck::FFragment_SmCondition_Current>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmCondition_UE::
    Request_StartOrResumeEvaluating(
        FCk_Handle_SmCondition& InCondition)
    -> FCk_Handle_SmCondition
{
    CK_ENSURE_IF_NOT(ck::IsValid(InCondition),
        TEXT("Invalid condition handle in Request_StartOrResumeEvaluating"))
    { return InCondition; }

    auto CondHandle = static_cast<FCk_Handle>(InCondition);
    CondHandle.Try_Remove<ck::FTag_SmCondition_EvaluationPaused>();

    return InCondition;
}

auto
    UCk_Utils_SmCondition_UE::
    Request_PauseEvaluation(
        FCk_Handle_SmCondition& InCondition)
    -> FCk_Handle_SmCondition
{
    CK_ENSURE_IF_NOT(ck::IsValid(InCondition),
        TEXT("Invalid condition handle in Request_PauseEvaluation"))
    { return InCondition; }

    auto CondHandle = static_cast<FCk_Handle>(InCondition);
    CondHandle.AddOrGet<ck::FTag_SmCondition_EvaluationPaused>();

    return InCondition;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmCondition_UE::
    MarkConditionAs_Satisfied(
        FCk_Handle_SmCondition& InCondition)
    -> FCk_Handle_SmCondition
{
    CK_ENSURE_IF_NOT(ck::IsValid(InCondition),
        TEXT("Invalid condition handle in MarkConditionAs_Satisfied"))
    { return InCondition; }

    auto CondHandle = static_cast<FCk_Handle>(InCondition);

    CK_ENSURE_IF_NOT(CondHandle.Has<ck::FFragment_SmCondition_Current>(),
        TEXT("Condition entity [{}] is missing FFragment_SmCondition_Current in MarkConditionAs_Satisfied"), InCondition)
    { return InCondition; }

    CondHandle.Get<ck::FFragment_SmCondition_Current>().Set_Result(ECk_SmConditionResult::Pass);
    CondHandle.Try_Remove<ck::FTag_SmCondition_EvaluationPaused>();

    // Wake the parent transition so FProcessor_SmTransition_EvaluateFromConditions
    // aggregates the updated condition result into the transition result this pump.
    if (ck::TUtils_Sm_ParentTransition::Has(CondHandle))
    {
        auto ParentTransitionHandle = static_cast<FCk_Handle>(
            ck::TUtils_Sm_ParentTransition::Get_StoredEntity(CondHandle));
        ParentTransitionHandle.AddOrGet<ck::FTag_SmTransition_Evaluating>();
        ParentTransitionHandle.AddOrGet<ck::FFragment_SmTransition_Current>();
    }

    return InCondition;
}

auto
    UCk_Utils_SmCondition_UE::
    MarkConditionAs_Unsatisfied(
        FCk_Handle_SmCondition& InCondition)
    -> FCk_Handle_SmCondition
{
    CK_ENSURE_IF_NOT(ck::IsValid(InCondition),
        TEXT("Invalid condition handle in MarkConditionAs_Unsatisfied"))
    { return InCondition; }

    auto CondHandle = static_cast<FCk_Handle>(InCondition);

    CK_ENSURE_IF_NOT(CondHandle.Has<ck::FFragment_SmCondition_Current>(),
        TEXT("Condition entity [{}] is missing FFragment_SmCondition_Current in MarkConditionAs_Unsatisfied"), InCondition)
    { return InCondition; }

    CondHandle.Get<ck::FFragment_SmCondition_Current>().Set_Result(ECk_SmConditionResult::Fail);

    // Wake the parent transition so FProcessor_SmTransition_EvaluateFromConditions
    // aggregates the Fail result and resolves the transition this pump.
    if (ck::TUtils_Sm_ParentTransition::Has(CondHandle))
    {
        auto ParentTransitionHandle = static_cast<FCk_Handle>(
            ck::TUtils_Sm_ParentTransition::Get_StoredEntity(CondHandle));
        ParentTransitionHandle.AddOrGet<ck::FTag_SmTransition_Evaluating>();
        ParentTransitionHandle.AddOrGet<ck::FFragment_SmTransition_Current>();
    }

    return InCondition;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmCondition_UE::
    Get_EvaluationResult(
        const FCk_Handle_SmCondition& InCondition)
    -> ECk_SmConditionResult
{
    if (ck::Is_NOT_Valid(InCondition))
    { return ECk_SmConditionResult::Undetermined; }

    auto CondHandle = static_cast<FCk_Handle>(InCondition);

    if (NOT CondHandle.Has<ck::FFragment_SmCondition_Current>())
    { return ECk_SmConditionResult::Undetermined; }

    return CondHandle.Get<ck::FFragment_SmCondition_Current>().Get_Result();
}

auto
    UCk_Utils_SmCondition_UE::
    Get_IsEventDriven(
        const FCk_Handle_SmCondition& InCondition)
    -> bool
{
    if (ck::Is_NOT_Valid(InCondition))
    { return false; }

    auto CondHandle = static_cast<FCk_Handle>(InCondition);
    return CondHandle.Has<ck::FTag_SmCondition_EventDriven>();
}

// --------------------------------------------------------------------------------------------------------------------
