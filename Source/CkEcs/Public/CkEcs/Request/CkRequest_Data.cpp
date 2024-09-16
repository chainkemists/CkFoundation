#include "CkRequest_Data.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Request_Base::
    PopulateRequestHandle(
        FCk_Handle InOwner) const
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::Is_NOT_Valid(_RequestHandle),
        TEXT("Cannot Populate RequestHandle with requested Owner [{}]. RequestHandle [{}] is VALID and already populated with Owner [{}]."),
        InOwner, _RequestHandle, UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(_RequestHandle))
    { return {}; }

    _RequestHandle = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);

    UCk_Utils_Handle_UE::Set_DebugName(_RequestHandle, *ck::Format_UE(TEXT("Request owned by [{}]"), InOwner));

    return _RequestHandle;
}

auto
    FCk_Request_Base::
    GetAndDestroyRequestHandle() const
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(_RequestHandle),
        TEXT("Cannot GetAndDestroy Request Handle [{}] since it is INVALID"), _RequestHandle)
    { return {}; }

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(_RequestHandle);

    return _RequestHandle;
}

auto
    FCk_Request_Base::
    Get_IsRequestHandleValid() const
    -> bool
{
    return ck::IsValid(_RequestHandle);
}

// --------------------------------------------------------------------------------------------------------------------