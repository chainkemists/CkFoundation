#include "CkSmCondition_EntityScript.h"

#include "CkStateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"

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
    auto Result = DoEvaluate(DoGet_ScriptEntity());
    return _NegateResult ? NOT Result : Result;
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

    ConditionHandle.Get<ck::FFragment_SmCondition_Current>().Set_IsSatisfied(_NegateResult ? false : true);
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

    ConditionHandle.Get<ck::FFragment_SmCondition_Current>().Set_IsSatisfied(_NegateResult ? true : false);
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

static auto
    ComputeTagFromClassName(
        const FString& InClassName,
        const FString& InComment)
    -> FGameplayTag
{
    auto ClassName = InClassName;

    if (ClassName.EndsWith(TEXT("_C")))
    { ClassName = ClassName.LeftChop(2); }

    ClassName = ClassName.Replace(TEXT("_"), TEXT("."));

    return UCk_Utils_GameplayTag_UE::ResolveGameplayTag(*ClassName, InComment);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_EntityScript::
    Get_ConditionTag() const
    -> FGameplayTag
{
    return ComputeTagFromClassName(
        GetClass()->GetName(),
        ck::Format_UE(TEXT("Auto-generated condition tag for {}"), GetClass()));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_EntityScript::
    Get_ConditionTagForClass(
        TSubclassOf<UCk_SmCondition_EntityScript> InClass)
    -> FGameplayTag
{
    CK_ENSURE_IF_NOT(ck::IsValid(InClass),
        TEXT("Invalid condition class in Get_ConditionTagForClass"))
    { return {}; }

    return ComputeTagFromClassName(
        InClass->GetName(),
        ck::Format_UE(TEXT("Auto-generated condition tag for {}"), *InClass->GetName()));
}

// --------------------------------------------------------------------------------------------------------------------
