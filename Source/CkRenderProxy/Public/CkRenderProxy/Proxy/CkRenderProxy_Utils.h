#pragma once

#include "CkRenderProxy/Proxy/CkRenderProxy_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkRenderProxy_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_RenderProxy"))
class CKRENDERPROXY_API UCk_Utils_RenderProxy_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_RenderProxy_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_RenderProxy);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][RenderProxy] Add Feature")
    static FCk_Handle_RenderProxy
    Add(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_Fragment_RenderProxy_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderProxy",
              DisplayName="[Ck][RenderProxy] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderProxy",
              DisplayName="[Ck][RenderProxy] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_RenderProxy
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderProxy",
              DisplayName="[Ck][RenderProxy] Handle -> RenderProxy Handle",
              meta = (CompactNodeTitle = "<AsRenderProxy>", BlueprintAutocast))
    static FCk_Handle_RenderProxy
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Invalid RenderProxy Handle",
              Category = "Ck|Utils|RenderProxy",
              meta = (CompactNodeTitle = "INVALID_RenderProxyHandle", Keywords = "make"))
    static FCk_Handle_RenderProxy
    Get_InvalidHandle() { return {}; }

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderProxy",
              DisplayName="[Ck][RenderProxy] Request Enable/Disable")
    static FCk_Handle_RenderProxy
    Request_EnableDisable(
        UPARAM(ref) FCk_Handle_RenderProxy& InHandle,
        const FCk_Request_RenderProxy_EnableDisable& InRequest);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderProxy",
              DisplayName="[Ck][RenderProxy] Get Mobility")
    static ECk_Mobility
    Get_Mobility(
        const FCk_Handle_RenderProxy& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderProxy",
              DisplayName="[Ck][RenderProxy] Get Mesh")
    static UStaticMesh*
    Get_Mesh(
        const FCk_Handle_RenderProxy& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderProxy",
              DisplayName="[Ck][RenderProxy] Get Bounds")
    static FBoxSphereBounds
    Get_Bounds(
        const FCk_Handle_RenderProxy& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderProxy",
              DisplayName="[Ck][RenderProxy] Get IsEnabled")
    static bool
    Get_IsEnabled(
        const FCk_Handle_RenderProxy& InHandle);
};

// --------------------------------------------------------------------------------------------------------------------