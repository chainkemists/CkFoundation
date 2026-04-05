#pragma once

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkShapes/CkShapes_Common.h"

#include "CkShapes_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Shape"))
class CKSHAPES_API UCk_Utils_Shapes_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Shapes_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Shape);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Shapes",
              DisplayName = "[Ck][Shapes] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Shapes",
              DisplayName = "[Ck][Shapes] Add Feature")
    static FCk_Handle_Shape
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_AnyShape& InParams);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Shapes] Get Type",
              Category = "Ck|Utils|Shapes")
    static ECk_Shape_Type
    Get_ShapeType(
        const FCk_Handle_Shape& InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Shapes] Get Radius",
              Category = "Ck|Utils|Shapes")
    static float
    Get_Radius(
        const FCk_Handle_Shape& InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Shapes] Make AnyShape (Box)",
              Category = "Ck|Utils|Shapes")
    static FCk_AnyShape
    Make_Box(
        const FCk_ShapeBox_Dimensions& InDimensions);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Shapes] Make AnyShape (Sphere)",
              Category = "Ck|Utils|Shapes")
    static FCk_AnyShape
    Make_Sphere(
        const FCk_ShapeSphere_Dimensions& InDimensions);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Shapes] Make AnyShape (Capsule)",
              Category = "Ck|Utils|Shapes")
    static FCk_AnyShape
    Make_Capsule(
        const FCk_ShapeCapsule_Dimensions& InDimensions);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Shapes] Make AnyShape (Cylinder)",
              Category = "Ck|Utils|Shapes")
    static FCk_AnyShape
    Make_Cylinder(
        const FCk_ShapeCylinder_Dimensions& InDimensions);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Shapes",
              DisplayName = "[Ck][Shapes] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Shape
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Shapes",
              DisplayName = "[Ck][Shapes] Handle -> Shape Handle",
              meta = (CompactNodeTitle = "<AsShape>", BlueprintAutocast))
    static FCk_Handle_Shape
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Invalid Shape Handle",
              Category = "Ck|Utils|Shapes",
              meta = (CompactNodeTitle = "INVALID_ShapeHandle", Keywords = "make"))
    static FCk_Handle_Shape
    Get_InvalidHandle() { return {}; };
};

// --------------------------------------------------------------------------------------------------------------------