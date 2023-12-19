#include "CkResourceLoader_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ResourceLoader_UE::
    Request_LoadObject(
        FCk_Handle InHandle,
        const FCk_Request_ResourceLoader_LoadObject& InRequest,
        const FCk_Delegate_ResourceLoader_OnObjectLoaded& InDelegate)
    -> void
{
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

    RequestEntity.AddOrGet<ck::FFragment_ResourceLoader_Requests>()._Requests.Emplace(InRequest);

    if (NOT InDelegate.IsBound())
    { return; }

    ck::UUtils_Signal_ResourceLoader_OnObjectLoaded_PostFireUnbind::Bind(RequestEntity, InDelegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
}

auto
    UCk_Utils_ResourceLoader_UE::
    Request_LoadObjectBatch(
        FCk_Handle InHandle,
        const FCk_Request_ResourceLoader_LoadObjectBatch& InRequest,
        const FCk_Delegate_ResourceLoader_OnObjectBatchLoaded& InDelegate)
    -> void
{
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

    RequestEntity.AddOrGet<ck::FFragment_ResourceLoader_Requests>()._Requests.Emplace(InRequest);

    if (NOT InDelegate.IsBound())
    { return; }

    ck::UUtils_Signal_ResourceLoader_OnObjectBatchLoaded_PostFireUnbind::Bind(RequestEntity, InDelegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
}

auto
    UCk_Utils_ResourceLoader_UE::
    DoAddPendingObject(
        FCk_Handle InHandle,
        const FCk_ResourceLoader_PendingObject& InPendingObject)
    -> void
{
    InHandle.AddOrGet<ck::FFragment_ResourceLoader_PendingObjects>()._PendingObjects.Add(InPendingObject);
}

auto
    UCk_Utils_ResourceLoader_UE::
    DoAddPendingObjectBatch(
        FCk_Handle InHandle,
        const FCk_ResourceLoader_PendingObjectBatch& InPendingObjectBatch)
    -> void
{
    InHandle.AddOrGet<ck::FFragment_ResourceLoader_PendingObjectBatches>()._PendingObjectBatches.Add(InPendingObjectBatch);
}

// --------------------------------------------------------------------------------------------------------------------
