#include "CkResourceLoader_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ResourceLoader_UE::
    Request_LoadObject(
        const FCk_Handle& InHandle,
        const FCk_Request_ResourceLoader_LoadObject& InRequest,
        const FCk_Delegate_ResourceLoader_OnObjectLoaded& InDelegate)
    -> void
{
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

    RequestEntity.AddOrGet<ck::FFragment_ResourceLoader_Requests>()._Requests.Emplace(InRequest);

    ck::UUtils_Signal_ResourceLoader_OnObjectLoaded_PostFireUnbind::Bind(RequestEntity, InDelegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
}

auto
    UCk_Utils_ResourceLoader_UE::
    Request_LoadObjectBatch(
        const FCk_Handle& InHandle,
        const FCk_Request_ResourceLoader_LoadObjectBatch& InRequest,
        const FCk_Delegate_ResourceLoader_OnObjectBatchLoaded& InDelegate)
    -> void
{
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

    RequestEntity.AddOrGet<ck::FFragment_ResourceLoader_Requests>()._Requests.Emplace(InRequest);

    ck::UUtils_Signal_ResourceLoader_OnObjectBatchLoaded_PostFireUnbind::Bind(RequestEntity, InDelegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
}

auto
    UCk_Utils_ResourceLoader_UE::
    Conv_SoftClassRef_ToSoftResourceLoaderObjectReference(
        TSoftClassPtr<> SoftClassReference)
    -> FCk_ResourceLoader_ObjectReference_Soft
{
    const auto SoftClassPath = FSoftClassPath(SoftClassReference.ToString());
    return FCk_ResourceLoader_ObjectReference_Soft(SoftClassPath);
}

auto
    UCk_Utils_ResourceLoader_UE::
    Conv_SoftObjRef_ToSoftResourceLoaderObjectReference(
        TSoftObjectPtr<UObject> SoftObjectReference)
    -> FCk_ResourceLoader_ObjectReference_Soft
{
    const auto SoftClassPath = FSoftClassPath(SoftObjectReference.ToString());
    return FCk_ResourceLoader_ObjectReference_Soft(SoftClassPath);
}

auto
    UCk_Utils_ResourceLoader_UE::
    Transform_SoftClassReferences_ToSoftResourceLoaderObjectReferences(
        const TArray<TSoftClassPtr<UObject>>& InReferences)
    -> TArray<FCk_ResourceLoader_ObjectReference_Soft>
{
    return ck::algo::Transform<TArray<FCk_ResourceLoader_ObjectReference_Soft>>(InReferences, [](TSoftClassPtr<UObject> InSoftClass) -> FCk_ResourceLoader_ObjectReference_Soft
    {
        return Conv_SoftClassRef_ToSoftResourceLoaderObjectReference(InSoftClass);
    });
}

auto
    UCk_Utils_ResourceLoader_UE::
    Transform_SoftObjectReferences_ToSoftResourceLoaderObjectReferences(
        const TArray<TSoftObjectPtr<>>& InReferences)
    -> TArray<FCk_ResourceLoader_ObjectReference_Soft>
{
    return ck::algo::Transform<TArray<FCk_ResourceLoader_ObjectReference_Soft>>(InReferences, [](TSoftObjectPtr<UObject> InSoftObject) -> FCk_ResourceLoader_ObjectReference_Soft
    {
        return Conv_SoftObjRef_ToSoftResourceLoaderObjectReference(InSoftObject);
    });
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
