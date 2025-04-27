#pragma once

#include "CkIsmRenderer/Renderer/CkIsmRenderer_Fragment_Data.h"

#include "CkIsmRenderer_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKISMRENDERER_API UCk_Utils_IsmRenderer_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IsmRenderer_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_IsmRenderer);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IsmRenderer",
              DisplayName="[Ck][IsmRenderer] Add Feature")
    static FCk_Handle_IsmRenderer
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const UCk_IsmRenderer_Data* InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmRenderer",
              DisplayName="[Ck][IsmRenderer] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IsmRenderer",
              DisplayName="[Ck][IsmRenderer] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_IsmRenderer
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmRenderer",
              DisplayName="[Ck][IsmRenderer] Handle -> IsmRenderer Handle",
              meta = (CompactNodeTitle = "<AsIsmRenderer>", BlueprintAutocast))
    static FCk_Handle_IsmRenderer
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid IsmRenderer Handle",
        Category = "Ck|Utils|IsmRenderer",
        meta = (CompactNodeTitle = "INVALID_IsmRendererHandle", Keywords = "make"))
    static FCk_Handle_IsmRenderer
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmRenderer",
              DisplayName="[Ck][IsmRenderer] Get Current Instance Count")
    static int32
    Get_CurrentInstanceCount(
        const FCk_Handle_IsmRenderer& InHandle);
};

// --------------------------------------------------------------------------------------------------------------------