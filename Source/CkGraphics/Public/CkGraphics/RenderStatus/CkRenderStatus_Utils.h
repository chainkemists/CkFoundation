#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkGraphics/RenderStatus/CkRenderStatus_Fragment_Data.h"

#include "CkRenderStatus_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_RenderStatus"))
class CKGRAPHICS_API UCk_Utils_RenderStatus_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_RenderStatus_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_RenderStatus);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderStatus",
              DisplayName="[Ck][RenderStatus] Add Feature")
    static FCk_Handle_RenderStatus
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_RenderStatus_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderStatus",
              DisplayName="[Ck][RenderStatus] Create")
    static FCk_Handle_RenderStatus
    Create(
        UPARAM(ref) FCk_Handle& InOwner,
        const FCk_Fragment_RenderStatus_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderStatus",
              DisplayName="[Ck][RenderStatus] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderStatus",
              DisplayName="[Ck][RenderStatus] Ensure Has Feature")
    static bool
    Ensure(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderStatus",
              meta = (AutoCreateRefTerm = "InOptionalPayload, InCompletionDelegate"),
              DisplayName="[Ck][RenderStatus] Request Query Rendered Actors")
    static void
    Request_QueryRenderedActors(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_RenderStatus_QueryRenderedActors& InRequest,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Delegate_RenderStatus_OnRenderedActorsQueried& InDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
