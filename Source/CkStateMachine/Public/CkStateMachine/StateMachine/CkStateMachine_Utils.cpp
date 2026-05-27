#include "CkStateMachine_Utils.h"

#include "CkStateMachine/Net/CkStateMachine_NetContextUtils.h"
#include "CkStateMachine/Net/CkStateMachineRelay_Subsystem.h"

#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/State/CkSmState_Utils.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/Debug/CkStateMachine_Debug_Fragment.h"

#if CK_BUILD_SM_GRAPH_WALK
#include "CkStateMachine/Debug/CkStateMachine_Debug_GraphWalk_Fragment.h"
#endif

#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkDynamic/CkDynamic_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.inl.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_StateMachine_UE, FCk_Handle_StateMachine, ck::FFragment_Sm_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    Add(
        FCk_Handle& InOwner,
        TSubclassOf<UCk_SmState_EntityScript> InInitialStateClass,
        ECk_SmAutoStart InAutoStart)
    -> FCk_Handle_StateMachine
{
    CK_ENSURE_IF_NOT(ck::IsValid(InInitialStateClass),
        TEXT("Invalid initial state class when creating StateMachine"))
    { return {}; }

    UCk_Utils_Handle_UE::Set_DebugName(InOwner,
        *ck::Format_UE(TEXT("State Machine [{}]"), InInitialStateClass->GetFName()), ECk_Override::DoNotOverride);

    auto Params = FCk_Fragment_StateMachine_ParamsData{InInitialStateClass};
    Params.Set_AutoStart(InAutoStart);

    InOwner.Add<ck::FTag_Sm_RequiresSetup>();
    InOwner.Add<ck::FFragment_Sm_Params>(Params);
    InOwner.Add<ck::FFragment_Sm_Current>();

#if CK_BUILD_SM_GRAPH_WALK
    InOwner.Add<ck::FTag_Sm_Debug_RequiresGraphWalk>();
#endif

    return Cast(InOwner);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    Add_WithParams(
        FCk_Handle& InOwner,
        const FCk_Fragment_StateMachine_ParamsData& InParams)
    -> FCk_Handle_StateMachine
{
    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_InitialStateClass()),
        TEXT("Invalid initial state class when creating StateMachine with params"))
    { return {}; }

    UCk_Utils_Handle_UE::Set_DebugName(InOwner,
        *ck::Format_UE(TEXT("State Machine [{}]"), InParams.Get_InitialStateClass()->GetFName()),
        ECk_Override::DoNotOverride);

    InOwner.Add<ck::FTag_Sm_RequiresSetup>();
    InOwner.Add<ck::FFragment_Sm_Params>(InParams);
    InOwner.Add<ck::FFragment_Sm_Current>();

#if CK_BUILD_SM_GRAPH_WALK
    InOwner.Add<ck::FTag_Sm_Debug_RequiresGraphWalk>();
#endif

    return Cast(InOwner);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    Request_Start(
        FCk_Handle_StateMachine& InStateMachine)
    -> FCk_Handle_StateMachine
{
    return DoAddRequest(InStateMachine, FCk_Request_Sm_Start{});
}

auto
    UCk_Utils_StateMachine_UE::
    Request_Stop(
        FCk_Handle_StateMachine& InStateMachine)
    -> FCk_Handle_StateMachine
{
    return DoAddRequest(InStateMachine, FCk_Request_Sm_Stop{});
}

auto
    UCk_Utils_StateMachine_UE::
    Request_Pause(
        FCk_Handle_StateMachine& InStateMachine)
    -> FCk_Handle_StateMachine
{
    return DoAddRequest(InStateMachine, FCk_Request_Sm_Pause{});
}

auto
    UCk_Utils_StateMachine_UE::
    Request_Resume(
        FCk_Handle_StateMachine& InStateMachine)
    -> FCk_Handle_StateMachine
{
    return DoAddRequest(InStateMachine, FCk_Request_Sm_Resume{});
}

auto
    UCk_Utils_StateMachine_UE::
    Request_Transition(
        FCk_Handle_StateMachine& InStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass)
    -> FCk_Handle_StateMachine
{
    InStateMachine.AddOrGet<ck::FTag_Sm_TransitionQueued>();
    return DoAddRequest(InStateMachine, FCk_Request_Sm_Transition{InTargetStateClass});
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
        TSubclassOf<UCk_SmState_EntityScript> InOverrideStateClass)
    -> FCk_Handle_StateMachine
{
    return DoAddRequest(InStateMachine, FCk_Request_Sm_AddOverrideState{InOverrideStateClass});
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
    Get_ReplicationModel(
        const FCk_Handle_StateMachine& InStateMachine)
    -> ECk_Sm_ReplicationModel
{
    return InStateMachine.Get<ck::FFragment_Sm_Params>().Get_ReplicationModel();
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
        const auto& InRequest)
    -> FCk_Handle_StateMachine
{
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

    auto& [Description, RealTimeSeconds] = InStateMachine.AddOrGet<ck::FFragment_Sm_Debug_BreakpointHit>();
    Description = TEXT("Entry: ") + UCk_Utils_Object_UE::Get_CleanClassName(InStateClass);
    RealTimeSeconds = FPlatformTime::Seconds();
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

    auto& [Description, RealTimeSeconds] = InStateMachine.AddOrGet<ck::FFragment_Sm_Debug_BreakpointHit>();
    Description = TEXT("Exit: ") + UCk_Utils_Object_UE::Get_CleanClassName(InStateClass);
    RealTimeSeconds = FPlatformTime::Seconds();
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

    // Request_AcquireAnyChannel was converted to the Promise/Pending pattern after this
    // wrapper was written (see CkActorRelay's d86703e24 — `convert Request_AcquireChannel to
    // Promise pattern`), so its return type is now `FCk_Handle_PendingActorRelay` rather than
    // a resolved `FCk_ActorRelay_ChannelResult`. The owning-client push processor wraps this
    // in a per-pump retry loop, so the sync-or-null shape is the right fit — Try_ResolvePending
    // does the immediate resolve without subscribing to a deferred ready-signal.
    auto Pending = Subsystem->Request_AcquireAnyChannel();
    return Subsystem->Try_ResolvePending(Pending);
}

// --------------------------------------------------------------------------------------------------------------------
