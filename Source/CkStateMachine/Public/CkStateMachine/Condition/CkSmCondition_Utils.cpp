#include "CkSmCondition_Utils.h"

#include "CkStateMachine/Condition/EntityScripts/CkSmCondition_EntityScript.h"
#include "CkStateMachine/Condition/EntityScripts/CkSmCondition_Polled.h"
#include "CkStateMachine/Transition/CkSmTransition_Fragment.h"
#include "CkStateMachine/Transition/CkSmTransition_Utils.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkStateMachine/Condition/EntityScripts/CkSmCondition_EventDriven.h"

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

    if (InConditionClass->IsChildOf(UCk_SmCondition_Polled::StaticClass()))
    {
        ConditionEntity.Add<ck::FTag_SmCondition_Polled>();

        // Auto-detect: a polled condition cascades NotFullyEventDriven up through
        // the parent transition to the parent state.
        UCk_Utils_SmTransition_UE::Request_MarkTransition_AsNotFullyEventDriven(InOwnerTransition);
    }
    else if (InConditionClass->IsChildOf(UCk_SmCondition_EventDriven::StaticClass()))
    {
        ConditionEntity.Add<ck::FTag_SmCondition_EventDriven>();
    }
    else
    {
        CK_TRIGGER_ENSURE(TEXT("Attempting to create an HFSM Condition with class [{}] that is neither Polled or EventDriven"), InConditionClass);
    }

    const auto* ConditionCDO = GetDefault<UCk_SmCondition_EntityScript>(InConditionClass);

    auto ConditionCurrent = ck::FFragment_SmCondition_Current{};
    if (ck::IsValid(ConditionCDO))
    {
        ConditionCurrent.Set_ResetBehavior(ConditionCDO->Get_ResetBehavior());

        if (ConditionCDO->Get_ResetBehavior() == ECk_SmConditionResetBehavior::ResetEveryFrame)
        {
            ConditionEntity.Add<ck::FTag_SmCondition_ResetsEveryFrame>();
        }
    }

    ConditionEntity.Add<ck::FFragment_SmCondition_Current>(ConditionCurrent);
    ConditionEntity.Add<ck::FTag_SmCondition_EvaluationPaused>();

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
    InCondition.Try_Remove<ck::FTag_SmCondition_EvaluationPaused>();

    return InCondition;
}

auto
    UCk_Utils_SmCondition_UE::
    Request_PauseEvaluation(
        FCk_Handle_SmCondition& InCondition)
    -> FCk_Handle_SmCondition
{
    InCondition.AddOrGet<ck::FTag_SmCondition_EvaluationPaused>();

    return InCondition;
}

auto
    UCk_Utils_SmCondition_UE::
    Request_ResetPolledCondition(
        FCk_Handle_SmCondition& InCondition)
    -> FCk_Handle_SmCondition
{
    InCondition.Get<ck::FFragment_SmCondition_Current>().Set_Result(ECk_SmConditionResult::Undetermined);
    InCondition.Try_Remove<ck::FTag_SmCondition_EvaluationPaused>();

    return InCondition;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmCondition_UE::
    Request_UpdateConditionResult(
        FCk_Handle_SmCondition& InCondition,
        ECk_SmConditionResult InResult)
    -> FCk_Handle_SmCondition
{
    InCondition.Get<ck::FFragment_SmCondition_Current>().Set_Result(InResult);

    // Wake the parent transition so the transition processor re-evaluates this pump.
    // Remove + Add forces a dirty version increment (AddOrGet is a noop when tag exists).
    auto ParentTransitionHandle = ck::TUtils_Sm_ParentTransition::Get_StoredEntity(InCondition);

    ParentTransitionHandle.Try_Remove<ck::FTag_SmTransition_Evaluating>();
    ParentTransitionHandle.AddOrGet<ck::FTag_SmTransition_Evaluating>();

    return InCondition;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmCondition_UE::
    Get_EvaluationResult(
        const FCk_Handle_SmCondition& InCondition)
    -> ECk_SmConditionResult
{
    return InCondition.Get<ck::FFragment_SmCondition_Current>().Get_Result();
}

auto
    UCk_Utils_SmCondition_UE::
    Get_ConditionMode(
        const FCk_Handle_SmCondition& InCondition)
    -> ECk_SmConditionMode
{
    if (InCondition.Has<ck::FTag_SmCondition_Polled>())
    { return ECk_SmConditionMode::Polled; }

    return ECk_SmConditionMode::EventDriven;
}

auto
    UCk_Utils_SmCondition_UE::
    Get_OwningStateMachine(
        const FCk_Handle_SmCondition& InCondition)
    -> FCk_Handle_StateMachine
{
    return ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(InCondition);
}

// --------------------------------------------------------------------------------------------------------------------
