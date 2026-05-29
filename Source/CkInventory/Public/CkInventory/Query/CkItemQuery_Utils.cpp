#include "CkItemQuery_Utils.h"

#include "CkInventory/Query/CkItemQuery_Fragment.h"
#include "CkInventory/Query/CkItemQuery_Subsystem.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemQuery_UE::
    Request_QueryItemDefinitions(
        const FCk_Handle& InAnyHandle,
        const FCk_Request_ItemQuery_QueryDefinitions& InRequest,
        const FCk_Delegate_ItemQuery_OnQueried& InDelegate)
    -> void
{
    // Kick the index build immediately so the result is ready as soon as
    // possible; the processor pends the request until it is.
    if (auto* Subsystem = UCk_ItemQuery_Subsystem_UE::Get())
    { Subsystem->Request_BuildIndex(); }

    // InAnyHandle only scopes the transient request entity — it carries no
    // meaning beyond "some valid entity in this world" (ck::TransientEntity() is fine).
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAnyHandle);

    RequestEntity.AddOrGet<ck::FFragment_ItemQuery_Requests>()._Requests.Add(InRequest);
    ck::UUtils_Signal_OnItemDefinitionsQueried_PostFireUnbind::Bind(
        RequestEntity, InDelegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
}

// --------------------------------------------------------------------------------------------------------------------
