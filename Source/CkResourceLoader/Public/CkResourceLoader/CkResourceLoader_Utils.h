#pragma once

#include "CkResourceLoader/CkResourceLoader_Fragment.h"

#include "CkCore/Macros/CkMacros.h"

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
              DisplayName="Add New Resource Loader")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_ResourceLoader_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ResourceLoader",
              DisplayName="Has Resource Loader")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ResourceLoader",
              DisplayName="Ensure Has Resource Loader")
    static bool
    Ensure(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ResourceLoader")
    static bool
    Get_IsObjectPending(
        FCk_Handle InHandle,
        const FCk_ResourceLoader_PendingObject& InPendingObject);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ResourceLoader")
    static bool
    Get_IsObjectBatchPending(
        FCk_Handle InHandle,
        const FCk_ResourceLoader_PendingObjectBatch& InPendingObjectBatch);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ResourceLoader")
    static bool
    Get_HasPendingObjects(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ResourceLoader")
    static bool
    Get_HasPendingObjectBatches(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ResourceLoader")
    static int32
    Get_NumPendingObjects(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ResourceLoader")
    static int32
    Get_NumPendingObjectBatches(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ResourceLoader")
    static void
    Request_LoadObject(
        FCk_Handle InHandle,
        const FCk_Request_ResourceLoader_LoadObject& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ResourceLoader")
    static void
    Request_LoadObjectBatch(
        FCk_Handle InHandle,
        const FCk_Request_ResourceLoader_LoadObjectBatch& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ResourceLoader")
    static void
    Request_UnloadObject(
        FCk_Handle InHandle,
        const FCk_Request_ResourceLoader_UnloadObject& InRequest);

private:
    static auto DoAddPendingObject(
        FCk_Handle InHandle,
        const FCk_ResourceLoader_PendingObject& InPendingObject) -> void;

    static auto DoTryRemovePendingObject(
        FCk_Handle InHandle,
        const FCk_ResourceLoader_PendingObject& InPendingObject) -> void;

    static auto DoAddPendingObjectBatch(
        FCk_Handle InHandle,
        const FCk_ResourceLoader_PendingObjectBatch& InPendingObjectBatch) -> void;

    static auto DoTryRemovePendingObjectBatch(
        FCk_Handle InHandle,
        const FCk_ResourceLoader_PendingObjectBatch& InPendingObjectBatch) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

