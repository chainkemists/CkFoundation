#include "CkResourceLoader_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkResourceLoader/CkResourceLoader_Log.h"
#include "CkResourceLoader/Settings/CkResourceLoader_Settings.h"

#include <Engine/AssetManager.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ResourceLoader_UE::
    Request_LoadObject(
        const FCk_Handle& InHandle,
        const FCk_Request_ResourceLoader_LoadObject& InRequest,
        const FCk_Delegate_ResourceLoader_OnObjectLoaded& InDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate)
    -> void
{
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

    CK_CALLSTACK_RECORD(ck::FFragment_ResourceLoader_Requests, RequestEntity);

    if (InCompletionDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InCompletionDelegate); }

    RequestEntity.AddOrGet<ck::FFragment_ResourceLoader_Requests>()._Requests.Emplace(InRequest);

    ck::UUtils_Signal_ResourceLoader_OnObjectLoaded_PostFireUnbind::Bind(RequestEntity, InDelegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
}

auto
    UCk_Utils_ResourceLoader_UE::
    Request_LoadObjectBatch(
        const FCk_Handle& InHandle,
        const FCk_Request_ResourceLoader_LoadObjectBatch& InRequest,
        const FCk_Delegate_ResourceLoader_OnObjectBatchLoaded& InDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate)
    -> void
{
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

    CK_CALLSTACK_RECORD(ck::FFragment_ResourceLoader_Requests, RequestEntity);

    if (InCompletionDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InCompletionDelegate); }

    RequestEntity.AddOrGet<ck::FFragment_ResourceLoader_Requests>()._Requests.Emplace(InRequest);

    ck::UUtils_Signal_ResourceLoader_OnObjectBatchLoaded_PostFireUnbind::Bind(RequestEntity, InDelegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
}

auto
    UCk_Utils_ResourceLoader_UE::
    RequestLoad_RootedBatch(
        FName InConsumerId,
        const TArray<FSoftObjectPath>& InSoftPaths)
    -> FCk_ResourceLoader_RootedAssetBatch
{
    auto Batch = FCk_ResourceLoader_RootedAssetBatch{};
    Batch._Requested = true;
    Batch._RequestedPaths = InSoftPaths;

    const auto AllPathsAreValid = ck::algo::NoneOf(InSoftPaths, [](const FSoftObjectPath& InPath)
    {
        return InPath.IsNull();
    });

    CK_ENSURE_IF_NOT(NOT InSoftPaths.IsEmpty() && AllPathsAreValid,
        TEXT("RequestLoad_RootedBatch for Consumer [{}] rejected - the soft path list is empty or contains a null path"),
        InConsumerId)
    {}

    if (InSoftPaths.IsEmpty() || NOT AllPathsAreValid)
    { return Batch; }

    const auto LoadingPolicy = UCk_Utils_ResourceLoader_Settings_UE::Get_LoadingPolicyForConsumer(InConsumerId);

    auto& StreamableManager = UAssetManager::GetStreamableManager();
    auto PathsToStream = InSoftPaths;

    switch (LoadingPolicy)
    {
        case ECk_ResourceLoader_LoadingPolicy::Synchronous:
        {
            Batch._StreamableHandle = StreamableManager.RequestSyncLoad(MoveTemp(PathsToStream));
            break;
        }
        case ECk_ResourceLoader_LoadingPolicy::Async:
        {
            Batch._StreamableHandle = StreamableManager.RequestAsyncLoad(MoveTemp(PathsToStream));
            break;
        }
        default:
        {
            CK_INVALID_ENUM(LoadingPolicy);
            break;
        }
    }

    ck::resource_loader::VeryVerbose(TEXT("RootedBatch load requested by Consumer [{}] with Policy [{}] for [{}] path(s)"),
        InConsumerId, LoadingPolicy, InSoftPaths.Num());

    return Batch;
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
