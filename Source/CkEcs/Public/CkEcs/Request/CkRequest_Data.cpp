#include "CkRequest_Data.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FRequest_Base::
        PopulateRequestHandle(
            const FCk_Handle& InOwner) const
            -> FCk_Handle
    {
        CK_ENSURE_IF_NOT(ck::Is_NOT_Valid(_RequestHandle),
            TEXT(
                "Cannot Populate RequestHandle with requested Owner [{}]. RequestHandle [{}] is VALID and already populated with Owner [{}]."
            ),
            InOwner, _RequestHandle, UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(_RequestHandle))
        {
            return {};
        }

        _RequestHandle = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
        if (const auto& DebugName = Get_RequestDebugName();
            DebugName.IsValid())
        {
            UCk_Utils_Handle_UE::Set_DebugName(_RequestHandle, Get_RequestDebugName());
        }
#endif

        return _RequestHandle;
    }

    auto
        FRequest_Base::
        GetAndDestroyRequestHandle() const
            -> FCk_Handle
    {
        CK_ENSURE_IF_NOT(ck::IsValid(_RequestHandle),
            TEXT("Cannot GetAndDestroy Request Handle [{}] since it is INVALID"), _RequestHandle)
        {
            return {};
        }

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(_RequestHandle);

        return _RequestHandle;
    }

    auto
        FRequest_Base::
        Get_IsRequestHandleValid() const
            -> bool
    {
        return ck::IsValid(_RequestHandle);
    }

    auto
        FRequest_Base::
        Request_TransferHandleToOther(
            const FRequest_Base& InOther) const
            -> void
    {
        InOther._RequestHandle = std::move(_RequestHandle);
    }

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
    auto
        FRequest_Base::
        Get_RequestDebugName() const
        -> FName
    {
        // Since this base class is used in FCk_Request_Base we have no way to ensure that subclasses of FRequest_Base implement this function using CK_REQUEST_DEFINE_DEBUG_NAME
        return NAME_None;
    }
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Request_Base::
    PopulateRequestHandle(
        const FCk_Handle& InOwner) const
        -> FCk_Handle
{
    auto RequestHandle = _RequestBase.PopulateRequestHandle(InOwner);

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
        UCk_Utils_Handle_UE::Set_DebugName(RequestHandle, Get_RequestDebugName());
#endif

    return RequestHandle;
}

auto
    FCk_Request_Base::
    GetAndDestroyRequestHandle() const
        -> FCk_Handle
{
    return _RequestBase.GetAndDestroyRequestHandle();
}

auto
    FCk_Request_Base::
    Get_IsRequestHandleValid() const
        -> bool
{
    return _RequestBase.Get_IsRequestHandleValid();
}

auto
    FCk_Request_Base::
    Request_TransferHandleToOther(
        const ck::FRequest_Base& InOther) const
        -> void
{
    _RequestBase.Request_TransferHandleToOther(InOther);
}

auto
    FCk_Request_Base::
    Request_TransferHandleToOther(
        const ThisType& InOther) const
        -> void
{
    _RequestBase.Request_TransferHandleToOther(InOther._RequestBase);
}

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
auto
    FCk_Request_Base::
    Get_RequestDebugName() const
    -> FName
{
    CK_ENSURE_IF_NOT(false, TEXT("Request type does not provide a debug type name!")) {}
    return "RequestBase";
}
#endif

// --------------------------------------------------------------------------------------------------------------------
