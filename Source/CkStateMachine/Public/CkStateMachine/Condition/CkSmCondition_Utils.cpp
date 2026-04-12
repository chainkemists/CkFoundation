#include "CkSmCondition_Utils.h"

#include "CkStateMachine/Condition/EntityScripts/CkSmCondition_EntityScript.h"
#include "CkStateMachine/Transition/CkSmTransition_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

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
    Create(
        FCk_Handle_SmTransition& InOwnerTransition,
        TSubclassOf<UCk_SmCondition_EntityScript> InConditionClass)
    -> FCk_Handle_SmCondition
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwnerTransition),
        TEXT("Invalid transition handle in SmCondition Create"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InConditionClass),
        TEXT("Invalid condition class in SmCondition Create"))
    { return {}; }

    auto ConditionEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwnerTransition);

    UCk_Utils_Handle_UE::Set_DebugName(ConditionEntity, InConditionClass->GetFName());

    if (InOwnerTransition.Has<ck::FFragment_Sm_Context>())
    {
        const auto& Context = InOwnerTransition.Get<ck::FFragment_Sm_Context>();
        ConditionEntity.Add<ck::FFragment_Sm_Context>(Context.Get_GameEntityHandle());
    }

    const auto* ConditionCDO = GetDefault<UCk_SmCondition_EntityScript>(InConditionClass);

    auto ConditionCurrent = ck::FFragment_SmCondition_Current{};
    if (ck::IsValid(ConditionCDO))
    {
        ConditionCurrent.Set_ResetBehavior(ConditionCDO->Get_ResetBehavior());

        if (ConditionCDO->Get_ConditionMode() == ECk_SmConditionMode::Polled)
        {
            ConditionEntity.Add<ck::FTag_SmCondition_Polled>();
        }
        else
        {
            ConditionEntity.Add<ck::FTag_SmCondition_EventDriven>();
        }

        if (ConditionCDO->Get_ResetBehavior() == ECk_SmConditionResetBehavior::ResetEveryFrame)
        {
            ConditionEntity.Add<ck::FTag_SmCondition_ResetsEveryFrame>();
        }
    }

    ConditionEntity.Add<ck::FFragment_SmCondition_Current>(ConditionCurrent);

    auto ConditionEntityTyped = CastChecked(ConditionEntity);

    UCk_Utils_StateMachine_UE::RecordOfSmConditions_Utils::AddIfMissing(InOwnerTransition);
    UCk_Utils_StateMachine_UE::RecordOfSmConditions_Utils::Request_Connect(
        InOwnerTransition, ConditionEntityTyped, ECk_Record_LabelRequirementPolicy::Optional);

    ck::TUtils_Sm_ParentTransition::AddOrReplace(ConditionEntity, InOwnerTransition);

    if (ck::TUtils_Sm_OwningStateMachine::Has(InOwnerTransition))
    {
        const auto OwningSm = ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(InOwnerTransition);
        ck::TUtils_Sm_OwningStateMachine::AddOrReplace(ConditionEntity, OwningSm);
    }

    UCk_Utils_EntityScript_UE::Add(ConditionEntity, InConditionClass, FInstancedStruct{});

    return ConditionEntityTyped;
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

    InCondition.Try_Remove<ck::FTag_SmCondition_EvaluationPaused>();

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

    InCondition.AddOrGet<ck::FTag_SmCondition_EvaluationPaused>();

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

    CK_ENSURE_IF_NOT(InCondition.Has<ck::FFragment_SmCondition_Current>(),
        TEXT("Condition entity [{}] is missing FFragment_SmCondition_Current in MarkConditionAs_Satisfied"), InCondition)
    { return InCondition; }

    InCondition.Get<ck::FFragment_SmCondition_Current>().Set_Result(ECk_SmConditionResult::Pass);
    InCondition.Try_Remove<ck::FTag_SmCondition_EvaluationPaused>();

    // Wake the parent transition so FProcessor_SmTransition_EvaluateFromConditions
    // aggregates the updated condition result into the transition result this pump.
    if (ck::TUtils_Sm_ParentTransition::Has(InCondition))
    {
        auto ParentTransitionHandle = ck::TUtils_Sm_ParentTransition::Get_StoredEntity(InCondition);
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

    CK_ENSURE_IF_NOT(InCondition.Has<ck::FFragment_SmCondition_Current>(),
        TEXT("Condition entity [{}] is missing FFragment_SmCondition_Current in MarkConditionAs_Unsatisfied"), InCondition)
    { return InCondition; }

    InCondition.Get<ck::FFragment_SmCondition_Current>().Set_Result(ECk_SmConditionResult::Fail);

    // Wake the parent transition so FProcessor_SmTransition_EvaluateFromConditions
    // aggregates the Fail result and resolves the transition this pump.
    if (ck::TUtils_Sm_ParentTransition::Has(InCondition))
    {
        auto ParentTransitionHandle = ck::TUtils_Sm_ParentTransition::Get_StoredEntity(InCondition);
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

    if (NOT InCondition.Has<ck::FFragment_SmCondition_Current>())
    { return ECk_SmConditionResult::Undetermined; }

    return InCondition.Get<ck::FFragment_SmCondition_Current>().Get_Result();
}

auto
    UCk_Utils_SmCondition_UE::
    Get_IsEventDriven(
        const FCk_Handle_SmCondition& InCondition)
    -> bool
{
    if (ck::Is_NOT_Valid(InCondition))
    { return false; }

    return InCondition.Has<ck::FTag_SmCondition_EventDriven>();
}

// --------------------------------------------------------------------------------------------------------------------
