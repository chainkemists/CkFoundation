#pragma once

#include "CkCore/Enums/CkEnums.h"

#include "CkPhysics_Common.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ShapeType : uint8
{
    Box,
    Sphere,
    Capsule
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ShapeType);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_CollisionDetectionType : uint8
{
    /* Does NOT use Continuous Collision Detection*/
    Normal,

    /* Use Continuous Collision Detection (expensive, use sparingly)*/
    CCD,

    /* Use Continuous Collision Detection on all bodies in component (potentially very expensive)*/
    CCD_All
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_CollisionDetectionType);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_NavigationEffect : uint8
{
    DoesNotAffectNavigation,
    AffectsNavigation
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_NavigationEffect);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ComponentOverlapBehavior : uint8
{
    /* Component can overlap with Components of other Actors only.*/
    OtherActorComponents,
    /* Component can overlap with Components of all Actors.*/
    AllActorComponents
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ComponentOverlapBehavior);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPHYSICS_API FCk_BoxExtents
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_BoxExtents);

public:
    FCk_BoxExtents() = default;
    FCk_BoxExtents(
        FVector InExtents,
        ECk_ScaledUnscaled InScalesUnscaled);

private:
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _Extents = FVector::ZeroVector;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_ScaledUnscaled _ScaledUnscaled = ECk_ScaledUnscaled::Scaled;

public:
    CK_PROPERTY_GET(_Extents);
    CK_PROPERTY_GET(_ScaledUnscaled);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPHYSICS_API FCk_SphereRadius
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SphereRadius);

public:
    FCk_SphereRadius() = default;
    FCk_SphereRadius(
        float InRadius,
        ECk_ScaledUnscaled InScaledUnscaled);

private:
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = true, Units = "cm"))
    float _Radius = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_ScaledUnscaled _ScaledUnscaled = ECk_ScaledUnscaled::Scaled;

public:
    CK_PROPERTY_GET(_Radius);
    CK_PROPERTY_GET(_ScaledUnscaled);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPHYSICS_API FCk_CapsuleSize
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_CapsuleSize);

public:
    FCk_CapsuleSize() = default;
    FCk_CapsuleSize(
        float InRadius,
        float InHalfHeight,
        ECk_ScaledUnscaled InScaledUnscaled);

private:
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = true, Units = "cm"))
    float _Radius = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = true, Units = "cm"))
    float _HalfHeight = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_ScaledUnscaled _ScaledUnscaled = ECk_ScaledUnscaled::Scaled;

public:
    CK_PROPERTY_GET(_Radius);
    CK_PROPERTY_GET(_HalfHeight);
    CK_PROPERTY_GET(_ScaledUnscaled);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake))
struct CKPHYSICS_API FCk_ShapeDimensions
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ShapeDimensions);

public:
    FCk_ShapeDimensions() = default;
    explicit FCk_ShapeDimensions(FCk_BoxExtents InBoxExtents);
    explicit FCk_ShapeDimensions(FCk_SphereRadius InSphereRadius);
    explicit FCk_ShapeDimensions(FCk_CapsuleSize InCapsuleSize);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ShapeType _ShapeType = ECk_ShapeType::Box;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition="_ShapeType == ECk_ShapeType::Box", EditConditionHides))
    FCk_BoxExtents _BoxExtents;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition="_ShapeType == ECk_ShapeType::Sphere", EditConditionHides))
    FCk_SphereRadius _SphereRadius;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition="_ShapeType == ECk_ShapeType::Capsule" ,EditConditionHides))
    FCk_CapsuleSize _CapsuleSize;

public:
    CK_PROPERTY_GET(_ShapeType);
    CK_PROPERTY_GET(_BoxExtents);
    CK_PROPERTY_GET(_SphereRadius);
    CK_PROPERTY_GET(_CapsuleSize);
};

// --------------------------------------------------------------------------------------------------------------------
