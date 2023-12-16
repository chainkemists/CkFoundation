#include "CkResourceLoader_Utils.h"

#include "CkResourceLoader/CkResourceLoader_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ResourceLoader_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_ResourceLoader_ParamsData& InParams)
    -> void
{
    InHandle.Add<ck::FFragment_ResourceLoader_Params>(InParams);
}

auto
    UCk_Utils_ResourceLoader_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_ResourceLoader_Params>();
}

auto
    UCk_Utils_ResourceLoader_UE::
    Ensure(
        FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have Resource Loader"), InHandle)
    { return false; }

    return true;
}

auto
    UCk_Utils_ResourceLoader_UE::
    Get_IsObjectPending(
        FCk_Handle InHandle,
        const FCk_ResourceLoader_PendingObject& InPendingObject)
    -> bool
{
    if (NOT Ensure(InHandle))
    { return {}; }

    if (NOT InHandle.Has<ck::FFragment_ResourceLoader_PendingObjects>())
    { return {}; }

    return InHandle.Get<ck::FFragment_ResourceLoader_PendingObjects>().Get_PendingObjects().Contains(InPendingObject);
}

auto
    UCk_Utils_ResourceLoader_UE::
    Get_IsObjectBatchPending(
        FCk_Handle InHandle,
        const FCk_ResourceLoader_PendingObjectBatch& InPendingObjectBatch)
    -> bool
{
    if (NOT Ensure(InHandle))
    { return {}; }

    if (NOT InHandle.Has<ck::FFragment_ResourceLoader_PendingObjectBatches>())
    { return {}; }

    return InHandle.Get<ck::FFragment_ResourceLoader_PendingObjectBatches>().Get_PendingObjectBatches().Contains(InPendingObjectBatch);
}

auto
    UCk_Utils_ResourceLoader_UE::
    Get_HasPendingObjectBatches(
        FCk_Handle InHandle)
    -> bool
{
    if (NOT Ensure(InHandle))
    { return {}; }

    return Get_NumPendingObjectBatches(InHandle) > 0;
}

auto
    UCk_Utils_ResourceLoader_UE::
    Get_HasPendingObjects(
        FCk_Handle InHandle)
    -> bool
{
    if (NOT Ensure(InHandle))
    { return {}; }

    return Get_NumPendingObjects(InHandle) > 0;
}

auto
    UCk_Utils_ResourceLoader_UE::
    Get_NumPendingObjects(
        FCk_Handle InHandle)
    -> int32
{
    if (NOT Ensure(InHandle))
    { return {}; }

    if (NOT InHandle.Has<ck::FFragment_ResourceLoader_PendingObjects>())
    { return {}; }

    return InHandle.Get<ck::FFragment_ResourceLoader_PendingObjects>().Get_PendingObjects().Num();
}

auto
    UCk_Utils_ResourceLoader_UE::
    Get_NumPendingObjectBatches(
        FCk_Handle InHandle)
    -> int32
{
    if (NOT Ensure(InHandle))
    { return {}; }

    if (NOT InHandle.Has<ck::FFragment_ResourceLoader_PendingObjectBatches>())
    { return {}; }

    return InHandle.Get<ck::FFragment_ResourceLoader_PendingObjectBatches>().Get_PendingObjectBatches().Num();
}

auto
    UCk_Utils_ResourceLoader_UE::
    Request_LoadObject(
        FCk_Handle InHandle,
        const FCk_Request_ResourceLoader_LoadObject& InRequest)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    InHandle.AddOrGet<ck::FFragment_ResourceLoader_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_ResourceLoader_UE::
    Request_LoadObjectBatch(
        FCk_Handle InHandle,
        const FCk_Request_ResourceLoader_LoadObjectBatch& InRequest)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    InHandle.AddOrGet<ck::FFragment_ResourceLoader_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_ResourceLoader_UE::
    Request_UnloadObject(
        FCk_Handle InHandle,
        const FCk_Request_ResourceLoader_UnloadObject& InRequest)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    InHandle.AddOrGet<ck::FFragment_ResourceLoader_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_ResourceLoader_UE::
    DoAddPendingObject(
        FCk_Handle InHandle,
        const FCk_ResourceLoader_PendingObject& InPendingObject)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    InHandle.AddOrGet<ck::FFragment_ResourceLoader_PendingObjects>()._PendingObjects.Add(InPendingObject);
}

auto
    UCk_Utils_ResourceLoader_UE::
    DoTryRemovePendingObject(
        FCk_Handle InHandle,
        const FCk_ResourceLoader_PendingObject& InPendingObject)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    if (NOT InHandle.Has<ck::FFragment_ResourceLoader_PendingObjects>())
    { return; }

    auto& PendingObjects = InHandle.Get<ck::FFragment_ResourceLoader_PendingObjects>()._PendingObjects;

    PendingObjects.Remove(InPendingObject);

    if (PendingObjects.IsEmpty())
    {
        InHandle.Remove<ck::FFragment_ResourceLoader_PendingObjects>();
    }
}

auto
    UCk_Utils_ResourceLoader_UE::
    DoAddPendingObjectBatch(
        FCk_Handle InHandle,
        const FCk_ResourceLoader_PendingObjectBatch& InPendingObjectBatch)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    InHandle.AddOrGet<ck::FFragment_ResourceLoader_PendingObjectBatches>()._PendingObjectBatches.Add(InPendingObjectBatch);
}

auto
    UCk_Utils_ResourceLoader_UE::
    DoTryRemovePendingObjectBatch(
        FCk_Handle InHandle,
        const FCk_ResourceLoader_PendingObjectBatch& InPendingObjectBatch)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    if (NOT InHandle.Has<ck::FFragment_ResourceLoader_PendingObjectBatches>())
    { return; }

    auto& PendingObjectBatches = InHandle.Get<ck::FFragment_ResourceLoader_PendingObjectBatches>()._PendingObjectBatches;

    PendingObjectBatches.Remove(InPendingObjectBatch);

    if (PendingObjectBatches.IsEmpty())
    {
        InHandle.Remove<ck::FFragment_ResourceLoader_PendingObjectBatches>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
