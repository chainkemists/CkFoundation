#include "CkSmTask_Utils.h"

#include "CkStateMachine/Task/EntityScripts/CkSmTask_EntityScript.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
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

    UCk_Utils_Handle_UE::Set_DebugName(TaskEntity, InTaskClass->GetFName());

    if (InOwnerState.Has<ck::FFragment_Sm_Context>())
    {
        const auto& Context = InOwnerState.Get<ck::FFragment_Sm_Context>();
        TaskEntity.Add<ck::FFragment_Sm_Context>(Context.Get_GameEntityHandle());
    }

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

    UCk_Utils_StateMachine_UE::RecordOfSmTasks_Utils::AddIfMissing(InOwnerState);
    UCk_Utils_StateMachine_UE::RecordOfSmTasks_Utils::Request_Connect(
        InOwnerState, TaskEntityTyped, ECk_Record_LabelRequirementPolicy::Optional);

    ck::TUtils_Sm_ParentState::AddOrReplace(TaskEntity, InOwnerState);
    ck::TUtils_Sm_OwningStateMachine::AddOrReplace(TaskEntity,
        ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(InOwnerState));

    UCk_Utils_EntityScript_UE::Add(TaskEntity, InTaskClass, FInstancedStruct{});

    return TaskEntityTyped;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmTask_UE::
    Request_UpdateTaskResult(
        FCk_Handle_SmTask& InTask,
        ECk_SmTaskResult InResult)
    -> FCk_Handle_SmTask
{
    const auto PrevResult = InTask.Get<ck::FFragment_SmTask_Current>().Get_LastResult();
    InTask.Get<ck::FFragment_SmTask_Current>()._LastResult = InResult;

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
