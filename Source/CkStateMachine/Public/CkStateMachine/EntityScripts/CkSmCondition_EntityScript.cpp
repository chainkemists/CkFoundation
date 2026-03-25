#include "CkSmCondition_EntityScript.h"

#include "CkStateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_EntityScript::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    return Super::Construct(InHandle, InSpawnParams);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_EntityScript::
    Evaluate() const
    -> bool
{
    return DoEvaluate(DoGet_ScriptEntity());
}

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_SmCondition_EntityScript::
    MarkSatisfied()
{
    auto ConditionHandle = DoGet_ScriptEntity();

    if (ck::Is_NOT_Valid(ConditionHandle))
    { return; }

    if (NOT ConditionHandle.Has<ck::FFragment_SmCondition_Current>())
    { return; }

    ConditionHandle.Get<ck::FFragment_SmCondition_Current>().Set_IsSatisfied(true);
}

void
    UCk_SmCondition_EntityScript::
    MarkUnsatisfied()
{
    auto ConditionHandle = DoGet_ScriptEntity();

    if (ck::Is_NOT_Valid(ConditionHandle))
    { return; }

    if (NOT ConditionHandle.Has<ck::FFragment_SmCondition_Current>())
    { return; }

    ConditionHandle.Get<ck::FFragment_SmCondition_Current>().Set_IsSatisfied(false);
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Handle_StateMachine
    UCk_SmCondition_EntityScript::
    DoGet_OwnerStateMachine() const
{
    auto ConditionHandle = DoGet_ScriptEntity();

    if (ck::Is_NOT_Valid(ConditionHandle))
    { return {}; }

    auto TransitionHandle = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(ConditionHandle);
    if (ck::Is_NOT_Valid(TransitionHandle))
    { return {}; }

    if (TransitionHandle.Has<ck::FFragment_SmTransition_Params>())
    {
        return TransitionHandle.Get<ck::FFragment_SmTransition_Params>().Get_OwnerStateMachine();
    }

    return {};
}

auto
    UCk_SmCondition_EntityScript::
    DoGet_GameEntity() const
    -> FCk_Handle
{
    auto ConditionHandle = DoGet_ScriptEntity();

    if (ConditionHandle.Has<ck::FFragment_Sm_Context>())
    {
        const auto& Context = ConditionHandle.Get<ck::FFragment_Sm_Context>();
        return Context.Get_GameEntityHandle();
    }

    return {};
}

// --------------------------------------------------------------------------------------------------------------------
