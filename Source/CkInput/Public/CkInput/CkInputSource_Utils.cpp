#include "CkInputSource_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(
    UCk_Utils_InputSource_UE,
    FCk_Handle_InputSource,
    ck::FFragment_InputSource_Params,
    ck::FFragment_InputSource_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InputSource_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_InputSource_ParamsData& InParams)
    -> FCk_Handle_InputSource
{
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Add: invalid Handle [{}] — cannot compose an InputSource onto it"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_InputSource_Params>(InParams);
    InHandle.Add<ck::FFragment_InputSource_Current>();

    return CastChecked(InHandle);
}

auto
    UCk_Utils_InputSource_UE::
    Create(
        FCk_Handle& InOwner,
        const FCk_Fragment_InputSource_ParamsData& InParams)
    -> FCk_Handle_InputSource
{
    const auto OwnerIsValid = ck::IsValid(InOwner);
    CK_ENSURE_IF_NOT(OwnerIsValid,
        TEXT("Create: invalid owner Handle [{}] — cannot create an InputSource under it"), InOwner)
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);

    return Add(NewEntity, InParams);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InputSource_UE::
    Get_LocalPlayerIndex(
        const FCk_Handle_InputSource& InInputSource)
    -> int32
{
    return InInputSource.Get<ck::FFragment_InputSource_Params>().Get_LocalPlayerIndex();
}

auto
    UCk_Utils_InputSource_UE::
    Get_PendingRawEvents(
        const FCk_Handle_InputSource& InInputSource)
    -> TArray<FCk_InputSource_RawEvent>
{
    return InInputSource.Get<ck::FFragment_InputSource_Current>().Get_PendingRawEvents();
}

auto
    UCk_Utils_InputSource_UE::
    Get_NumPendingRawEvents(
        const FCk_Handle_InputSource& InInputSource)
    -> int32
{
    return InInputSource.Get<ck::FFragment_InputSource_Current>().Get_PendingRawEvents().Num();
}

auto
    UCk_Utils_InputSource_UE::
    Get_OwnedDevices(
        const FCk_Handle_InputSource& InInputSource)
    -> TArray<FCk_InputSource_DeviceId>
{
    return InInputSource.Get<ck::FFragment_InputSource_Current>().Get_OwnedDevices();
}

auto
    UCk_Utils_InputSource_UE::
    Get_OwnsDevice(
        const FCk_Handle_InputSource& InInputSource,
        const FCk_InputSource_DeviceId& InDeviceId)
    -> bool
{
    return InInputSource.Get<ck::FFragment_InputSource_Current>().Get_OwnedDevices().Contains(InDeviceId);
}

auto
    UCk_Utils_InputSource_UE::
    TryGet_SourceOwningDevice(
        FCk_Handle InAnyEntityInWorld,
        const FCk_InputSource_DeviceId& InDeviceId)
    -> FCk_Handle_InputSource
{
    if (ck::Is_NOT_Valid(InAnyEntityInWorld))
    { return {}; }

    const auto Registry = InAnyEntityInWorld.Get_RegistryView();

    auto FoundSource = FCk_Handle_InputSource{};

    InAnyEntityInWorld.View<
        ck::FFragment_InputSource_Params,
        ck::FFragment_InputSource_Current,
        CK_IGNORE_PENDING_KILL>().ForEach(
        [&](
            FCk_Entity InEntity,
            const ck::FFragment_InputSource_Params&,
            const ck::FFragment_InputSource_Current& InCurrent)
        {
            if (ck::IsValid(FoundSource))
            { return; }

            if (NOT InCurrent.Get_OwnedDevices().Contains(InDeviceId))
            { return; }

            auto EntityHandle = ck::MakeHandle(InEntity, Registry);
            FoundSource = CastChecked(EntityHandle);
        });

    return FoundSource;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InputSource_UE::
    Request_InjectRawEvent(
        FCk_Handle_InputSource& InInputSource,
        const FCk_Request_InputSource_InjectRawEvent& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_InputSource
{
    const auto SourceIsValid = ck::IsValid(InInputSource);
    CK_ENSURE_IF_NOT(SourceIsValid,
        TEXT("Request_InjectRawEvent: invalid InputSource handle [{}] — the event is dropped"), InInputSource)
    {
        InDelegate.ExecuteIfBound(InInputSource, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InInputSource;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InInputSource.AddOrGet<ck::FFragment_InputSource_Requests>()._Requests.Emplace(InRequest);

    return InInputSource;
}

auto
    UCk_Utils_InputSource_UE::
    Request_AssignDevice(
        FCk_Handle_InputSource& InInputSource,
        const FCk_Request_InputSource_AssignDevice& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_InputSource
{
    const auto SourceIsValid = ck::IsValid(InInputSource);
    CK_ENSURE_IF_NOT(SourceIsValid,
        TEXT("Request_AssignDevice: invalid InputSource handle [{}] — a device may only be assigned to a "
             "source that exists"), InInputSource)
    {
        InDelegate.ExecuteIfBound(InInputSource, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InInputSource;
    }

    const auto RawDeviceUserIndex = InRequest.Get_DeviceId().Get_RawDeviceUserIndex();

    const auto UserIndexIsValid = RawDeviceUserIndex >= 0;
    CK_ENSURE_IF_NOT(UserIndexIsValid,
        TEXT("Request_AssignDevice: raw device user index [{}] on InputSource [{}] is negative and cannot "
             "identify a device"), RawDeviceUserIndex, InInputSource)
    {
        InDelegate.ExecuteIfBound(InInputSource, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InInputSource;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InInputSource.AddOrGet<ck::FFragment_InputSource_Requests>()._Requests.Emplace(InRequest);

    return InInputSource;
}

// --------------------------------------------------------------------------------------------------------------------
