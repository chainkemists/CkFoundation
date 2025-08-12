#pragma once

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkShapes/Box/CkShapeBox_Fragment_Data.h"
#include "CkShapes/Capsule/CkShapeCapsule_Fragment_Data.h"
#include "CkShapes/Cylinder/CkShapeCylinder_Fragment_Data.h"
#include "CkShapes/Sphere/CkShapeSphere_Fragment_Data.h"

#include "CkShapes_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Shape_Type : uint8
{
    Box,
    Capsule,
    Cylinder,
    Sphere,

    None UMETA(DisplayName = "No Shape"),
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Shape_Type);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake))
struct CKSHAPES_API FCk_AnyShape
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_AnyShape);

public:
    FCk_AnyShape() = default;
    explicit FCk_AnyShape(FCk_ShapeBox_Dimensions InDimensions);
    explicit FCk_AnyShape(FCk_ShapeCapsule_Dimensions InDimensions);
    explicit FCk_AnyShape(FCk_ShapeCylinder_Dimensions InDimensions);
    explicit FCk_AnyShape(FCk_ShapeSphere_Dimensions InDimensions);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Shape_Type _ShapeType = ECk_Shape_Type::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition="_ShapeType == ECk_Shape_Type::Box", EditConditionHides))
    FCk_ShapeBox_Dimensions _Box;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition="_ShapeType == ECk_Shape_Type::Capsule" ,EditConditionHides))
    FCk_ShapeCapsule_Dimensions _Capsule;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition="_ShapeType == ECk_Shape_Type::Capsule" ,EditConditionHides))
    FCk_ShapeCylinder_Dimensions _Cylinder;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition="_ShapeType == ECk_Shape_Type::Sphere", EditConditionHides))
    FCk_ShapeSphere_Dimensions _Sphere;

public:
    CK_PROPERTY_GET(_ShapeType);
    CK_PROPERTY_GET(_Box);
    CK_PROPERTY_GET(_Capsule);
    CK_PROPERTY_GET(_Cylinder);
    CK_PROPERTY_GET(_Sphere);
};

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