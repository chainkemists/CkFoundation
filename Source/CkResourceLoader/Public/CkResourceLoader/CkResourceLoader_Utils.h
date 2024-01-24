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
              DisplayName = "[ResourceLoader] Request Load Object")
    static void
    Request_LoadObject(
        FCk_Handle InHandle,
        const FCk_Request_ResourceLoader_LoadObject& InRequest,
        const FCk_Delegate_ResourceLoader_OnObjectLoaded& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ResourceLoader",
              DisplayName = "[ResourceLoader] Request Load Object Batch")
    static void
    Request_LoadObjectBatch(
        FCk_Handle InHandle,
        const FCk_Request_ResourceLoader_LoadObjectBatch& InRequest,
        const FCk_Delegate_ResourceLoader_OnObjectBatchLoaded& InDelegate);

private:
    static auto DoAddPendingObject(
        FCk_Handle InHandle,
        const FCk_ResourceLoader_PendingObject& InPendingObject) -> void;

    static auto DoAddPendingObjectBatch(
        FCk_Handle InHandle,
        const FCk_ResourceLoader_PendingObjectBatch& InPendingObjectBatch) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

