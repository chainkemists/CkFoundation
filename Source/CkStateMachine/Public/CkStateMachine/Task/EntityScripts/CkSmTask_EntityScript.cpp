#include "CkSmTask_EntityScript.h"

#include "CkCore/Time/CkTime.h"
#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"
#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/Debug/CkStateMachine_Debug_GraphWalk_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/Task/CkSmTask_Fragment.h"
#include "CkStateMachine/Task/CkSmTask_Utils.h"
#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"

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

    // See UCk_SmState_EntityScript::BeginPlay for rationale. Skipping EnterTask here
    // is the critical guard — tasks like UBb_Hfsm_Task_TerminateOwningSm issue
    // Request_Stop on the owning SM during DoEnterTask, which would kill the real
    // sub-SM when the graph-walk temp-entity reaches the terminal state.
    if (_AssociatedEntity.Has<ck::FTag_Sm_Debug_GraphWalkEntity>())
    { return; }

    auto Self = UCk_Utils_SmTask_UE::CastChecked(_AssociatedEntity);
    EnterTask(Self);
}

auto
    UCk_SmTask_EntityScript::
    EndPlay()
    -> void
{
    // Fallback for cascade-destroyed tasks whose FProcessor_SmTask_Exit never runs
    // (e.g. sub-SM tasks destroyed via parent state cascade). Dedup'd via FTag_SmTask_Active
    // inside ExitTask — if the processor already handled this task, the tag is gone and
    // ExitTask is a no-op.
    auto Self = UCk_Utils_SmTask_UE::CastChecked(_AssociatedEntity);
    if (ck::IsValid(Self))
    {
        ExitTask(Self);
    }
    Super::EndPlay();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmTask_EntityScript::
    EnterTask(
        FCk_Handle_SmTask InHandle)
    -> void
{
    ck::sm::VeryVerbose(TEXT("[SM Lifecycle] EnterTask [{}] on entity [{}]"), GetClass(), InHandle);

    InHandle.AddOrGet<ck::FTag_SmTask_Active>();
    DoEnterTask(InHandle);
}

auto
    UCk_SmTask_EntityScript::
    ExitTask(
        FCk_Handle_SmTask InHandle)
    -> void
{
    if (NOT InHandle.Has<ck::FTag_SmTask_Active>())
    { return; }

    ck::sm::VeryVerbose(TEXT("[SM Lifecycle] ExitTask [{}] on entity [{}]"), GetClass(), InHandle);

    InHandle.Try_Remove<ck::FTag_SmTask_Active>();
    DoExitTask(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmTask_EntityScript::
    Tick(
        FCk_Handle_SmTask InHandle,
        FCk_Time InDeltaT)
    -> ECk_SmTaskResult
{
    return DoTick(InHandle, InDeltaT);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmTask_EntityScript::
    Mark_Result(
        ECk_SmTaskResult InResult)
    -> void
{
    auto ScriptEntity = DoGet_ScriptEntity();

    if (NOT ScriptEntity.Has<ck::FFragment_SmTask_Current>())
    { return; }

    auto& Current = ScriptEntity.Get<ck::FFragment_SmTask_Current>();
    const auto PrevResult = Current.Get_LastResult();
    Current._LastResult = InResult;

    if (PrevResult == ECk_SmTaskResult::Running && InResult != ECk_SmTaskResult::Running)
    {
        ScriptEntity.AddOrGet<ck::FTag_SmTask_ResultDirty>();
    }
}

auto
    UCk_SmTask_EntityScript::
    Get_OwningStateMachine() const
    -> FCk_Handle_StateMachine
{
    const auto ScriptEntity = DoGet_ScriptEntity();

    if (NOT ck::TUtils_Sm_OwningStateMachine::Has(ScriptEntity))
    { return {}; }

    return ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(ScriptEntity);
}

auto
    UCk_SmTask_EntityScript::
    Get_StateMachineContext() const
    -> FCk_Handle
{
    return UCk_Utils_ContextOwner_UE::Get_ContextOwner(DoGet_ScriptEntity());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmTask_EntityScript::
    Get_TaskTag() const
    -> FGameplayTag
{
    return UCk_Utils_Object_UE::Get_TagFromClassName(
        GetClass(),
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

    return UCk_Utils_Object_UE::Get_TagFromClassName(
        InClass,
        ck::Format_UE(TEXT("Auto-generated task tag for {}"), *InClass->GetName()));
}

// --------------------------------------------------------------------------------------------------------------------
