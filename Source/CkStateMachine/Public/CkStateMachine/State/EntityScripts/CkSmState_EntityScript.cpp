#include "CkSmState_EntityScript.h"

#include "CkStateMachine/Task/EntityScripts/CkSmTask_EntityScript.h"
#include "CkStateMachine/Condition/EntityScripts/CkSmCondition_EntityScript.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"
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

    auto TaskEntityTyped = ck::StaticCast<FCk_Handle_SmTask>(TaskEntity);
    auto StateHandleTyped = ck::StaticCast<FCk_Handle_SmState>(StateHandle);

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
    CK_ENSURE_IF_NOT(ck::IsValid(InTargetStateClass),
        TEXT("Invalid target state class in AddTransition"))
    { return {}; }

    auto StateHandle = DoGet_ScriptEntity();

    CK_ENSURE_IF_NOT(ck::IsValid(StateHandle),
        TEXT("Invalid state handle in AddTransition"))
    { return {}; }

    auto TransitionEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(StateHandle);

    UCk_Utils_Handle_UE::Set_DebugName(TransitionEntity,
        *ck::Format_UE(TEXT("Transition -> {}"), InTargetStateClass->GetFName()));

    if (_OwnerStateMachine.Has<ck::FFragment_Sm_Context>())
    {
        const auto& Context = _OwnerStateMachine.Get<ck::FFragment_Sm_Context>();
        TransitionEntity.Add<ck::FFragment_Sm_Context>(Context.Get_GameEntityHandle());
    }

    auto TransitionParams = ck::FFragment_SmTransition_Params{InTargetStateClass, _TransitionOrderCounter++};

    TransitionEntity.Add<ck::FFragment_SmTransition_Params>(TransitionParams);
    TransitionEntity.Add<ck::FFragment_SmTransition_Current>();

    auto TransitionEntityTyped = ck::StaticCast<FCk_Handle_SmTransition>(TransitionEntity);
    auto StateHandleTyped = ck::StaticCast<FCk_Handle_SmState>(StateHandle);

    UCk_Utils_StateMachine_UE::RecordOfSmTransitions_Utils::AddIfMissing(StateHandle);
    UCk_Utils_StateMachine_UE::RecordOfSmTransitions_Utils::Request_Connect(
        StateHandle, TransitionEntityTyped, ECk_Record_LabelRequirementPolicy::Optional);

    ck::TUtils_Sm_ParentState::AddOrReplace(TransitionEntity, StateHandleTyped);
    ck::TUtils_Sm_OwningStateMachine::AddOrReplace(TransitionEntity, _OwnerStateMachine);

    return TransitionEntityTyped;
}

auto
    UCk_SmState_EntityScript::
    AddCondition(
        FCk_Handle_SmTransition InTransition,
        TSubclassOf<UCk_SmCondition_EntityScript> InConditionClass)
    -> FCk_Handle_SmCondition
{
    CK_ENSURE_IF_NOT(ck::IsValid(InTransition),
        TEXT("Invalid transition handle in AddCondition"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InConditionClass),
        TEXT("Invalid condition class in AddCondition"))
    { return {}; }

    auto ConditionEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InTransition);

    UCk_Utils_Handle_UE::Set_DebugName(ConditionEntity, InConditionClass->GetFName());

    if (InTransition.Has<ck::FFragment_Sm_Context>())
    {
        const auto& Context = InTransition.Get<ck::FFragment_Sm_Context>();
        ConditionEntity.Add<ck::FFragment_Sm_Context>(Context.Get_GameEntityHandle());
    }

    const auto* ConditionCDO = GetDefault<UCk_SmCondition_EntityScript>(InConditionClass);

    auto ConditionCurrent = ck::FFragment_SmCondition_Current{};
    if (ck::IsValid(ConditionCDO))
    {
        ConditionCurrent.Set_ResetBehavior(ConditionCDO->Get_ResetBehavior());

        if (ConditionCDO->Get_ConditionMode() == ECk_SmConditionMode::Polled)
        {
            ConditionEntity.Add<ck::FTag_SmCondition_Polled>();
        }
        else
        {
            ConditionEntity.Add<ck::FTag_SmCondition_EventDriven>();
        }

        if (ConditionCDO->Get_ResetBehavior() == ECk_SmConditionResetBehavior::ResetEveryFrame)
        {
            ConditionEntity.Add<ck::FTag_SmCondition_ResetsEveryFrame>();
        }
    }

    ConditionEntity.Add<ck::FFragment_SmCondition_Current>(ConditionCurrent);

    auto ConditionEntityTyped = ck::StaticCast<FCk_Handle_SmCondition>(ConditionEntity);

    UCk_Utils_StateMachine_UE::RecordOfSmConditions_Utils::AddIfMissing(TransitionHandle);
    UCk_Utils_StateMachine_UE::RecordOfSmConditions_Utils::Request_Connect(
        TransitionHandle, ConditionEntityTyped, ECk_Record_LabelRequirementPolicy::Optional);

    ck::TUtils_Sm_ParentTransition::AddOrReplace(ConditionEntity, InTransition);

    if (ck::TUtils_Sm_OwningStateMachine::Has(TransitionHandle))
    {
        const auto OwningSm = ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(TransitionHandle);
        ck::TUtils_Sm_OwningStateMachine::AddOrReplace(ConditionEntity, OwningSm);
    }

    UCk_Utils_EntityScript_UE::Add(
        ConditionEntity,
        InConditionClass,
        FInstancedStruct{});

    return ConditionEntityTyped;
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
