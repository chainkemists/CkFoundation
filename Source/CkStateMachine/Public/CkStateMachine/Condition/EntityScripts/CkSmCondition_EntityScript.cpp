#include "CkSmCondition_EntityScript.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

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

FCk_Handle_StateMachine
    UCk_SmCondition_EntityScript::
    Get_OwningStateMachine() const
{
    auto ConditionHandle = DoGet_ScriptEntity();

    if (ck::Is_NOT_Valid(ConditionHandle))
    { return {}; }

    if (NOT ck::TUtils_Sm_OwningStateMachine::Has(ConditionHandle))
    { return {}; }

    return ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(ConditionHandle);
}

FCk_Handle_SmTransition
    UCk_SmCondition_EntityScript::
    Get_ParentTransition() const
{
    auto ConditionHandle = DoGet_ScriptEntity();

    if (ck::Is_NOT_Valid(ConditionHandle))
    { return {}; }

    if (NOT ck::TUtils_Sm_ParentTransition::Has(ConditionHandle))
    { return {}; }

    return ck::TUtils_Sm_ParentTransition::Get_StoredEntity(ConditionHandle);
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

namespace ck_state_machine_entity_script
{
    auto ComputeTagFromClassName(const FString& InClassName, const FString& InComment) -> FGameplayTag;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_EntityScript::
    Get_ConditionTag() const
    -> FGameplayTag
{
    return ck_state_machine_entity_script::ComputeTagFromClassName(
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

    return ck_state_machine_entity_script::ComputeTagFromClassName(
        InClass->GetName(),
        ck::Format_UE(TEXT("Auto-generated condition tag for {}"), *InClass->GetName()));
}

// --------------------------------------------------------------------------------------------------------------------
