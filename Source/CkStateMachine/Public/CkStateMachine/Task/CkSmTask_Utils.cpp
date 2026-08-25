#include "CkSmTask_Utils.h"

#include "CkStateMachine/Debug/CkStateMachine_Debug_GraphWalk_Fragment.h"
#include "CkStateMachine/Task/EntityScripts/CkSmTask_EntityScript.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h" // FTag_Snapshot_SaveTransient
#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmTask_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return ck::IsValid(InHandle) && InHandle.Has_All<ck::FFragment_SmTask_Current, ck::FFragment_SmTask_Params>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmTask_UE::
    Create(
        FCk_Handle_SmState& InOwnerState,
        TSubclassOf<UCk_SmTask_EntityScript> InTaskClass)
    -> FCk_Handle_SmTask
{
    CK_ENSURE_IF_NOT(ck::IsValid(InTaskClass),
        TEXT("Invalid task class in SmTask Create"))
    { return {}; }

    auto TaskEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwnerState);

    // SM graph element — save-transient, the redrive recreates it (see CkSmState_Utils::Create).
    TaskEntity.Add<ck::FTag_Snapshot_SaveTransient>();

    UCk_Utils_Handle_UE::Set_DebugName(TaskEntity, InTaskClass->GetFName());

    if (const auto* TaskCDO = GetDefault<UCk_SmTask_EntityScript>(InTaskClass);
        ck::IsValid(TaskCDO))
    {
        if (TaskCDO->Get_TaskMode() == ECk_SmTaskMode::Tick)
        {
            TaskEntity.Add<ck::FTag_SmTask_Tick>();
        }
        else
        {
            TaskEntity.Add<ck::FTag_SmTask_EnterExit>();
        }
    }

    TaskEntity.Add<ck::FFragment_SmTask_Current>();
    TaskEntity.Add<ck::FFragment_SmTask_Params>(InTaskClass);

    auto TaskEntityTyped = CastChecked(TaskEntity);

    if (InOwnerState.Has<ck::FTag_Sm_Debug_GraphWalkEntity>())
    { TaskEntity.Add<ck::FTag_Sm_Debug_GraphWalkEntity>(); }

    UCk_Utils_StateMachine_UE::RecordOfSmTasks_Utils::AddIfMissing(InOwnerState);
    UCk_Utils_StateMachine_UE::RecordOfSmTasks_Utils::Request_Connect(
        InOwnerState, TaskEntityTyped, ECk_Record_LabelRequirementPolicy::Optional);

    ck::TUtils_Sm_ParentState::AddOrReplace(TaskEntity, InOwnerState);
    ck::TUtils_Sm_OwningStateMachine::AddOrReplace(TaskEntity,
        ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(InOwnerState));

    // Deferring the attach (see FProcessor_SmScript_CommitPendingAttach) keeps the task handle usable
    // for chaining while making a same-frame RemoveTask race-free.
    TaskEntity.Add<ck::FFragment_SmScript_PendingAttach>(InTaskClass);
    TaskEntity.Add<ck::FTag_SmScript_PendingAttach>();

    return TaskEntityTyped;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmTask_UE::
    Request_Exit(
        FCk_Handle_SmTask& InTask)
    -> FCk_Handle_SmTask
{
    if (ck::Is_NOT_Valid(InTask))
    { return InTask; }

    // A target already in the destruction pipeline gets its exit from the EntityScript EndPlay
    // path (Active-deduped); tagging it here would leave a pending-exit key no Exit processor
    // can reach (their views skip dying entities), which pins the settle barrier's presence check.
    if (UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InTask, ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    { return InTask; }

    InTask.AddOrGet<ck::FTag_SmTask_PendingExit>();
    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InTask);
    return InTask;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmTask_UE::
    Request_UpdateTaskResult(
        FCk_Handle_SmTask& InTask,
        ECk_SmTaskResult InResult)
    -> FCk_Handle_SmTask
{
    auto& Current = InTask.Get<ck::FFragment_SmTask_Current>();
    const auto PrevResult = Current.Get_LastResult();

    // A terminal result not yet broadcast (ResultDirty pending) must not be clobbered back to
    // Running: Tick runs before FireFinishedSignal, so a same-frame Running (e.g. an unimplemented BP
    // DoTick, whose default is the enum's zero value) would broadcast "finished: Running" and lose
    // the completion. Post-broadcast flips remain legal.
    if (InResult == ECk_SmTaskResult::Running
        && PrevResult != ECk_SmTaskResult::Running
        && InTask.Has<ck::FTag_SmTask_ResultDirty>())
    { return InTask; }

    Current._LastResult = InResult;

    if (PrevResult == ECk_SmTaskResult::Running && InResult != ECk_SmTaskResult::Running)
    {
        InTask.AddOrGet<ck::FTag_SmTask_ResultDirty>();
    }

    return InTask;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmTask_UE::
    Get_LastResult(
        const FCk_Handle_SmTask& InTask)
    -> ECk_SmTaskResult
{
    return InTask.Get<ck::FFragment_SmTask_Current>().Get_LastResult();
}

auto
    UCk_Utils_SmTask_UE::
    Get_OwningStateMachine(
        const FCk_Handle_SmTask& InTask)
    -> FCk_Handle_StateMachine
{
    return ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(InTask);
}

auto
    UCk_Utils_SmTask_UE::
    Get_ScriptClass(
        const FCk_Handle_SmTask& InTask)
    -> TSubclassOf<UCk_SmTask_EntityScript>
{
    return InTask.Get<ck::FFragment_SmTask_Params>().Get_ScriptClass();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmTask_UE::
    BindTo_OnSubSmConstructed(
        FCk_Handle_SmTask& InTask,
        const FCk_Delegate_SmTask_OnSubSmConstructed& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_SmTask
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnSubSmConstructed, InTask, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InTask;
}

auto
    UCk_Utils_SmTask_UE::
    UnbindFrom_OnSubSmConstructed(
        FCk_Handle_SmTask& InTask,
        const FCk_Delegate_SmTask_OnSubSmConstructed& InDelegate)
    -> FCk_Handle_SmTask
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnSubSmConstructed, InTask, InDelegate);
    return InTask;
}

// --------------------------------------------------------------------------------------------------------------------
