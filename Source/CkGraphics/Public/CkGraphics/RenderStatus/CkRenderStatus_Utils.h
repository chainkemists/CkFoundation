#pragma once

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
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
              DisplayName="[Ck][RenderStatus] Add Feature")
    static void
    Add(
        UPARAM(ref) FCk_Handle_UnderConstruction& InHandle,
        const FCk_Fragment_RenderStatus_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderStatus",
              DisplayName="[Ck][RenderStatus] Has Feature")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderStatus",
              DisplayName="[Ck][RenderStatus] Ensure Has Feature")
    static bool
    Ensure(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderStatus",
              meta = (AutoCreateRefTerm = "InOptionalPayload"),
              DisplayName="[Ck][RenderStatus] Request Query Rendered Actors")
    static void
    Request_QueryRenderedActors(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_RenderStatus_QueryRenderedActors& InRequest,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Delegate_RenderStatus_OnRenderedActorsQueried& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
