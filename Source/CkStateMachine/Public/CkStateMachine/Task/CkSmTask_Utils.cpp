#include "CkSmTask_Utils.h"

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
    MarkTaskAs_Succeeded(
        FCk_Handle_SmTask& InTask)
    -> FCk_Handle_SmTask
{
    CK_ENSURE_IF_NOT(ck::IsValid(InTask),
        TEXT("Invalid task handle in MarkTaskAs_Succeeded"))
    { return InTask; }

    auto TaskHandle = static_cast<FCk_Handle>(InTask);

    CK_ENSURE_IF_NOT(TaskHandle.Has<ck::FFragment_SmTask_Current>(),
        TEXT("Task entity [{}] is missing FFragment_SmTask_Current in MarkTaskAs_Succeeded"), InTask)
    { return InTask; }

    const auto PrevResult = TaskHandle.Get<ck::FFragment_SmTask_Current>().Get_LastResult();
    TaskHandle.Get<ck::FFragment_SmTask_Current>()._LastResult = ECk_SmTaskResult::Succeeded;

    if (PrevResult == ECk_SmTaskResult::Running)
    {
        TaskHandle.AddOrGet<ck::FTag_SmTask_ResultDirty>();
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

    auto TaskHandle = static_cast<FCk_Handle>(InTask);

    CK_ENSURE_IF_NOT(TaskHandle.Has<ck::FFragment_SmTask_Current>(),
        TEXT("Task entity [{}] is missing FFragment_SmTask_Current in MarkTaskAs_Failed"), InTask)
    { return InTask; }

    const auto PrevResult = TaskHandle.Get<ck::FFragment_SmTask_Current>().Get_LastResult();
    TaskHandle.Get<ck::FFragment_SmTask_Current>()._LastResult = ECk_SmTaskResult::Failed;

    if (PrevResult == ECk_SmTaskResult::Running)
    {
        TaskHandle.AddOrGet<ck::FTag_SmTask_ResultDirty>();
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

    auto TaskHandle = static_cast<FCk_Handle>(InTask);

    CK_ENSURE_IF_NOT(TaskHandle.Has<ck::FFragment_SmTask_Current>(),
        TEXT("Task entity [{}] is missing FFragment_SmTask_Current in MarkTaskAs_Running"), InTask)
    { return InTask; }

    TaskHandle.Get<ck::FFragment_SmTask_Current>()._LastResult = ECk_SmTaskResult::Running;

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

    auto TaskHandle = static_cast<FCk_Handle>(InTask);

    if (NOT TaskHandle.Has<ck::FFragment_SmTask_Current>())
    { return ECk_SmTaskResult::Running; }

    return TaskHandle.Get<ck::FFragment_SmTask_Current>().Get_LastResult();
}

// --------------------------------------------------------------------------------------------------------------------
