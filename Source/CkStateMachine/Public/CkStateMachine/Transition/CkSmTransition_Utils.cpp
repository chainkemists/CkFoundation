#include "CkSmTransition_Utils.h"

#include "CkStateMachine/Condition/CkSmCondition_Fragment.h"
#include "CkStateMachine/Condition/CkSmCondition_Utils.h"
#include "CkStateMachine/Debug/CkStateMachine_Debug_GraphWalk_Fragment.h"
#include "CkStateMachine/State/CkSmState_Fragment.h"
#include "CkStateMachine/State/CkSmState_Utils.h"
#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h" // FTag_Snapshot_SaveTransient

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

    // A freshly-created transition has zero conditions — it's a vacuous
    // transition that will Pass on first evaluation. That requires the parent
    // state to be ticked (so State_Update adds FTag_SmState_NeedsEvaluation),
    // which means the state must NOT be FullyEventDriven.
    //
    // Run AFTER the parent-state link is established above so the cascade to
    // Request_MarkState_AsNotFullyEventDriven fires correctly.
    //
    // If a Polled condition is added later: state stays not-FullyEventDriven
    // (Request_MarkTransition_AsNotFullyEventDriven runs again — idempotent).
    // If an EventDriven condition is added later (and no Polled condition
    // exists), CkSmCondition_Utils::Create re-marks the transition + state
    // back to FullyEventDriven to preserve the perf optimization.
    //
    // The previous default of "Add FTag_SmTransition_FullyEventDriven at
    // Create" assumed every transition would eventually get an event-driven
    // condition. Vacuous transitions broke that assumption and starved the
    // state of evaluation. See FProcessor_SmTransition_Evaluate's vacuous-
    // Pass branch (CkSmTransition_Processor.cpp:49-59) — it handles the case
    // correctly once it gets a chance to run.
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
    // A transition qualifies as FullyEventDriven iff it has at least one condition
    // and every condition is EventDriven (zero Polled). Zero conditions = vacuous =
    // NOT FullyEventDriven (the parent state must tick to evaluate the vacuous Pass).
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

    // Only reset polled conditions. Event-driven conditions keep their last result:
    // when their event fires again, Request_UpdateConditionResult directly adds
    // FTag_SmTransition_Evaluating to re-trigger evaluation regardless of pause state.
    // Resetting event-driven conditions to Undetermined would leave them permanently
    // waiting if the event never fires again.
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
