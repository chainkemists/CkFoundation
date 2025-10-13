#include "CkService_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcsExt/ContextOwner/CkContextOwner_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Service_UE::
    Create(
        FCk_Handle_State& InStateHandle)
    -> FCk_Handle_Service
{
    // Convert typesafe handle to generic handle for entity creation
    FCk_Handle& GenericHandle = InStateHandle;
    
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(GenericHandle, [&](FCk_Handle InNew)
    {
        InNew.Add<ck::FFragment_Service_Current>();
        InNew.AddOrGet<ck::FTag_Service_Setup>();

        // Set context owner to parent state
        UCk_Utils_ContextOwner_UE::Set_Owner(InNew, GenericHandle);
    });

    return CastChecked(NewEntity);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Service_UE, FCk_Handle_Service, 
    ck::FFragment_Service_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Service_UE::
    Request_Start(
        FCk_Handle_Service& InHandle)
    -> FCk_Handle_Service
{
    InHandle.AddOrGet<ck::FFragment_Service_Requests>()._Requests.Emplace(
        FCk_Request_Service_Command{ECk_Service_Command::Start});

    return InHandle;
}

auto
    UCk_Utils_Service_UE::
    Request_Stop(
        FCk_Handle_Service& InHandle)
    -> FCk_Handle_Service
{
    InHandle.AddOrGet<ck::FFragment_Service_Requests>()._Requests.Emplace(
        FCk_Request_Service_Command{ECk_Service_Command::Stop});

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Service_UE::
    Get_IsWorkDone(
        const FCk_Handle_Service& InHandle)
    -> bool
{
    return InHandle.Has<ck::FTag_Service_WorkDone>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Service_UE::
    BindTo_OnStart(
        FCk_Handle_Service& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Service& InDelegate)
    -> FCk_Handle_Service
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnServiceStart, InHandle, InDelegate, InBindingPolicy);
    return InHandle;
}

auto
    UCk_Utils_Service_UE::
    BindTo_OnStop(
        FCk_Handle_Service& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Service& InDelegate)
    -> FCk_Handle_Service
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnServiceStop, InHandle, InDelegate, InBindingPolicy);
    return InHandle;
}

auto
    UCk_Utils_Service_UE::
    BindTo_OnWorkDone(
        FCk_Handle_Service& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Service& InDelegate)
    -> FCk_Handle_Service
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnServiceWorkDone, InHandle, InDelegate, InBindingPolicy);
    return InHandle;
}

auto
    UCk_Utils_Service_UE::
    UnbindFrom_OnStart(
        FCk_Handle_Service& InHandle,
        const FCk_Delegate_Service& InDelegate)
    -> FCk_Handle_Service
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnServiceStart, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_Service_UE::
    UnbindFrom_OnStop(
        FCk_Handle_Service& InHandle,
        const FCk_Delegate_Service& InDelegate)
    -> FCk_Handle_Service
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnServiceStop, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_Service_UE::
    UnbindFrom_OnWorkDone(
        FCk_Handle_Service& InHandle,
        const FCk_Delegate_Service& InDelegate)
    -> FCk_Handle_Service
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnServiceWorkDone, InHandle, InDelegate);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------