#pragma once

#include "CkIsmRenderer/CkIsmRenderer_Fragment_Data.h"

#include "CkIsmRenderer_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKISMRENDERER_API UCk_Utils_AntAgent_Renderer_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AntAgent_Renderer_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_IsmRenderer);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Renderer|ISM",
              DisplayName="[Ck][IsmRenderer] Add Feature")
    static FCk_Handle_IsmRenderer
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_IsmRenderer_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Renderer|ISM",
              DisplayName="[Ck][IsmRenderer] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Renderer|ISM",
              DisplayName="[Ck][IsmRenderer] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_IsmRenderer
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Renderer|ISM",
              DisplayName="[Ck][IsmRenderer] Handle -> AntAgentRenderer Handle",
              meta = (CompactNodeTitle = "<AsAntAgentRenderer>", BlueprintAutocast))
    static FCk_Handle_IsmRenderer
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Renderer|ISM",
              DisplayName="[Ck][IsmRenderer] Request Render Instance",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IsmRenderer
    Request_RenderInstance(
        UPARAM(ref) FCk_Handle_IsmRenderer& InHandle,
        const FCk_Request_IsmRenderer_NewInstance& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKISMRENDERER_API UCk_Utils_IsmProxy_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IsmProxy_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_IsmProxy);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Renderer|ISM",
              DisplayName="[Ck][IsmProxy] Add Feature")
    static FCk_Handle_IsmProxy
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_IsmProxy_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Renderer|ISM",
              DisplayName="[Ck][IsmProxy] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Renderer|ISM",
              DisplayName="[Ck][IsmProxy] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_IsmProxy
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Renderer|ISM",
              DisplayName="[Ck][IsmProxy] Handle -> AntAgentRenderer Handle",
              meta = (CompactNodeTitle = "<AsAntAgentRenderer>", BlueprintAutocast))
    static FCk_Handle_IsmProxy
    DoCastChecked(
        FCk_Handle InHandle);
};
