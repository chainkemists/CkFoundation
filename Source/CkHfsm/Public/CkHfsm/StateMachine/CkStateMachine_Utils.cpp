#include "CkHfsm/StateMachine/CkStateMachine_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcsExt/ContextOwner/CkContextOwner_Utils.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkHfsm/State/CkState_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    struct RecordOfStates_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfStates> {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_StateMachine_ParamsData& InParams)
    -> FCk_Handle_StateMachine
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, [&](FCk_Handle InNew)
    {
        InNew.Add<ck::FFragment_StateMachine_Params>(InParams);
        InNew.Add<ck::FFragment_StateMachine_Current>();
        InNew.Add<ck::FTag_StateMachine_Setup>();

        // Initialize record for states
        RecordOfStates_Utils::AddIfMissing(InNew, ECk_Record_EntryHandlingPolicy::Default);
    });

    return CastChecked(NewEntity);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_StateMachine_UE, FCk_Handle_StateMachine,
    ck::FFragment_StateMachine_Params, ck::FFragment_StateMachine_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    AddState(
        FCk_Handle_StateMachine& InStateMachineHandle,
        const FCk_Fragment_State_ParamsData& InStateParams)
    -> FCk_Handle_State
{
    // Convert typesafe handle to generic handle
    FCk_Handle& GenericHandle = InStateMachineHandle;
    return UCk_Utils_State_UE::Add(GenericHandle, InStateParams);
}

auto
    UCk_Utils_StateMachine_UE::
    GetAllStates(
        const FCk_Handle_StateMachine& InStateMachineHandle)
    -> TArray<FCk_Handle_State>
{
    auto States = TArray<FCk_Handle_State>{};
    RecordOfStates_Utils::ForEach_ValidEntry(InStateMachineHandle, [&](FCk_Handle_State InState)
    {
        States.Add(InState);
    });
    return States;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    Request_Start(
        FCk_Handle_StateMachine& InHandle)
    -> FCk_Handle_StateMachine
{
    InHandle.AddOrGet<ck::FTag_StateMachine_Enter>();

    return InHandle;
}

auto
    UCk_Utils_StateMachine_UE::
    Request_Stop(
        FCk_Handle_StateMachine& InHandle)
    -> FCk_Handle_StateMachine
{
    InHandle.AddOrGet<ck::FTag_StateMachine_Exit>();

    return InHandle;
}

auto
    UCk_Utils_StateMachine_UE::
    Request_Transition(
        FCk_Handle_StateMachine& InHandle)
    -> FCk_Handle_StateMachine
{
    InHandle.AddOrGet<ck::FTag_StateMachine_Transition>();

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    Get_CurrentState(
        const FCk_Handle_StateMachine& InHandle)
    -> FCk_Handle
{
    return InHandle.Get<ck::FFragment_StateMachine_Current>().Get_CurrentState();
}

auto
    UCk_Utils_StateMachine_UE::
    Get_PreviousState(
        const FCk_Handle_StateMachine& InHandle)
    -> FCk_Handle
{
    return InHandle.Get<ck::FFragment_StateMachine_Current>().Get_PreviousState();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    BindTo_OnStart(
        FCk_Handle_StateMachine& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_StateMachine& InDelegate)
    -> FCk_Handle_StateMachine
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnStateMachineStart, InHandle, InDelegate, InBindingPolicy);
    return InHandle;
}

auto
    UCk_Utils_StateMachine_UE::
    BindTo_OnStop(
        FCk_Handle_StateMachine& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_StateMachine& InDelegate)
    -> FCk_Handle_StateMachine
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnStateMachineStop, InHandle, InDelegate, InBindingPolicy);
    return InHandle;
}

auto
    UCk_Utils_StateMachine_UE::
    BindTo_OnTransition(
        FCk_Handle_StateMachine& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_StateMachine_Transition& InDelegate)
    -> FCk_Handle_StateMachine
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnStateMachineTransition, InHandle, InDelegate, InBindingPolicy);
    return InHandle;
}

auto
    UCk_Utils_StateMachine_UE::
    UnbindFrom_OnStart(
        FCk_Handle_StateMachine& InHandle,
        const FCk_Delegate_StateMachine& InDelegate)
    -> FCk_Handle_StateMachine
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnStateMachineStart, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_StateMachine_UE::
    UnbindFrom_OnStop(
        FCk_Handle_StateMachine& InHandle,
        const FCk_Delegate_StateMachine& InDelegate)
    -> FCk_Handle_StateMachine
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnStateMachineStop, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_StateMachine_UE::
    UnbindFrom_OnTransition(
        FCk_Handle_StateMachine& InHandle,
        const FCk_Delegate_StateMachine_Transition& InDelegate)
    -> FCk_Handle_StateMachine
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnStateMachineTransition, InHandle, InDelegate);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------