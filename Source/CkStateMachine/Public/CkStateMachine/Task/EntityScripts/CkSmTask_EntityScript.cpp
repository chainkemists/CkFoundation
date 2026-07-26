#include "CkSmTask_EntityScript.h"

#include "CkCore/Time/CkTime.h"
#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"
#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/Net/CkStateMachine_NetContextUtils.h"
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

    // Skipping EnterTask on the graph-walk temp entity is the critical guard: tasks like
    // UBb_Hfsm_Task_TerminateOwningSm issue Request_Stop on the owning SM during DoEnterTask, which
    // would kill the real sub-SM when the temp entity reaches the terminal state.
    if (_AssociatedEntity.Has<ck::FTag_Sm_Debug_GraphWalkEntity>())
    { return; }

    // A snapshot-restored SM-graph entity keeps its EntityScript but not its SmTask fragment (the
    // redrive rebuilds the real graph fresh), so this BeginPlay can run on a husk.
    if (NOT UCk_Utils_SmTask_UE::Has(_AssociatedEntity))
    { return; }

    auto Self = UCk_Utils_SmTask_UE::CastChecked(_AssociatedEntity);
    const auto SmHandle = UCk_Utils_SmTask_UE::Get_OwningStateMachine(Self);
    const auto NetContext = ck::statemachine::ComputeNetContext(SmHandle);
    EnterTask(Self, NetContext);
}

auto
    UCk_SmTask_EntityScript::
    EndPlay()
    -> void
{
    // Fallback for cascade-destroyed tasks whose FProcessor_SmTask_Exit never runs; dedup'd via
    // FTag_SmTask_Active inside ExitTask. Has-guard rather than CastChecked because a
    // snapshot-restored SM-graph entity keeps its EntityScript but not its SmTask fragment.
    if (UCk_Utils_SmTask_UE::Has(_AssociatedEntity))
    {
        auto Self = UCk_Utils_SmTask_UE::CastChecked(_AssociatedEntity);
        const auto SmHandle = UCk_Utils_SmTask_UE::Get_OwningStateMachine(Self);
        const auto NetContext = ck::statemachine::ComputeNetContext(SmHandle);
        ExitTask(Self, NetContext);
    }
    Super::EndPlay();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmTask_EntityScript::
    Get_EffectiveReplication() const
    -> ECk_Replication
{
    // The state sub-graph is never independently replicated; clients rebuild it via the replay path.
    return ECk_Replication::DoesNotReplicate;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmTask_EntityScript::
    EnterTask(
        FCk_Handle_SmTask InHandle,
        ECk_Sm_NetContext InNetContext)
    -> void
{
    ck::sm::VeryVerbose(TEXT("[SM Lifecycle] EnterTask [{}] on entity [{}]"), GetClass(), InHandle);

    InHandle.AddOrGet<ck::FTag_SmTask_Active>();
    DoEnterTask(InHandle, InNetContext);
}

auto
    UCk_SmTask_EntityScript::
    ExitTask(
        FCk_Handle_SmTask InHandle,
        ECk_Sm_NetContext InNetContext)
    -> void
{
    if (NOT InHandle.Has<ck::FTag_SmTask_Active>())
    { return; }

    ck::sm::VeryVerbose(TEXT("[SM Lifecycle] ExitTask [{}] on entity [{}]"), GetClass(), InHandle);

    InHandle.Try_Remove<ck::FTag_SmTask_Active>();
    DoExitTask(InHandle, InNetContext);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmTask_EntityScript::
    Tick(
        FCk_Handle_SmTask InHandle,
        FCk_Time InDeltaT,
        ECk_Sm_NetContext InNetContext)
    -> ECk_SmTaskResult
{
    return DoTick(InHandle, InDeltaT, InNetContext);
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

    // Single write path: the util owns the unbroadcast-terminal-result latch and the Running→terminal
    // dirty edge.
    auto TaskHandle = UCk_Utils_SmTask_UE::CastChecked(ScriptEntity);
    UCk_Utils_SmTask_UE::Request_UpdateTaskResult(TaskHandle, InResult);
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
