#pragma once

#include "CkResourceLoader/CkResourceLoader_Fragment.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkResourceLoader_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_ResourceLoader_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKRESOURCELOADER_API UCk_Utils_ResourceLoader_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ResourceLoader_UE);

public:
    friend class ck::FProcessor_ResourceLoader_HandleRequests;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ResourceLoader",
              DisplayName = "[Ck][ResourceLoader] Request Load Object",
              meta = (AutoCreateRefTerm = "InCompletionDelegate"))
    static void
    Request_LoadObject(
        const FCk_Handle& InHandle,
        const FCk_Request_ResourceLoader_LoadObject& InRequest,
        const FCk_Delegate_ResourceLoader_OnObjectLoaded& InDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ResourceLoader",
              DisplayName = "[Ck][ResourceLoader] Request Load Object Batch",
              meta = (AutoCreateRefTerm = "InCompletionDelegate"))
    static void
    Request_LoadObjectBatch(
        const FCk_Handle& InHandle,
        const FCk_Request_ResourceLoader_LoadObjectBatch& InRequest,
        const FCk_Delegate_ResourceLoader_OnObjectBatchLoaded& InDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate);

public:
    // C++-only, processor-friendly load: no request entity, no delegate to bind. The loading policy
    // resolves from the loader project settings — the per-consumer override for InConsumerId if one
    // is configured, else the project default (Async). Synchronous is ready on return; Async is
    // polled via the returned batch (an async request whose assets are already resident completes
    // inline). The batch's streamable handle is the GC root for the loaded assets: store the batch
    // where the assets must stay alive, reset it to release them.
    static auto
    RequestLoad_RootedBatch(
        FName InConsumerId,
        const TArray<FSoftObjectPath>& InSoftPaths) -> FCk_ResourceLoader_RootedAssetBatch;

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Convert Soft Class Reference To Soft Resource Loader Object Reference",
        Category = "Ck|Utils|ResourceLoader",
        meta = (CompactNodeTitle = "->"))
    static FCk_ResourceLoader_ObjectReference_Soft
    Conv_SoftClassRef_ToSoftResourceLoaderObjectReference(
        TSoftClassPtr<UObject> SoftClassReference);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Convert Soft Object Reference To Soft Resource Loader Object Reference",
        Category = "Ck|Utils|ResourceLoader",
        meta = (CompactNodeTitle = "->"))
    static FCk_ResourceLoader_ObjectReference_Soft
    Conv_SoftObjRef_ToSoftResourceLoaderObjectReference(
        TSoftObjectPtr<UObject> SoftObjectReference);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Transform Soft Class References To Soft Resource Loader Object References",
        Category = "Ck|Utils|ResourceLoader",
        meta = (CompactNodeTitle = "Transform_ToSoftResourceLoaderRefs"))
    static TArray<FCk_ResourceLoader_ObjectReference_Soft>
    Transform_SoftClassReferences_ToSoftResourceLoaderObjectReferences(
        const TArray<TSoftClassPtr<UObject>>& InReferences);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Transform Soft Object References To Soft Resource Loader Object References",
        Category = "Ck|Utils|ResourceLoader",
        meta = (CompactNodeTitle = "Transform_ToSoftResourceLoaderRefs"))
    static TArray<FCk_ResourceLoader_ObjectReference_Soft>
    Transform_SoftObjectReferences_ToSoftResourceLoaderObjectReferences(
        const TArray<TSoftObjectPtr<UObject>>& InReferences);

private:
    static auto
    DoAddPendingObject(
        FCk_Handle InHandle,
        const FCk_ResourceLoader_PendingObject& InPendingObject) -> void;

    static auto
    DoAddPendingObjectBatch(
        FCk_Handle InHandle,
        const FCk_ResourceLoader_PendingObjectBatch& InPendingObjectBatch) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

