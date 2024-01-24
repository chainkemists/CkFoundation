#pragma once

#include "CkECS/Handle/CkHandle.h"

#include "CkGraphics/RenderStatus/CkRenderStatus_Fragment_Data.h"

#include "CkRenderStatus_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKGRAPHICS_API UCk_Utils_RenderStatus_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_RenderStatus_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderStatus",
              DisplayName="Add RenderStatus Group")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_RenderStatus_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderStatus",
              DisplayName="Has RenderStatus Group")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderStatus",
              DisplayName="Ensure Has RenderStatus Group")
    static bool
    Ensure(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderStatus")
    static void
    Request_QueryRenderedActors(
        FCk_Handle InHandle,
        const FCk_Request_RenderStatus_QueryRenderedActors& InRequest,
        FInstancedStruct InOptionalPayload,
        const FCk_Delegate_RenderStatus_OnRenderedActorsQueried& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
