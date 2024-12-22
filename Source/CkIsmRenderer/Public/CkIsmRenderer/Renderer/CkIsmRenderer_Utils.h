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
        const FCk_Fragment_IsmRenderer_ParamsData& InParams);

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

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmRenderer",
              DisplayName="[Ck][IsmRenderer] Get Renderer Name")
    static FGameplayTag
    Get_RendererName(
        const FCk_Handle_IsmRenderer& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmRenderer",
              DisplayName="[Ck][IsmRenderer] Get Mesh To Render")
    static UStaticMesh*
    Get_MeshToRender(
        const FCk_Handle_IsmRenderer& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmRenderer",
              DisplayName="[Ck][IsmRenderer] Get Mesh Shadow Casting")
    static ECk_EnableDisable
    Get_MeshShadowCasting(
        const FCk_Handle_IsmRenderer& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmRenderer",
              DisplayName="[Ck][IsmRenderer] Get Mesh Collision")
    static ECk_Collision
    Get_MeshCollision(
        const FCk_Handle_IsmRenderer& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IsmRenderer",
              DisplayName="[Ck][IsmRenderer] Get Mesh Num Custom Data")
    static int32
    Get_MeshNumCustomData(
        const FCk_Handle_IsmRenderer& InHandle);
};

// --------------------------------------------------------------------------------------------------------------------