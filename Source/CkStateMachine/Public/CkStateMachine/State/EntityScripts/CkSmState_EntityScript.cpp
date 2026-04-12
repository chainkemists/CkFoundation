#include "CkSmState_EntityScript.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

#include "CkStateMachine/Task/CkSmTask_Utils.h"
#include "CkStateMachine/Transition/CkSmTransition_Utils.h"
#include "CkStateMachine/Condition/CkSmCondition_Utils.h"
#include "CkStateMachine/State/CkSmState_Utils.h"
#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    const auto ParentFlow = Super::Construct(InHandle, InSpawnParams);

    if (ck::TUtils_Sm_OwningStateMachine::Has(InHandle))
    {
        _OwnerStateMachine = ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(InHandle);
    }

    auto StateHandle = UCk_Utils_SmState_UE::CastChecked(InHandle);
    DefineState(StateHandle);

    return ParentFlow;
}

auto
    UCk_SmState_EntityScript::
    BeginPlay()
    -> void
{
    Super::BeginPlay();
}

auto
    UCk_SmState_EntityScript::
    EndPlay()
    -> void
{
    Super::EndPlay();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    DefineState(
        FCk_Handle_SmState& InHandle)
    -> void
{
    DoDefineState(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    AddTask(
        TSubclassOf<UCk_SmTask_EntityScript> InTaskClass) const
    -> FCk_Handle_SmTask
{
    auto StateHandle = UCk_Utils_SmState_UE::CastChecked(DoGet_ScriptEntity());
    return UCk_Utils_SmTask_UE::Create(StateHandle, InTaskClass);
}

auto
    UCk_SmState_EntityScript::
    AddTransition(
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass) const
    -> FCk_Handle_SmTransition
{
    auto StateHandle = UCk_Utils_SmState_UE::CastChecked(DoGet_ScriptEntity());
    return UCk_Utils_SmTransition_UE::Create(StateHandle, InTargetStateClass);
}

auto
    UCk_SmState_EntityScript::
    AddCondition(
        FCk_Handle_SmTransition InTransition,
        TSubclassOf<UCk_SmCondition_EntityScript> InConditionClass)
    -> FCk_Handle_SmCondition
{
    return UCk_Utils_SmCondition_UE::Create(InTransition, InConditionClass);
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Handle_SmTask
    UCk_SmState_EntityScript::
    DoAddTask(
        TSubclassOf<UCk_SmTask_EntityScript> InTaskClass)
{
    return AddTask(InTaskClass);
}

FCk_Handle_SmTransition
    UCk_SmState_EntityScript::
    DoAddTransition(
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass)
{
    return AddTransition(InTargetStateClass);
}

FCk_Handle_SmCondition
    UCk_SmState_EntityScript::
    DoAddCondition(
        FCk_Handle_SmTransition InTransition,
        TSubclassOf<UCk_SmCondition_EntityScript> InConditionClass)
{
    return AddCondition(InTransition, InConditionClass);
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Handle_StateMachine
    UCk_SmState_EntityScript::
    DoGet_OwnerStateMachine() const
{
    return _OwnerStateMachine;
}

auto
    UCk_SmState_EntityScript::
    DoGet_GameEntity() const
    -> FCk_Handle
{
    auto StateHandle = DoGet_ScriptEntity();

    if (StateHandle.Has<ck::FFragment_Sm_Context>())
    {
        const auto& Context = StateHandle.Get<ck::FFragment_Sm_Context>();
        return Context.Get_GameEntityHandle();
    }

    return {};
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_state_machine_entity_script
{
    auto
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
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    Get_StateTag() const
    -> FGameplayTag
{
    return ck_state_machine_entity_script::ComputeTagFromClassName(
        GetClass()->GetName(),
        ck::Format_UE(TEXT("Auto-generated state tag for {}"), GetClass()));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    Get_StateTagForClass(
        TSubclassOf<UCk_SmState_EntityScript> InClass)
    -> FGameplayTag
{
    CK_ENSURE_IF_NOT(ck::IsValid(InClass),
        TEXT("Invalid state class in Get_StateTagForClass"))
    { return {}; }

    return ck_state_machine_entity_script::ComputeTagFromClassName(
        InClass->GetName(),
        ck::Format_UE(TEXT("Auto-generated state tag for {}"), *InClass->GetName()));
}

// --------------------------------------------------------------------------------------------------------------------
