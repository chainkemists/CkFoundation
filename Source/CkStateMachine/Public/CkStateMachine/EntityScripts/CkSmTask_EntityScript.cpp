#include "CkSmTask_EntityScript.h"

#include "CkStateMachine/CkStateMachine_Fragment.h"
#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmTask_EntityScript::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    return Super::Construct(InHandle, InSpawnParams);
}

auto
    UCk_SmTask_EntityScript::
    BeginPlay()
    -> void
{
    Super::BeginPlay();
}

auto
    UCk_SmTask_EntityScript::
    EndPlay()
    -> void
{
    Super::EndPlay();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmTask_EntityScript::
    Tick(
        float InDeltaSeconds)
    -> ECk_SmTaskResult
{
    return DoTick(DoGet_ScriptEntity(), InDeltaSeconds);
}

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_SmTask_EntityScript::
    Mark_Result(
        ECk_SmTaskResult InResult)
{
    auto ScriptEntity = DoGet_ScriptEntity();

    if (NOT ScriptEntity.Has<ck::FFragment_SmTask_Current>())
    { return; }

    auto& Current = ScriptEntity.Get<ck::FFragment_SmTask_Current>();
    const auto PrevResult = Current.Get_LastResult();
    Current._LastResult = InResult;

    if (PrevResult == ECk_SmTaskResult::Running && InResult != ECk_SmTaskResult::Running)
    {
        ScriptEntity.Try_Add<ck::FTag_SmTask_ResultDirty>();
    }
}

FCk_Handle_StateMachine
    UCk_SmTask_EntityScript::
    Get_OwningStateMachine() const
{
    auto ScriptEntity = DoGet_ScriptEntity();

    if (NOT ck::TUtils_Sm_OwningStateMachine::Has(ScriptEntity))
    { return {}; }

    return ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(ScriptEntity);
}

auto
    UCk_SmTask_EntityScript::
    DoGet_GameEntity() const
    -> FCk_Handle
{
    auto TaskHandle = DoGet_ScriptEntity();

    if (TaskHandle.Has<ck::FFragment_Sm_Context>())
    {
        const auto& Context = TaskHandle.Get<ck::FFragment_Sm_Context>();
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
    UCk_SmTask_EntityScript::
    Get_TaskTag() const
    -> FGameplayTag
{
    return ck_state_machine_entity_script::ComputeTagFromClassName(
        GetClass()->GetName(),
        ck::Format_UE(TEXT("Auto-generated task tag for {}"), GetClass()));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmTask_EntityScript::
    Get_TaskTagForClass(
        TSubclassOf<UCk_SmTask_EntityScript> InClass)
    -> FGameplayTag
{
    CK_ENSURE_IF_NOT(ck::IsValid(InClass),
        TEXT("Invalid task class in Get_TaskTagForClass"))
    { return {}; }

    return ck_state_machine_entity_script::ComputeTagFromClassName(
        InClass->GetName(),
        ck::Format_UE(TEXT("Auto-generated task tag for {}"), *InClass->GetName()));
}

// --------------------------------------------------------------------------------------------------------------------
