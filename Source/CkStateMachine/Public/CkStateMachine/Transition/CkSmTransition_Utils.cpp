#include "CkSmTransition_Utils.h"

#include "CkStateMachine/Condition/CkSmCondition_Fragment.h"
#include "CkStateMachine/Condition/CkSmCondition_Utils.h"
#include "CkStateMachine/Debug/CkStateMachine_Debug_GraphWalk_Fragment.h"
#include "CkStateMachine/State/CkSmState_Fragment.h"
#include "CkStateMachine/State/CkSmState_Utils.h"
#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmTransition_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return ck::IsValid(InHandle) && InHandle.Has_All<ck::FFragment_SmTransition_Params, ck::FFragment_SmTransition_Current>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmTransition_UE::
    Create(
        FCk_Handle_SmState& InOwnerState,
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass)
    -> FCk_Handle_SmTransition
{
    CK_ENSURE_IF_NOT(ck::IsValid(InTargetStateClass),
        TEXT("Invalid target state class in SmTransition Create"))
    { return {}; }

    auto TransitionEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwnerState);

    // SM graph element — save-transient, the redrive recreates it (see CkSmState_Utils::Create).
    TransitionEntity.Add<ck::FTag_Snapshot_SaveTransient>();

    UCk_Utils_Handle_UE::Set_DebugName(TransitionEntity,
        *ck::Format_UE(TEXT("Transition -> {}"), InTargetStateClass->GetFName()));

    TransitionEntity.Add<ck::FFragment_SmTransition_Current>();
    TransitionEntity.Add<ck::FFragment_SmTransition_Params>(
        ck::FFragment_SmTransition_Params{InTargetStateClass});

    auto TransitionEntityTyped = CastChecked(TransitionEntity);

    if (InOwnerState.Has<ck::FTag_Sm_Debug_GraphWalkEntity>())
    { TransitionEntity.Add<ck::FTag_Sm_Debug_GraphWalkEntity>(); }

    UCk_Utils_StateMachine_UE::RecordOfSmTransitions_Utils::AddIfMissing(InOwnerState);
    UCk_Utils_StateMachine_UE::RecordOfSmTransitions_Utils::Request_Connect(
        InOwnerState, TransitionEntityTyped, ECk_Record_LabelRequirementPolicy::Optional);

    ck::TUtils_Sm_ParentState::AddOrReplace(TransitionEntity, InOwnerState);

    if (ck::TUtils_Sm_OwningStateMachine::Has(InOwnerState))
    {
        const auto OwningSm = ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(InOwnerState);
        ck::TUtils_Sm_OwningStateMachine::AddOrReplace(TransitionEntity, OwningSm);
    }

    // Must run AFTER the parent-state link above so the mark cascades to the parent state — a
    // vacuous (zero-condition) transition needs the parent to tick to evaluate its Pass.
    Request_MarkTransition_AsNotFullyEventDriven(TransitionEntityTyped);

    return TransitionEntityTyped;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmTransition_UE::
    Request_Exit(
        FCk_Handle_SmTransition& InTransition)
    -> FCk_Handle_SmTransition
{
    if (ck::Is_NOT_Valid(InTransition))
    { return InTransition; }

    // A target already in the destruction pipeline gets its exit from the EntityScript EndPlay
    // path (Active-deduped); tagging it here would leave a pending-exit key no Exit processor
    // can reach (their views skip dying entities), which pins the settle barrier's presence check.
    if (UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InTransition, ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    { return InTransition; }

    InTransition.AddOrGet<ck::FTag_SmTransition_PendingExit>();
    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InTransition);
    return InTransition;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmTransition_UE::
    Request_MarkTransition_AsNotFullyEventDriven(
        FCk_Handle_SmTransition& InTransition)
    -> FCk_Handle_SmTransition
{
    InTransition.Try_Remove<ck::FTag_SmTransition_FullyEventDriven>();

    if (ck::TUtils_Sm_ParentState::Has(InTransition))
    {
        auto ParentState = UCk_Utils_SmState_UE::CastChecked(
            ck::TUtils_Sm_ParentState::Get_StoredEntity(InTransition));
        UCk_Utils_SmState_UE::Request_MarkState_AsNotFullyEventDriven(ParentState);
    }

    return InTransition;
}

auto
    UCk_Utils_SmTransition_UE::
    Request_RecomputeFullyEventDrivenStatus(
        FCk_Handle_SmTransition& InTransition)
    -> FCk_Handle_SmTransition
{
    auto ConditionCount = int32{0};
    auto HasPolled = false;

    UCk_Utils_StateMachine_UE::RecordOfSmConditions_Utils::ForEach_ValidEntry(InTransition,
    [&](FCk_Handle_SmCondition InCondition) -> ECk_Record_ForEachIterationResult
    {
        ++ConditionCount;
        FCk_Handle Generic = InCondition;
        if (Generic.Has<ck::FTag_SmCondition_Polled>())
        {
            HasPolled = true;
            return ECk_Record_ForEachIterationResult::Break;
        }
        return ECk_Record_ForEachIterationResult::Continue;
    });

    const auto ShouldBeFullyEventDriven = (ConditionCount > 0) && (NOT HasPolled);

    if (ShouldBeFullyEventDriven)
    { InTransition.AddOrGet<ck::FTag_SmTransition_FullyEventDriven>(); }
    else
    { InTransition.Try_Remove<ck::FTag_SmTransition_FullyEventDriven>(); }

    if (ck::TUtils_Sm_ParentState::Has(InTransition))
    {
        auto ParentState = UCk_Utils_SmState_UE::CastChecked(
            ck::TUtils_Sm_ParentState::Get_StoredEntity(InTransition));
        UCk_Utils_SmState_UE::Request_RecomputeFullyEventDrivenStatus(ParentState);
    }

    return InTransition;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmTransition_UE::
    Request_StartEvaluating(
        FCk_Handle_SmTransition& InTransition)
    -> FCk_Handle_SmTransition
{
    InTransition.Get<ck::FFragment_SmTransition_Current>().Set_Result(ECk_SmTransitionResult::Undetermined);
    InTransition.AddOrGet<ck::FTag_SmTransition_Evaluating>();

    return InTransition;
}

auto
    UCk_Utils_SmTransition_UE::
    Request_UpdateTransitionResult(
        FCk_Handle_SmTransition& InTransition,
        ECk_SmTransitionResult InResult)
    -> FCk_Handle_SmTransition
{
    InTransition.Get<ck::FFragment_SmTransition_Current>().Set_Result(InResult);
    InTransition.Try_Remove<ck::FTag_SmTransition_Evaluating>();

    return InTransition;
}

auto
    UCk_Utils_SmTransition_UE::
    Request_ResetTransition(
        FCk_Handle_SmTransition& InTransition)
    -> FCk_Handle_SmTransition
{
    InTransition.Get<ck::FFragment_SmTransition_Current>().Set_Result(ECk_SmTransitionResult::Undetermined);

    if (Get_IsFullyEventDriven(InTransition))
    { return InTransition; }

    // Polled conditions only: an event-driven condition reset to Undetermined would wait forever
    // if its event never fires again — it re-triggers via Request_UpdateConditionResult instead.
    for (const auto& Conditions = UCk_Utils_StateMachine_UE::RecordOfSmConditions_Utils::Get_ValidEntries(InTransition);
        auto Condition : Conditions)
    {
        if (Condition.Has<ck::FTag_SmCondition_Polled>())
        {
            UCk_Utils_SmCondition_UE::Request_ResetCondition(Condition);
        }
    }

    return InTransition;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmTransition_UE::
    Get_IsFullyEventDriven(
        const FCk_Handle_SmTransition& InTransition)
    -> bool
{
    return InTransition.Has<ck::FTag_SmTransition_FullyEventDriven>();
}

auto
    UCk_Utils_SmTransition_UE::
    Get_EvaluationResult(
        const FCk_Handle_SmTransition& InTransition)
    -> ECk_SmTransitionResult
{
    return InTransition.Get<ck::FFragment_SmTransition_Current>().Get_Result();
}

auto
    UCk_Utils_SmTransition_UE::
    Get_TargetStateClass(
        const FCk_Handle_SmTransition& InTransition)
    -> TSubclassOf<UCk_SmState_EntityScript>
{
    return InTransition.Get<ck::FFragment_SmTransition_Params>().Get_TargetStateClass();
}

auto
    UCk_Utils_SmTransition_UE::
    Get_OwningStateMachine(
        const FCk_Handle_SmTransition& InTransition)
    -> FCk_Handle_StateMachine
{
    return ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(InTransition);
}

// --------------------------------------------------------------------------------------------------------------------
