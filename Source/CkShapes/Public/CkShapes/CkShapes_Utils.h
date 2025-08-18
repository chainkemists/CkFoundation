#pragma once

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkShapes/CkShapes_Common.h"

#include "CkShapes_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKSHAPES_API UCk_Utils_Shapes_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Shapes_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Shapes",
              DisplayName="[Ck][Shapes] Add Feature")
    static FCk_Handle
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_AnyShape& InParams);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][Shapes] Has Feature",
              Category = "Ck|Utils|Shapes")
    static bool
    Has_Any(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][Shapes] Get Type",
              Category = "Ck|Utils|Shapes")
    static ECk_Shape_Type
    Get_ShapeType(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][Shapes] Get Radius",
              Category = "Ck|Utils|Shapes")
    static float
    Get_Radius(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][Shapes] Make AnyShape (Box)",
              Category = "Ck|Utils|Shapes")
    static FCk_AnyShape
    Make_Box(
        const FCk_ShapeBox_Dimensions& InDimensions);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][Shapes] Make AnyShape (Sphere)",
              Category = "Ck|Utils|Shapes")
    static FCk_AnyShape
    Make_Sphere(
        const FCk_ShapeSphere_Dimensions& InDimensions);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][Shapes] Make AnyShape (Capsule)",
              Category = "Ck|Utils|Shapes")
    static FCk_AnyShape
    Make_Capsule(
        const FCk_ShapeCapsule_Dimensions& InDimensions);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][Shapes] Make AnyShape (Cylinder)",
              Category = "Ck|Utils|Shapes")
    static FCk_AnyShape
    Make_Cylinder(
        const FCk_ShapeCylinder_Dimensions& InDimensions);
};

// --------------------------------------------------------------------------------------------------------------------