#pragma once

#include "CkShapes/Sphere/CkShapeSphere_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkNet/CkNet_Utils.h"

#include "CkShapeSphere_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKSHAPES_API UCk_Utils_ShapeSphere_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ShapeSphere_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_ShapeSphere);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ShapeSphere",
              DisplayName="[Ck][Shapes][Sphere] Add Feature")
    static FCk_Handle_ShapeSphere
    Add(
        UPARAM(ref) FCk_Handle InHandle,
        const FCk_Fragment_ShapeSphere_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ShapeSphere",
              DisplayName="[Ck][Shapes][Sphere] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ShapeSphere",
        DisplayName="[Ck][ShapeSphere] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_ShapeSphere
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ShapeSphere",
        DisplayName="[Ck][ShapeSphere] Handle -> ShapeSphere Handle",
        meta = (CompactNodeTitle = "<AsShapeSphere>", BlueprintAutocast))
    static FCk_Handle_ShapeSphere
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ShapeSphere",
        DisplayName="[Ck][ShapeSphere] Request ExampleRequest")
    static FCk_Handle_ShapeSphere
    Request_UpdateShape(
        UPARAM(ref) FCk_Handle_ShapeSphere& InShapeSphere,
        const FCk_Request_ShapeSphere_UpdateShape& InRequest);

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ShapeSphere",
        DisplayName="[Ck][ShapeSphere] Request ExampleRequest")
    static FCk_Fragment_ShapeSphere_ShapeData
    Get_ShapeData(
        const FCk_Handle_ShapeSphere& InShapeSphere);
};

// --------------------------------------------------------------------------------------------------------------------
