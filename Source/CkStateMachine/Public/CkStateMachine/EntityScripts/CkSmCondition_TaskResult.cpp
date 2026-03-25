#include "CkSmCondition_TaskResult.h"

#include "CkStateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_TaskResult::
    Evaluate() const
    -> bool
{
    if (ck::Is_NOT_Valid(_TaskClass))
    {
        return false;
    }

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

    for (const auto& ChildHandle : StateChildren)
    {
        if (NOT ChildHandle.Has<ck::FFragment_SmTask_Current>())
        {
            continue;
        }

        if (NOT ChildHandle.Has<ck::FFragment_EntityScript_Current>())
        {
            continue;
        }

        auto* Script = ChildHandle.Get<ck::FFragment_EntityScript_Current>().Get_Script().Get();
        if (ck::Is_NOT_Valid(Script))
        {
            continue;
        }

        if (Script->GetClass() == _TaskClass.Get())
        {
            return ChildHandle.Get<ck::FFragment_SmTask_Current>().Get_LastResult() == _ExpectedResult;
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------
