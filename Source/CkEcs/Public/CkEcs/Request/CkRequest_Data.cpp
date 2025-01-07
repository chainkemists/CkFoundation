#include "CkRequest_Data.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

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
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Request_Base::
    PopulateRequestHandle(
        const FCk_Handle& InOwner) const
        -> FCk_Handle
{
    return _RequestBase.PopulateRequestHandle(InOwner);
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

// --------------------------------------------------------------------------------------------------------------------
