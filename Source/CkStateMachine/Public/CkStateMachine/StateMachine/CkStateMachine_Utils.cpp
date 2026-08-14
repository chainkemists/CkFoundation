#include "CkStateMachine_Utils.h"

#include "CkStateMachine/Net/CkStateMachine_NetContextUtils.h"
#include "CkStateMachine/Net/CkStateMachineRelay_Subsystem.h"

#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/State/CkSmState_Utils.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/Task/CkSmTask_Fragment.h"
#include "CkStateMachine/Debug/CkStateMachine_Debug_Fragment.h"
#include "CkStateMachine/Debug/CkStateMachine_Debug_Utils.h"

#if CK_BUILD_SM_GRAPH_WALK
#include "CkStateMachine/Debug/CkStateMachine_Debug_GraphWalk_Fragment.h"
#endif

#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkDynamic/CkDynamic_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.inl.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_StateMachine_UE, FCk_Handle_StateMachine, ck::FFragment_Sm_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    Add(
        FCk_Handle& InOwner,
        const FCk_Fragment_StateMachine_ParamsData& InParams)
    -> FCk_Handle_StateMachine
{
    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_InitialStateClass()),
        TEXT("Invalid initial state class when creating StateMachine"))
    { return {}; }

    UCk_Utils_Handle_UE::Set_DebugName(InOwner,
        *ck::Format_UE(TEXT("State Machine [{}]"), InParams.Get_InitialStateClass()->GetFName()),
        ECk_Override::DoNotOverride);

    InOwner.Add<ck::FTag_Sm_RequiresSetup>();
    InOwner.Add<ck::FFragment_Sm_Params>(InParams);
    InOwner.Add<ck::FFragment_Sm_Current>();

    auto StateMachine = Cast(InOwner);
    UCk_Utils_StateMachineDebug_UE::BeginDebuggerCapture(StateMachine);

    return Cast(InOwner);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    Create(
        FCk_Handle& InOwner,
        const FCk_Fragment_StateMachine_ParamsData& InParams)
    -> FCk_Handle_StateMachine
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    return Add(NewEntity, InParams);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    Request_Start(
        FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_StateMachine
{
    return DoAddRequest(InStateMachine, FCk_Request_Sm_Start{}, InDelegate);
}

auto
    UCk_Utils_StateMachine_UE::
    Request_Stop(
        FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_StateMachine
{
    return DoAddRequest(InStateMachine, FCk_Request_Sm_Stop{}, InDelegate);
}

auto
    UCk_Utils_StateMachine_UE::
    Request_Pause(
        FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_StateMachine
{
    return DoAddRequest(InStateMachine, FCk_Request_Sm_Pause{}, InDelegate);
}

auto
    UCk_Utils_StateMachine_UE::
    Request_Resume(
        FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_StateMachine
{
    return DoAddRequest(InStateMachine, FCk_Request_Sm_Resume{}, InDelegate);
}

auto
    UCk_Utils_StateMachine_UE::
    Get_HasReplayTransitionsInFlight(
        const FCk_Handle& InEntity)
    -> bool
{
    if (InEntity.Has<ck::FFragment_Sm_ReplayQueue>()
        && NOT InEntity.Get<ck::FFragment_Sm_ReplayQueue>().Get_Queue().IsEmpty())
    { return true; }

    return InEntity.Has<ck::FFragment_Sm_PendingTransition>();
}

auto
    UCk_Utils_StateMachine_UE::
    Request_TryDiscardPendingTransition(
        FCk_Handle& InEntity)
    -> void
{
    if (NOT InEntity.Has<ck::FFragment_Sm_PendingTransition>())
    { return; }

    const auto PreviousStateHandle =
        InEntity.Get<ck::FFragment_Sm_PendingTransition>().Get_PreviousStateHandle();

    if (ck::IsValid(PreviousStateHandle))
    {
        auto PreviousToDestroy = FCk_Handle{PreviousStateHandle};
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(PreviousToDestroy);
    }

    InEntity.Try_Remove<ck::FFragment_Sm_PendingTransition>();
}

auto
    UCk_Utils_StateMachine_UE::
    Request_Transition(
        FCk_Handle_StateMachine& InStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_StateMachine
{
    const auto TargetStateClassIsValid = ck::IsValid(InTargetStateClass);

    CK_ENSURE_IF_NOT(TargetStateClassIsValid,
        TEXT("Request_Transition on [{}] with an invalid target state class — request dropped"),
        InStateMachine)
    {}
    if (NOT TargetStateClassIsValid)
    {
        InDelegate.ExecuteIfBound(InStateMachine, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InStateMachine;
    }

    InStateMachine.AddOrGet<ck::FTag_Sm_TransitionQueued>();
    return DoAddRequest(InStateMachine, FCk_Request_Sm_Transition{InTargetStateClass}, InDelegate);
}

auto
    UCk_Utils_StateMachine_UE::
    Request_ExitStateMachine(
        FCk_Handle_StateMachine& InStateMachine)
    -> FCk_Handle_StateMachine
{
    if (ck::Is_NOT_Valid(InStateMachine))
    { return InStateMachine; }

    auto& Current = InStateMachine.Get<ck::FFragment_Sm_Current>();
    if (ck::IsValid(Current.Get_CurrentStateHandle()))
    {
        auto StateHandle = Current._CurrentStateHandle;
        Current._CurrentStateHandle = {};
        Current._CurrentStateClass = nullptr;
        UCk_Utils_SmState_UE::Request_Exit(StateHandle);
    }

    return InStateMachine;
}

auto
    UCk_Utils_StateMachine_UE::
    Request_AddOverrideState(
        FCk_Handle_StateMachine& InStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InOverrideStateClass,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_StateMachine
{
    return DoAddRequest(InStateMachine, FCk_Request_Sm_AddOverrideState{InOverrideStateClass}, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    Get_RunStatus(
        const FCk_Handle_StateMachine& InStateMachine)
    -> ECk_SmRunStatus
{
    return InStateMachine.Get<ck::FFragment_Sm_Current>().Get_RunStatus();
}

auto
    UCk_Utils_StateMachine_UE::
    Get_CurrentStateClass(
        const FCk_Handle_StateMachine& InStateMachine)
    -> TSubclassOf<UCk_SmState_EntityScript>
{
    return InStateMachine.Get<ck::FFragment_Sm_Current>().Get_CurrentStateClass();
}

auto
    UCk_Utils_StateMachine_UE::
    Get_CurrentStateHandle(
        const FCk_Handle_StateMachine& InStateMachine)
    -> FCk_Handle_SmState
{
    return InStateMachine.Get<ck::FFragment_Sm_Current>().Get_CurrentStateHandle();
}

auto
    UCk_Utils_StateMachine_UE::
    IsInState(
        const FCk_Handle_StateMachine& InStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InStateClass)
    -> bool
{
    return InStateMachine.Get<ck::FFragment_Sm_Current>().Get_CurrentStateClass() == InStateClass;
}

auto
    UCk_Utils_StateMachine_UE::
    Get_NetContext(
        const FCk_Handle_StateMachine& InStateMachine)
    -> ECk_Sm_NetContext
{
    return ck::statemachine::ComputeNetContext(InStateMachine);
}

auto
    UCk_Utils_StateMachine_UE::
    Get_Replication(
        const FCk_Handle_StateMachine& InStateMachine)
    -> ECk_Replication
{
    return InStateMachine.Get<ck::FFragment_Sm_Params>().Get_Replication();
}

auto
    UCk_Utils_StateMachine_UE::
    Get_AuthorityModel(
        const FCk_Handle_StateMachine& InStateMachine)
    -> ECk_Sm_AuthorityModel
{
    return InStateMachine.Get<ck::FFragment_Sm_Params>().Get_AuthorityModel();
}

auto
    UCk_Utils_StateMachine_UE::
    Get_EffectiveAuthorityModel(
        const FCk_Handle_StateMachine& InStateMachine)
    -> ECk_Sm_AuthorityModel
{
    // Sub-SMs carry a resolved-once concrete model (never AutoDetect) because they cannot resolve
    // their owning pawn live — see FFragment_Sm_NetIdentity.
    if (InStateMachine.Has<ck::FFragment_Sm_NetIdentity>())
    { return InStateMachine.Get<ck::FFragment_Sm_NetIdentity>().Get_EffectiveAuthority(); }

    const auto Authored = InStateMachine.Get<ck::FFragment_Sm_Params>().Get_AuthorityModel();

    if (Authored != ECk_Sm_AuthorityModel::AutoDetect)
    { return Authored; }

    // AutoDetect: a player-controlled pawn host means the owning client drives; bots, non-pawn, or
    // non-actor-bridged hosts fall back to ServerAuthoritative. Resolves identically on every machine
    // because IsPlayerControlled reads the replicated PlayerState, not a per-machine "is it mine".
    const auto IsPlayerControlled = UCk_Utils_Net_UE::Get_IsEntityPlayerControlled(InStateMachine)
        == ECk_Utils_Net_IsPlayerControlled_Result::IsPlayerControlled;

    return IsPlayerControlled
        ? ECk_Sm_AuthorityModel::OwningClientAuthoritative
        : ECk_Sm_AuthorityModel::ServerAuthoritative;
}

auto
    UCk_Utils_StateMachine_UE::
    Get_ReplicationModel(
        const FCk_Handle_StateMachine& InStateMachine)
    -> ECk_Sm_ReplicationModel
{
    return InStateMachine.Get<ck::FFragment_Sm_Params>().Get_ReplicationModel();
}

// --------------------------------------------------------------------------------------------------------------------
// SUB-SM TRANSITION RELAY (identity + resolution)
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    Get_RootStateMachine(
        const FCk_Handle_StateMachine& InStateMachine)
    -> FCk_Handle_StateMachine
{
    auto Current = InStateMachine;
    while (ck::TUtils_Sm_OwningStateMachine::Has(Current))
    {
        const auto Owner = ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(Current);
        if (ck::Is_NOT_Valid(Owner))
        { break; }
        Current = Owner;
    }
    return Current;
}

auto
    UCk_Utils_StateMachine_UE::
    Get_SubSmParentHierarchy(
        const FCk_Handle_StateMachine& InStateMachine)
    -> TArray<FGameplayTag>
{
    if (InStateMachine.Has<ck::FFragment_Sm_ParentHierarchy>())
    { return InStateMachine.Get<ck::FFragment_Sm_ParentHierarchy>().Get_ParentHierarchy(); }
    return {};
}

namespace ck_state_machine_utils
{
    // Depth-first walk of the ACTIVE state tree: current state -> hosted sub-SMs -> recurse. The
    // hierarchy path is a unique deterministic address, so the first exact match is the one.
    auto
    DoFind_ActiveSubSm(
        const FCk_Handle_StateMachine& InSm,
        const TArray<FGameplayTag>& InTarget)
    -> FCk_Handle_StateMachine
    {
        const auto CurrentState = UCk_Utils_StateMachine_UE::Get_CurrentStateHandle(InSm);
        if (ck::Is_NOT_Valid(CurrentState))
        { return {}; }

        auto Result = FCk_Handle_StateMachine{};

        UCk_Utils_StateMachine_UE::RecordOfSmTasks_Utils::ForEach_ValidEntry(CurrentState,
        [&](FCk_Handle_SmTask InTask) -> ECk_Record_ForEachIterationResult
        {
            if (NOT InTask.Has<ck::FFragment_SmTask_SubStateMachine>())
            { return ECk_Record_ForEachIterationResult::Continue; }

            const auto SubSm = InTask.Get<ck::FFragment_SmTask_SubStateMachine>().Get_SubStateMachineHandle();
            if (ck::Is_NOT_Valid(SubSm))
            { return ECk_Record_ForEachIterationResult::Continue; }

            if (UCk_Utils_StateMachine_UE::Get_SubSmParentHierarchy(SubSm) == InTarget)
            {
                Result = SubSm;
                return ECk_Record_ForEachIterationResult::Break;
            }

            if (const auto Nested = DoFind_ActiveSubSm(SubSm, InTarget);
                ck::IsValid(Nested))
            {
                Result = Nested;
                return ECk_Record_ForEachIterationResult::Break;
            }

            return ECk_Record_ForEachIterationResult::Continue;
        });

        return Result;
    }
}

auto
    UCk_Utils_StateMachine_UE::
    TryFind_ActiveSubSm_ByParentHierarchy(
        const FCk_Handle_StateMachine& InRoot,
        const TArray<FGameplayTag>& InParentHierarchy)
    -> FCk_Handle_StateMachine
{
    if (ck::Is_NOT_Valid(InRoot) || InParentHierarchy.IsEmpty())
    { return {}; }

    return ck_state_machine_utils::DoFind_ActiveSubSm(InRoot, InParentHierarchy);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    BindTo_OnStateChanged(
        FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Sm_OnStateChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_StateMachine
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnSmStateChanged, InStateMachine, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InStateMachine;
}

auto
    UCk_Utils_StateMachine_UE::
    UnbindFrom_OnStateChanged(
        FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Sm_OnStateChanged& InDelegate)
    -> FCk_Handle_StateMachine
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnSmStateChanged, InStateMachine, InDelegate);
    return InStateMachine;
}

auto
    UCk_Utils_StateMachine_UE::
    BindTo_OnStarted(
        FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Sm_OnStarted& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_StateMachine
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnSmStarted, InStateMachine, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InStateMachine;
}

auto
    UCk_Utils_StateMachine_UE::
    UnbindFrom_OnStarted(
        FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Sm_OnStarted& InDelegate)
    -> FCk_Handle_StateMachine
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnSmStarted, InStateMachine, InDelegate);
    return InStateMachine;
}

auto
    UCk_Utils_StateMachine_UE::
    BindTo_OnStopped(
        FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Sm_OnStopped& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_StateMachine
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnSmStopped, InStateMachine, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InStateMachine;
}

auto
    UCk_Utils_StateMachine_UE::
    UnbindFrom_OnStopped(
        FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Sm_OnStopped& InDelegate)
    -> FCk_Handle_StateMachine
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnSmStopped, InStateMachine, InDelegate);
    return InStateMachine;
}

auto
    UCk_Utils_StateMachine_UE::
    BindTo_OnSmTaskFinished(
        FCk_Handle_SmTask& InTask,
        const FCk_Delegate_SmTask_OnFinished& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_SmTask
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnSmTaskFinished, InTask, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InTask;
}

auto
    UCk_Utils_StateMachine_UE::
    UnbindFrom_OnSmTaskFinished(
        FCk_Handle_SmTask& InTask,
        const FCk_Delegate_SmTask_OnFinished& InDelegate)
    -> FCk_Handle_SmTask
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnSmTaskFinished, InTask, InDelegate);
    return InTask;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    DoAddRequest(
        FCk_Handle_StateMachine& InStateMachine,
        const auto& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_StateMachine
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    auto& Requests = InStateMachine.AddOrGet<ck::FFragment_Sm_Requests>();
    Requests._Requests.Add(InRequest);

    return InStateMachine;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    TryCheckEntryBreakpoint(
        FCk_Handle_StateMachine& InStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InStateClass)
    -> void
{
#if !UE_BUILD_SHIPPING
    if (NOT InStateMachine.Has<ck::FFragment_Sm_Breakpoints>())
    { return; }

    if (NOT InStateMachine.Get<ck::FFragment_Sm_Breakpoints>().Get_EntryBreakpoints().Contains(InStateClass))
    { return; }

    auto& Hit = InStateMachine.AddOrGet<ck::FFragment_Sm_Debug_BreakpointHit>();
    Hit.Kind = ck::ECk_SmDebug_BreakpointHitKind::StateEntry;
    Hit.StateClass = InStateClass;
    Hit.SourceStateClass = nullptr;
    Hit.TargetStateClass = nullptr;
    Hit.Description = TEXT("Entry: ") + UCk_Utils_Object_UE::Get_CleanClassName(InStateClass);
    Hit.RealTimeSeconds = FPlatformTime::Seconds();
    UCk_Utils_EditorOnly_UE::Request_DebugPauseExecution();
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    TryCheckExitBreakpoint(
        FCk_Handle_StateMachine& InStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InStateClass)
    -> void
{
#if !UE_BUILD_SHIPPING
    if (NOT InStateMachine.Has<ck::FFragment_Sm_Breakpoints>())
    { return; }

    if (NOT InStateMachine.Get<ck::FFragment_Sm_Breakpoints>().Get_ExitBreakpoints().Contains(InStateClass))
    { return; }

    auto& Hit = InStateMachine.AddOrGet<ck::FFragment_Sm_Debug_BreakpointHit>();
    Hit.Kind = ck::ECk_SmDebug_BreakpointHitKind::StateExit;
    Hit.StateClass = InStateClass;
    Hit.SourceStateClass = nullptr;
    Hit.TargetStateClass = nullptr;
    Hit.Description = TEXT("Exit: ") + UCk_Utils_Object_UE::Get_CleanClassName(InStateClass);
    Hit.RealTimeSeconds = FPlatformTime::Seconds();
    UCk_Utils_EditorOnly_UE::Request_DebugPauseExecution();
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    Acquire_RelayChannel(
        const FCk_Handle_StateMachine& InStateMachine)
    -> FCk_ActorRelay_ChannelResult
{
    CK_ENSURE_IF_NOT(ck::IsValid(InStateMachine),
        TEXT("Acquire_RelayChannel called with invalid SM handle"))
    { return {}; }

    const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InStateMachine);
    if (ck::Is_NOT_Valid(World, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    auto* Subsystem = World->GetSubsystem<UCk_StateMachineRelay_Subsystem_UE>();
    if (ck::Is_NOT_Valid(Subsystem, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    // The SM relay is a CLIENT→SERVER push: it must use the channel owned by the SM's OWNING player —
    // on that client the AutonomousProxy with a valid NetConnection, the only actor a client may
    // invoke Server_* RPCs on. Any other pooled channel is a SimulatedProxy there and UE silently
    // drops the RPC. Sync-or-null: an unresolved result means "retry next pump".
    auto* OwningActor      = UCk_Utils_OwningActor_UE::TryGet_EntityOwningActor_Recursive(InStateMachine);
    auto* OwningPawn       = ::Cast<APawn>(OwningActor); // ::Cast to avoid the class's own typesafe-handle Cast
    auto* OwningPlayerState = OwningPawn != nullptr ? OwningPawn->GetPlayerState() : nullptr;

    if (OwningPlayerState == nullptr)
    {
        // PlayerState not resolved yet — retry next pump rather than fall back to a non-owned channel
        // the client can't Server_* RPC on.
        return {};
    }

    auto Pending = Subsystem->Request_AcquireChannel_ForPlayer(OwningPlayerState);
    return Subsystem->Try_ResolvePending(Pending);
}

// --------------------------------------------------------------------------------------------------------------------
