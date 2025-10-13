#include "CkHfsm/State/CkState_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkHfsm/Service/CkService_Utils.h"
#include "CkHfsm/Transition/CkTransition_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_State_UE::
    Create(
        FCk_Handle_StateMachine& InStateMachine,
        const FCk_Fragment_State_ParamsData& InParams)
    -> FCk_Handle_State
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InStateMachine, [&](FCk_Handle InNew)
    {
        InNew.Add<ck::FFragment_State_Params>(InParams);
        InNew.Add<ck::FFragment_State_Current>();
        InNew.Add<ck::FTag_State_Setup>();
        InNew.Add<ck::FTag_State_IsEventDriven>(); // Assume event-driven by default

        // Initialize records for services and transitions
        RecordOfServices_Utils::AddIfMissing(InNew, ECk_Record_EntryHandlingPolicy::Default);
        RecordOfTransitions_Utils::AddIfMissing(InNew, ECk_Record_EntryHandlingPolicy::Default);
    });

    // Connect to parent state machine's record
    RecordOfStates_Utils::AddIfMissing(InStateMachine, ECk_Record_EntryHandlingPolicy::Default);
    RecordOfStates_Utils::Request_Connect(InStateMachine, CastChecked(NewEntity),
        ECk_Record_LabelRequirementPolicy::Optional);

    return CastChecked(NewEntity);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_State_UE, FCk_Handle_State, ck::FFragment_State_Params, ck::FFragment_State_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_State_UE::
    AddService(
        FCk_Handle_State& InStateHandle)
    -> FCk_Handle_Service
{
    return UCk_Utils_Service_UE::Create(InStateHandle);
}

auto
    UCk_Utils_State_UE::
    GetAllServices(
        const FCk_Handle_State& InStateHandle)
    -> TArray<FCk_Handle_Service>
{
    auto Services = TArray<FCk_Handle_Service>{};
    RecordOfServices_Utils::ForEach_ValidEntry(InStateHandle, [&](FCk_Handle_Service InService)
    {
        Services.Add(InService);
    });
    return Services;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_State_UE::
    AddTransition(
        FCk_Handle_State& InStateHandle,
        const FCk_Fragment_Transition_ParamsData& InTransitionParams)
    -> FCk_Handle_Transition
{
    return UCk_Utils_Transition_UE::Create(InStateHandle, InTransitionParams);
}

auto
    UCk_Utils_State_UE::
    GetAllTransitions(
        const FCk_Handle_State& InStateHandle)
    -> TArray<FCk_Handle_Transition>
{
    auto Transitions = TArray<FCk_Handle_Transition>{};
    RecordOfTransitions_Utils::ForEach_ValidEntry(InStateHandle, [&](FCk_Handle_Transition InTransition)
    {
        Transitions.Add(InTransition);
    });
    return Transitions;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_State_UE::
    Request_Enter(
        FCk_Handle_State& InHandle)
    -> FCk_Handle_State
{
    InHandle.AddOrGet<ck::FTag_State_Enter>();

    return InHandle;
}

auto
    UCk_Utils_State_UE::
    Request_Exit(
        FCk_Handle_State& InHandle)
    -> FCk_Handle_State
{
    InHandle.AddOrGet<ck::FTag_State_Exit>();

    return InHandle;
}

auto
    UCk_Utils_State_UE::
    Request_Evaluate(
        FCk_Handle_State& InHandle)
    -> FCk_Handle_State
{
    if (NOT InHandle.Has<ck::FTag_State_ReadyToTransition>())
    {
        InHandle.AddOrGet<ck::FTag_StateMachine_Evaluate_State>();
    }

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_State_UE::
    Get_Name(
        const FCk_Handle_State& InHandle)
    -> FGameplayTag
{
    return InHandle.Get<ck::FFragment_State_Params>().Get_Name();
}

auto
    UCk_Utils_State_UE::
    Get_IsReadyToTransition(
        const FCk_Handle_State& InHandle)
    -> bool
{
    return InHandle.Has<ck::FTag_State_ReadyToTransition>();
}

auto
    UCk_Utils_State_UE::
    Get_NextState(
        const FCk_Handle_State& InHandle)
    -> FCk_Handle
{
    return InHandle.Get<ck::FFragment_State_Current>().Get_NextState();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_State_UE::
    BindTo_OnEnter(
        FCk_Handle_State& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_State& InDelegate)
    -> FCk_Handle_State
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnStateEnter, InHandle, InDelegate, InBindingPolicy);
    return InHandle;
}

auto
    UCk_Utils_State_UE::
    BindTo_OnExit(
        FCk_Handle_State& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_State& InDelegate)
    -> FCk_Handle_State
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnStateExit, InHandle, InDelegate, InBindingPolicy);
    return InHandle;
}

auto
    UCk_Utils_State_UE::
    BindTo_OnUpdate(
        FCk_Handle_State& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_State& InDelegate)
    -> FCk_Handle_State
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnStateUpdate, InHandle, InDelegate, InBindingPolicy);
    return InHandle;
}

auto
    UCk_Utils_State_UE::
    UnbindFrom_OnEnter(
        FCk_Handle_State& InHandle,
        const FCk_Delegate_State& InDelegate)
    -> FCk_Handle_State
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnStateEnter, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_State_UE::
    UnbindFrom_OnExit(
        FCk_Handle_State& InHandle,
        const FCk_Delegate_State& InDelegate)
    -> FCk_Handle_State
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnStateExit, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_State_UE::
    UnbindFrom_OnUpdate(
        FCk_Handle_State& InHandle,
        const FCk_Delegate_State& InDelegate)
    -> FCk_Handle_State
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnStateUpdate, InHandle, InDelegate);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------