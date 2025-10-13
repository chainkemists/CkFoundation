#include "CkHfsm/Service/CkService_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkHfsm/State/CkState_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    struct RecordOfServices_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfServices> {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Service_UE::
    Create(
        FCk_Handle_State& InStateHandle)
    -> FCk_Handle_Service
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InStateHandle, [&](FCk_Handle InNew)
    {
        InNew.Add<ck::FFragment_Service_Current>();
        InNew.Add<ck::FTag_Service_Setup>();
    });

    // Connect to parent state's record
    RecordOfServices_Utils::AddIfMissing(InStateHandle, ECk_Record_EntryHandlingPolicy::Default);
    RecordOfServices_Utils::Request_Connect(InStateHandle, CastChecked(NewEntity),
        ECk_Record_LabelRequirementPolicy::Optional);

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
    InHandle.AddOrGet<ck::FTag_Service_Enter>();

    return InHandle;
}

auto
    UCk_Utils_Service_UE::
    Request_Stop(
        FCk_Handle_Service& InHandle)
    -> FCk_Handle_Service
{
    InHandle.AddOrGet<ck::FTag_Service_Exit>();

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