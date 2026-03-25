#include "CkStateMachine_Utils.h"

#include "CkStateMachine/CkStateMachine_Fragment.h"

#if CK_BUILD_SM_GRAPH_WALK
#include "CkStateMachine/CkStateMachine_Debug_GraphWalk_Fragment.h"
#endif

#include "CkDynamic/CkDynamic_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.inl.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    Add(
        FCk_Handle& InOwner,
        TSubclassOf<UCk_SmState_EntityScript> InInitialStateClass,
        ECk_SmAutoStart InAutoStart)
    -> FCk_Handle_StateMachine
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwner),
        TEXT("Invalid owner handle when creating StateMachine"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InInitialStateClass),
        TEXT("Invalid initial state class when creating StateMachine"))
    { return {}; }

    auto SmEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);

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
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return ck::IsValid(InHandle)
        && InHandle.Has<ck::FFragment_Sm_Current>();
}

auto
    UCk_Utils_StateMachine_UE::
    Get_RunStatus(
        const FCk_Handle_StateMachine& InStateMachine)
    -> ECk_SmRunStatus
{
    CK_ENSURE_IF_NOT(ck::IsValid(InStateMachine),
        TEXT("Invalid SM handle in Get_RunStatus"))
    { return ECk_SmRunStatus::Stopped; }

    return InStateMachine.Get<ck::FFragment_Sm_Current>().Get_RunStatus();
}

auto
    UCk_Utils_StateMachine_UE::
    Get_CurrentStateClass(
        const FCk_Handle_StateMachine& InStateMachine)
    -> TSubclassOf<UCk_SmState_EntityScript>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InStateMachine),
        TEXT("Invalid SM handle in Get_CurrentStateClass"))
    { return nullptr; }

    return InStateMachine.Get<ck::FFragment_Sm_Current>().Get_CurrentStateClass();
}

auto
    UCk_Utils_StateMachine_UE::
    Get_CurrentStateHandle(
        const FCk_Handle_StateMachine& InStateMachine)
    -> FCk_Handle_SmState
{
    CK_ENSURE_IF_NOT(ck::IsValid(InStateMachine),
        TEXT("Invalid SM handle in Get_CurrentStateHandle"))
    { return {}; }

    return InStateMachine.Get<ck::FFragment_Sm_Current>().Get_CurrentStateHandle();
}

auto
    UCk_Utils_StateMachine_UE::
    IsInState(
        const FCk_Handle_StateMachine& InStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InStateClass)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InStateMachine),
        TEXT("Invalid SM handle in IsInState"))
    { return false; }

    return InStateMachine.Get<ck::FFragment_Sm_Current>().Get_CurrentStateClass() == InStateClass;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    AddPayload(
        FCk_Handle& InEntity,
        const FInstancedStruct& InPayload)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InEntity),
        TEXT("Invalid entity handle in AddPayload"))
    { return InEntity; }

    UCk_Utils_DynamicFragment_UE::Add_Fragment(InEntity, InPayload);

    return InEntity;
}

auto
    UCk_Utils_StateMachine_UE::
    GetPayload(
        const FCk_Handle& InEntity,
        const UScriptStruct* InType)
    -> FInstancedStruct&
{
    return UCk_Utils_DynamicFragment_UE::Get_Fragment_TypeUnsafe(InEntity, InType);
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

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    DoCast(
        FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult)
    -> FCk_Handle_StateMachine
{
    if (Has(InHandle))
    {
        OutResult = ECk_SucceededFailed::Succeeded;
        return Cast(InHandle);
    }

    OutResult = ECk_SucceededFailed::Failed;
    return {};
}

auto
    UCk_Utils_StateMachine_UE::
    DoCastChecked(
        FCk_Handle InHandle)
    -> FCk_Handle_StateMachine
{
    return CastChecked(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachine_UE::
    DoAddRequest(
        FCk_Handle_StateMachine& InSm,
        const auto& InRequest)
    -> FCk_Handle_StateMachine
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSm),
        TEXT("Invalid SM handle when adding request"))
    { return InSm; }

    auto& Requests = InSm.AddOrGet<ck::FFragment_Sm_Requests>();
    Requests._Requests.Add(InRequest);

    return InSm;
}

// --------------------------------------------------------------------------------------------------------------------
