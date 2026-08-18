#pragma once

#include "CkShapes/Box/CkShapeBox_Fragment_Data.h"
#include "CkShapes/Capsule/CkShapeCapsule_Fragment_Data.h"
#include "CkShapes/Cylinder/CkShapeCylinder_Fragment_Data.h"
#include "CkShapes/Sphere/CkShapeSphere_Fragment_Data.h"

#include "CkShapes_Common.generated.h"

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
              meta = (AllowPrivateAccess = true, EditCondition="_ShapeType == ECk_Shape_Type::Capsule", EditConditionHides))
    FCk_ShapeCapsule_Dimensions _Capsule;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition="_ShapeType == ECk_Shape_Type::Cylinder", EditConditionHides))
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

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Shape_FromMeshFidelity : uint8
{
    Exact,
    // One primitive, but the scale/rotation pair is one an axis-aligned shape cannot represent:
    // a non-uniformly scaled sphere is an ellipsoid, a rotated box shears.
    Approximated,
    PrimitiveUnion,
    // Convex hull, tri-mesh, or no authored collision at all.
    VisualBounds,
    None UMETA(DisplayName = "No Shape"),
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Shape_FromMeshFidelity);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSHAPES_API FCk_Shape_FromMeshResult
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Shape_FromMeshResult);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_AnyShape _Shape;

    // UNSCALED, unlike the dimensions: the usual consumer composes this under a node that already
    // applies the mesh scale, so scaling both would apply it twice.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FTransform _LocalOffset = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Shape_FromMeshFidelity _Fidelity = ECk_Shape_FromMeshFidelity::None;

public:
    CK_PROPERTY_GET(_Shape);
    CK_PROPERTY_GET(_LocalOffset);
    CK_PROPERTY_GET(_Fidelity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Shape_FromMeshResult, _Shape, _LocalOffset, _Fidelity);
};

// --------------------------------------------------------------------------------------------------------------------
