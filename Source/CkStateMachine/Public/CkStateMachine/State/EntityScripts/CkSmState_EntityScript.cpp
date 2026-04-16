#include "CkSmState_EntityScript.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

#include "CkStateMachine/Task/CkSmTask_Utils.h"
#include "CkStateMachine/Transition/CkSmTransition_Utils.h"
#include "CkStateMachine/Condition/CkSmCondition_Utils.h"
#include "CkStateMachine/State/CkSmState_Utils.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"
#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"

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

    auto StateHandle = ck::StaticCast<FCk_Handle_SmState_UnderConstruction>(InHandle);
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
        FCk_Handle_SmState_UnderConstruction& InHandle)
    -> void
{
    DoDefineState(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    Get_StatesToOverride() const
    -> TArray<FGameplayTag>
{
    return DoGet_StatesToOverride();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    AddTask(
        FCk_Handle_SmState_UnderConstruction& InStateHandle,
        TSubclassOf<UCk_SmTask_EntityScript> InTaskClass) const
    -> FCk_Handle_SmTask
{
    return UCk_Utils_SmTask_UE::Create(InStateHandle, InTaskClass);
}

auto
    UCk_SmState_EntityScript::
    AddTransition(
        FCk_Handle_SmState_UnderConstruction& InStateHandle,
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass) const
    -> FCk_Handle_SmTransition
{
    return UCk_Utils_SmTransition_UE::Create(InStateHandle, InTargetStateClass);
}

auto
    UCk_SmState_EntityScript::
    AddCondition(
        FCk_Handle_SmTransition& InTransition,
        TSubclassOf<UCk_SmCondition_EntityScript> InConditionClass) const
    -> FCk_Handle_SmCondition
{
    return UCk_Utils_SmCondition_UE::Create(InTransition, InConditionClass);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    ComposeFromState(
        FCk_Handle_SmState_UnderConstruction& InStateHandle,
        TSubclassOf<UCk_SmState_EntityScript> InOtherStateClass) const
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOtherStateClass),
        TEXT("Invalid OtherStateClass in ComposeFromState on [{}]"), InStateHandle)
    { return; }

    thread_local TSet<UClass*> VisitedClasses;

    CK_ENSURE_IF_NOT(NOT VisitedClasses.Contains(InOtherStateClass.Get()),
        TEXT("Infinite recursion detected in ComposeFromState: [{}] already visited"),
        *InOtherStateClass->GetName())
    { return; }

    VisitedClasses.Add(InOtherStateClass.Get());
    ON_SCOPE_EXIT { VisitedClasses.Remove(InOtherStateClass.Get()); };

    auto* CDO = InOtherStateClass->GetDefaultObject<UCk_SmState_EntityScript>();
    CDO->DefineState(InStateHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    Get_OwnerStateMachine() const
    -> FCk_Handle_StateMachine
{
    return _OwnerStateMachine;
}

auto
    UCk_SmState_EntityScript::
    Get_GameEntity() const
    -> FCk_Handle
{
    if (auto StateHandle = DoGet_ScriptEntity();
        StateHandle.Has<ck::FFragment_Sm_Context>())
    {
        const auto& Context = StateHandle.Get<ck::FFragment_Sm_Context>();
        return Context.Get_GameEntityHandle();
    }

    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    Get_StateTag() const
    -> FGameplayTag
{
    return UCk_Utils_Object_UE::Get_TagFromClassName(
        GetClass(),
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

    return UCk_Utils_Object_UE::Get_TagFromClassName(
        InClass,
        ck::Format_UE(TEXT("Auto-generated state tag for {}"), *InClass->GetName()));
}

// --------------------------------------------------------------------------------------------------------------------
