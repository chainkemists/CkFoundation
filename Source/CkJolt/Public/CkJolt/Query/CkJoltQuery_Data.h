#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkJolt/CollisionLayers/CkJoltCollisionLayer_Data.h"

#include "CkJoltQuery_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Jolt_ShapeType : uint8
{
    Box,
    Sphere,
    Capsule,
    Cylinder
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Jolt_ShapeType);

// --------------------------------------------------------------------------------------------------------------------

/// CkJolt-owned shape description (deliberately NOT CkShapes fragments — CkJolt sits below the
/// CkShapes feature). Box uses _HalfExtents; Sphere uses _Radius; Capsule/Cylinder use _Radius +
/// _HalfHeight (capsule half-height = cylinder section only, matching UE's FKSphylElem).
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Jolt_ShapeDimensions
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Jolt_ShapeDimensions);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Jolt_ShapeType _ShapeType = ECk_Jolt_ShapeType::Box;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true,
              EditCondition = "_ShapeType == ECk_Jolt_ShapeType::Box"))
    FVector _HalfExtents = FVector{50.0};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = 0.1,
              EditCondition = "_ShapeType != ECk_Jolt_ShapeType::Box"))
    float _Radius = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = 0.1,
              EditCondition = "_ShapeType == ECk_Jolt_ShapeType::Capsule || _ShapeType == ECk_Jolt_ShapeType::Cylinder"))
    float _HalfHeight = 50.0f;

public:
    CK_PROPERTY(_ShapeType);
    CK_PROPERTY(_HalfExtents);
    CK_PROPERTY(_Radius);
    CK_PROPERTY(_HalfHeight);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Jolt_ShapeDimensions, _ShapeType);
};

// --------------------------------------------------------------------------------------------------------------------

/// UE-channel-equivalent query filtering: a body is considered when its authored response to
/// _Channel is at least _MinResponse (Block = trace semantics, Overlap = overlap semantics).
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Jolt_QueryFilter
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Jolt_QueryFilter);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TEnumAsByte<ECollisionChannel> _Channel = ECC_Visibility;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Jolt_PairInteraction _MinResponse = ECk_Jolt_PairInteraction::Block;

public:
    CK_PROPERTY(_Channel);
    CK_PROPERTY(_MinResponse);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Jolt_HitResult
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Jolt_HitResult);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _HasHit = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _Position = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _Normal = FVector::UpVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _Fraction = 0.0f;

    // Valid when the hit body belongs to a live entity — probes, dynamic JoltBodies, AND baked-static source
    // actors (each baked body carries its source actor's attribution entity id as Jolt user-data).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle _Entity;

public:
    CK_PROPERTY(_HasHit);
    CK_PROPERTY(_Position);
    CK_PROPERTY(_Normal);
    CK_PROPERTY(_Fraction);
    CK_PROPERTY(_Entity);
};

// --------------------------------------------------------------------------------------------------------------------
