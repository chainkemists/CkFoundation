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
    return ck::IsValid(InHandle) && InHandle.Has<ck::FFragment_SmTask_Current>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmTask_UE::
    Create(
        FCk_Handle_SmState& InOwnerState,
        TSubclassOf<UCk_SmTask_EntityScript> InTaskClass)
    -> FCk_Handle_SmTask
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwnerState),
        TEXT("Invalid state handle in SmTask Create"))
    { return {}; }

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

    const auto* TaskCDO = GetDefault<UCk_SmTask_EntityScript>(InTaskClass);
    if (ck::IsValid(TaskCDO))
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
    MarkTaskAs_Succeeded(
        FCk_Handle_SmTask& InTask)
    -> FCk_Handle_SmTask
{
    CK_ENSURE_IF_NOT(ck::IsValid(InTask),
        TEXT("Invalid task handle in MarkTaskAs_Succeeded"))
    { return InTask; }

    CK_ENSURE_IF_NOT(InTask.Has<ck::FFragment_SmTask_Current>(),
        TEXT("Task entity [{}] is missing FFragment_SmTask_Current in MarkTaskAs_Succeeded"), InTask)
    { return InTask; }

    const auto PrevResult = InTask.Get<ck::FFragment_SmTask_Current>().Get_LastResult();
    InTask.Get<ck::FFragment_SmTask_Current>()._LastResult = ECk_SmTaskResult::Succeeded;

    if (PrevResult == ECk_SmTaskResult::Running)
    {
        InTask.AddOrGet<ck::FTag_SmTask_ResultDirty>();
    }

    return InTask;
}

auto
    UCk_Utils_SmTask_UE::
    MarkTaskAs_Failed(
        FCk_Handle_SmTask& InTask)
    -> FCk_Handle_SmTask
{
    CK_ENSURE_IF_NOT(ck::IsValid(InTask),
        TEXT("Invalid task handle in MarkTaskAs_Failed"))
    { return InTask; }

    CK_ENSURE_IF_NOT(InTask.Has<ck::FFragment_SmTask_Current>(),
        TEXT("Task entity [{}] is missing FFragment_SmTask_Current in MarkTaskAs_Failed"), InTask)
    { return InTask; }

    const auto PrevResult = InTask.Get<ck::FFragment_SmTask_Current>().Get_LastResult();
    InTask.Get<ck::FFragment_SmTask_Current>()._LastResult = ECk_SmTaskResult::Failed;

    if (PrevResult == ECk_SmTaskResult::Running)
    {
        InTask.AddOrGet<ck::FTag_SmTask_ResultDirty>();
    }

    return InTask;
}

auto
    UCk_Utils_SmTask_UE::
    MarkTaskAs_Running(
        FCk_Handle_SmTask& InTask)
    -> FCk_Handle_SmTask
{
    CK_ENSURE_IF_NOT(ck::IsValid(InTask),
        TEXT("Invalid task handle in MarkTaskAs_Running"))
    { return InTask; }

    CK_ENSURE_IF_NOT(InTask.Has<ck::FFragment_SmTask_Current>(),
        TEXT("Task entity [{}] is missing FFragment_SmTask_Current in MarkTaskAs_Running"), InTask)
    { return InTask; }

    InTask.Get<ck::FFragment_SmTask_Current>()._LastResult = ECk_SmTaskResult::Running;

    return InTask;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmTask_UE::
    Get_LastResult(
        const FCk_Handle_SmTask& InTask)
    -> ECk_SmTaskResult
{
    if (ck::Is_NOT_Valid(InTask))
    { return ECk_SmTaskResult::Running; }

    if (NOT InTask.Has<ck::FFragment_SmTask_Current>())
    { return ECk_SmTaskResult::Running; }

    return InTask.Get<ck::FFragment_SmTask_Current>().Get_LastResult();
}

// --------------------------------------------------------------------------------------------------------------------
