#pragma once

#include "CkIsmRenderer/Proxy/CkIsmProxy_Fragment_Data.h"

#include "CkIsmProxy_Utils.generated.h"

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
              Category = "Ck|Utils|IsmRenderer|Proxy",
              DisplayName="[Ck][IsmProxy] Add Feature")
    static FCk_Handle_IsmProxy
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_IsmProxy_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmRenderer|Proxy",
              DisplayName="[Ck][IsmProxy] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IsmRenderer|Proxy",
              DisplayName="[Ck][IsmProxy] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_IsmProxy
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmRenderer|Proxy",
              DisplayName="[Ck][IsmProxy] Handle -> IsmProxy Handle",
              meta = (CompactNodeTitle = "<AsIsmProxy>", BlueprintAutocast))
    static FCk_Handle_IsmProxy
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmRenderer|Proxy",
              DisplayName="[Ck][IsmProxy] Get Associated Renderer Name")
    static FGameplayTag
    Get_RendererName(
        const FCk_Handle_IsmProxy& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmRenderer|Proxy",
              DisplayName="[Ck][IsmProxy] Get Relative Transform")
    static FTransform
    Get_RelativeTransform(
        const FCk_Handle_IsmProxy& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmRenderer|Proxy",
              DisplayName="[Ck][IsmProxy] Get Mobility")
    static ECk_Mobility
    Get_Mobility(
        const FCk_Handle_IsmProxy& InHandle);
};

// --------------------------------------------------------------------------------------------------------------------
