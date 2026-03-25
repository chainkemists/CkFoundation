#include "CkSmCondition_TaskResults.h"

#include "CkStateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_TaskResults::
    Evaluate() const
    -> bool
{
    auto ConditionHandle = DoGet_ScriptEntity();
    if (ck::Is_NOT_Valid(ConditionHandle))
    {
        return false;
    }

    auto TransitionHandle = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(ConditionHandle);
    if (ck::Is_NOT_Valid(TransitionHandle))
    {
        return false;
    }

    auto StateHandle = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(TransitionHandle);
    if (ck::Is_NOT_Valid(StateHandle))
    {
        return false;
    }

    const auto StateChildren = UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(StateHandle);

    auto TickTaskCount = 0;
    auto SucceededCount = 0;
    auto FailedCount = 0;

    for (const auto& ChildHandle : StateChildren)
    {
        if (NOT ChildHandle.Has<ck::FFragment_SmTask_Current>())
        {
            continue;
        }

        if (NOT ChildHandle.Has<ck::FTag_SmTask_Tick>())
        {
            continue;
        }

        ++TickTaskCount;

        auto Result = ChildHandle.Get<ck::FFragment_SmTask_Current>().Get_LastResult();

        if (Result == ECk_SmTaskResult::Succeeded)
        {
            ++SucceededCount;
        }
        else if (Result == ECk_SmTaskResult::Failed)
        {
            ++FailedCount;
        }
    }

    if (TickTaskCount == 0)
    {
        return false;
    }

    switch (_Check)
    {
    case ECk_SmCondition_TaskResultsCheck::AnySucceeded:
        return SucceededCount > 0;

    case ECk_SmCondition_TaskResultsCheck::AnyFailed:
        return FailedCount > 0;

    case ECk_SmCondition_TaskResultsCheck::AllSucceeded:
        return SucceededCount == TickTaskCount;

    case ECk_SmCondition_TaskResultsCheck::AllFailed:
        return FailedCount == TickTaskCount;
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------
