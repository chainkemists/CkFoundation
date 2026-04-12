#include "CkSmState_EntityScript.h"

#include "CkStateMachine/Task/EntityScripts/CkSmTask_EntityScript.h"
#include "CkStateMachine/Condition/EntityScripts/CkSmCondition_EntityScript.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"
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

    _TransitionOrderCounter = 0;
    DefineState(InHandle);

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
        FCk_Handle& InHandle)
    -> void
{
    DoDefineState(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    AddTask(
        TSubclassOf<UCk_SmTask_EntityScript> InTaskClass)
    -> FCk_Handle_SmTask
{
    CK_ENSURE_IF_NOT(ck::IsValid(InTaskClass),
        TEXT("Invalid task class in AddTask"))
    { return {}; }

    auto StateHandle = DoGet_ScriptEntity();

    CK_ENSURE_IF_NOT(ck::IsValid(StateHandle),
        TEXT("Invalid state handle in AddTask"))
    { return {}; }

    auto TaskEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(StateHandle);

    UCk_Utils_Handle_UE::Set_DebugName(TaskEntity, InTaskClass->GetFName());

    if (_OwnerStateMachine.Has<ck::FFragment_Sm_Context>())
    {
        const auto& Context = _OwnerStateMachine.Get<ck::FFragment_Sm_Context>();
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

    auto TaskEntityTyped = UCk_Utils_SmTask_UE::CastChecked(TaskEntity);
    auto StateHandleTyped = UCk_Utils_SmState_UE::CastChecked(StateHandle);

    UCk_Utils_StateMachine_UE::RecordOfSmTasks_Utils::AddIfMissing(StateHandle);
    UCk_Utils_StateMachine_UE::RecordOfSmTasks_Utils::Request_Connect(
        StateHandle, TaskEntityTyped, ECk_Record_LabelRequirementPolicy::Optional);

    ck::TUtils_Sm_ParentState::AddOrReplace(TaskEntity, StateHandleTyped);
    ck::TUtils_Sm_OwningStateMachine::AddOrReplace(TaskEntity, _OwnerStateMachine);

    UCk_Utils_EntityScript_UE::Add(
        TaskEntity,
        InTaskClass,
        FInstancedStruct{});

    return TaskEntityTyped;
}

auto
    UCk_SmState_EntityScript::
    AddTransition(
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass)
    -> FCk_Handle_SmTransition
{
    auto StateHandle = UCk_Utils_SmState_UE::CastChecked(DoGet_ScriptEntity());
    return UCk_Utils_SmTransition_UE::Create(StateHandle, InTargetStateClass, _TransitionOrderCounter++);
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
