#include "CkStateMachine_Utils.h"

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

    auto SmEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);

    UCk_Utils_Handle_UE::Set_DebugName(SmEntity,
        *ck::Format_UE(TEXT("State Machine [{}]"), InInitialStateClass->GetFName()));

    auto Params = FCk_Fragment_StateMachine_ParamsData{InInitialStateClass};
    Params.Set_AutoStart(InAutoStart);

    SmEntity.Add<ck::FTag_Sm_RequiresSetup>();
    SmEntity.Add<ck::FFragment_Sm_Params>(Params);
    SmEntity.Add<ck::FFragment_Sm_Current>();

#if CK_BUILD_SM_GRAPH_WALK
    SmEntity.Add<ck::FTag_Sm_Debug_RequiresGraphWalk>();
#endif

    return Cast(SmEntity);
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
    return DoAddRequest(InStateMachine, FCk_Request_Sm_Transition{InTargetStateClass});
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
