#include "CkSmCondition_SubSmFinished.h"

#include "CkStateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/CkStateMachine_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_SubSmFinished::
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

    for (const auto& ChildHandle : StateChildren)
    {
        if (NOT ChildHandle.Has<ck::FFragment_SmTask_SubStateMachine>())
        {
            continue;
        }

        auto SubSmHandle = ChildHandle.Get<ck::FFragment_SmTask_SubStateMachine>()
            .Get_SubStateMachineHandle();

        if (ck::Is_NOT_Valid(SubSmHandle))
        {
            continue;
        }

        auto RunStatus = UCk_Utils_StateMachine_UE::Get_RunStatus(SubSmHandle);
        return RunStatus == ECk_SmRunStatus::Stopped;
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------
