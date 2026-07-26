#include "CkSmCondition_Utils.h"

#include "CkStateMachine/Condition/EntityScripts/CkSmCondition_EntityScript.h"
#include "CkStateMachine/Condition/EntityScripts/CkSmCondition_Polled.h"
#include "CkStateMachine/Debug/CkStateMachine_Debug_GraphWalk_Fragment.h"
#include "CkStateMachine/Transition/CkSmTransition_Fragment.h"
#include "CkStateMachine/Transition/CkSmTransition_Utils.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h" // FTag_Snapshot_SaveTransient

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

    // SM graph element — save-transient, the redrive recreates it (see CkSmState_Utils::Create).
    ConditionEntity.Add<ck::FTag_Snapshot_SaveTransient>();

    UCk_Utils_Handle_UE::Set_DebugName(ConditionEntity, InConditionClass->GetFName());

    if (InConditionClass->IsChildOf(UCk_SmCondition_Polled::StaticClass()))
    {
        ConditionEntity.Add<ck::FTag_SmCondition_Polled>();

        UCk_Utils_SmTransition_UE::Request_MarkTransition_AsNotFullyEventDriven(InOwnerTransition);
    }
    else if (InConditionClass->IsChildOf(UCk_SmCondition_EventDriven::StaticClass()))
    {
        ConditionEntity.Add<ck::FTag_SmCondition_EventDriven>();
        // Recompute is deferred to after the Connect below, where the walk can see this condition.
    }
    else
    {
        CK_TRIGGER_ENSURE(TEXT("Attempting to create an HFSM Condition with class [{}] that is neither Polled or EventDriven"), InConditionClass);

        // Do NOT fall through: a mode-less condition is never evaluated, stays Undetermined forever,
        // and the owning state's evaluate walk waits on it — the SM silently deadlocks.
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(ConditionEntity);
        return {};
    }

    ConditionEntity.Add<ck::FFragment_SmCondition_Current>();
    ConditionEntity.Add<ck::FFragment_SmCondition_Params>(InConditionClass);

    auto ConditionEntityTyped = CastChecked(ConditionEntity);

    if (InOwnerTransition.Has<ck::FTag_Sm_Debug_GraphWalkEntity>())
    { ConditionEntity.Add<ck::FTag_Sm_Debug_GraphWalkEntity>(); }

    UCk_Utils_StateMachine_UE::RecordOfSmConditions_Utils::AddIfMissing(InOwnerTransition);
    UCk_Utils_StateMachine_UE::RecordOfSmConditions_Utils::Request_Connect(
        InOwnerTransition, ConditionEntityTyped, ECk_Record_LabelRequirementPolicy::Optional);

    // Transitions default to NOT FullyEventDriven at Create (for the vacuous-transition case), so
    // adding an EventDriven condition with no Polled siblings restores the optimization. Idempotent
    // with Request_MarkTransition_AsNotFullyEventDriven, so the Polled branch above benefits too.
    UCk_Utils_SmTransition_UE::Request_RecomputeFullyEventDrivenStatus(InOwnerTransition);

    ck::TUtils_Sm_ParentTransition::AddOrReplace(ConditionEntity, InOwnerTransition);

    if (ck::TUtils_Sm_OwningStateMachine::Has(InOwnerTransition))
    {
        const auto OwningSm = ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(InOwnerTransition);
        ck::TUtils_Sm_OwningStateMachine::AddOrReplace(ConditionEntity, OwningSm);
    }

    // Deferred attach (FProcessor_SmScript_CommitPendingAttach) lets a condition added during
    // DefineState be removed in the same frame without its script reaching Construct/BeginPlay.
    ConditionEntity.Add<ck::FFragment_SmScript_PendingAttach>(InConditionClass);
    ConditionEntity.Add<ck::FTag_SmScript_PendingAttach>();

    return ConditionEntityTyped;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmCondition_UE::
    Request_Exit(
        FCk_Handle_SmCondition& InCondition)
    -> FCk_Handle_SmCondition
{
    if (ck::Is_NOT_Valid(InCondition))
    { return InCondition; }

    InCondition.AddOrGet<ck::FTag_SmCondition_PendingExit>();
    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InCondition);
    return InCondition;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmCondition_UE::
    Request_StartOrResumeEvaluating(
        FCk_Handle_SmCondition& InCondition)
    -> FCk_Handle_SmCondition
{
    InCondition.AddOrGet<ck::FTag_SmCondition_Evaluating>();

    return InCondition;
}

auto
    UCk_Utils_SmCondition_UE::
    Request_PauseEvaluation(
        FCk_Handle_SmCondition& InCondition)
    -> FCk_Handle_SmCondition
{
    InCondition.Try_Remove<ck::FTag_SmCondition_Evaluating>();

    return InCondition;
}

auto
    UCk_Utils_SmCondition_UE::
    Request_ResetCondition(
        FCk_Handle_SmCondition& InCondition)
    -> FCk_Handle_SmCondition
{
    InCondition.Get<ck::FFragment_SmCondition_Current>().Set_Result(ECk_SmConditionResult::Undetermined);
    InCondition.AddOrGet<ck::FTag_SmCondition_Evaluating>();

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
    // AddOrGet bumps the dirty-marker version even when the tag already exists.
    auto ParentTransitionHandle = ck::TUtils_Sm_ParentTransition::Get_StoredEntity(InCondition);

    ParentTransitionHandle.AddOrGet<ck::FTag_SmTransition_Evaluating>();

    return InCondition;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmCondition_UE::
    Request_SetInitialResult(
        FCk_Handle_SmCondition& InCondition,
        ECk_SmConditionResult InResult)
    -> FCk_Handle_SmCondition
{
    // Direct write only — no parent-transition wake-up. See header for usage rules.
    InCondition.Get<ck::FFragment_SmCondition_Current>().Set_Result(InResult);

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

auto
    UCk_Utils_SmCondition_UE::
    Get_ScriptClass(
        const FCk_Handle_SmCondition& InCondition)
    -> TSubclassOf<UCk_SmCondition_EntityScript>
{
    if (ck::Is_NOT_Valid(InCondition) || NOT InCondition.Has<ck::FFragment_SmCondition_Params>())
    { return nullptr; }

    return InCondition.Get<ck::FFragment_SmCondition_Params>().Get_ScriptClass();
}

// --------------------------------------------------------------------------------------------------------------------
