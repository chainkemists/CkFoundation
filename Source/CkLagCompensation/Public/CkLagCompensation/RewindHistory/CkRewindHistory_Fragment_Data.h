#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkShapes/CkShapes_Common.h"

#include <GameplayTagContainer.h>

#include "CkRewindHistory_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKLAGCOMPENSATION_API FCk_Handle_RewindHistory : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_RewindHistory); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_RewindHistory);

// --------------------------------------------------------------------------------------------------------------------

/**
 * One hit shape recorded into the rewind history — a labelled primitive posed relative to the
 * entity's transform (e.g. a 'Head' sphere and a 'Body' capsule).
 */
USTRUCT(BlueprintType)
struct CKLAGCOMPENSATION_API FCk_LagComp_HitShape
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_LagComp_HitShape);

private:
    // Identifies which part of the target was hit (reported back in rewind hits)
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _Label;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_AnyShape _Shape;

    // Pose of the shape relative to the entity's transform
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FTransform _LocalOffset = FTransform::Identity;

public:
    CK_PROPERTY_GET(_Label);
    CK_PROPERTY_GET(_Shape);
    CK_PROPERTY(_LocalOffset);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_LagComp_HitShape, _Label, _Shape);
};

// --------------------------------------------------------------------------------------------------------------------

/** World-space pose of one hit shape at one recorded moment */
USTRUCT(BlueprintType)
struct CKLAGCOMPENSATION_API FCk_LagComp_HitShapeSnapshot
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_LagComp_HitShapeSnapshot);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _Label;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_AnyShape _Shape;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FTransform _WorldTransform = FTransform::Identity;

public:
    CK_PROPERTY_GET(_Label);
    CK_PROPERTY_GET(_Shape);
    CK_PROPERTY_GET(_WorldTransform);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_LagComp_HitShapeSnapshot, _Label, _Shape, _WorldTransform);
};

// --------------------------------------------------------------------------------------------------------------------

/** All hit shape poses of one entity at one recorded moment, with a bounding box for fast rejection */
USTRUCT(BlueprintType)
struct CKLAGCOMPENSATION_API FCk_LagComp_RewindFrame
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_LagComp_RewindFrame);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Time _WorldTime;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_LagComp_HitShapeSnapshot> _Snapshots;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FBox _Bounds = FBox{ForceInit};

public:
    CK_PROPERTY_GET(_WorldTime);
    CK_PROPERTY_GET(_Snapshots);
    CK_PROPERTY_GET(_Bounds);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_LagComp_RewindFrame, _WorldTime, _Snapshots, _Bounds);
};

// --------------------------------------------------------------------------------------------------------------------

/** A confirmed hit against a target's PAST hit shape pose */
USTRUCT(BlueprintType)
struct CKLAGCOMPENSATION_API FCk_LagComp_RewindHit
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_LagComp_RewindHit);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _TargetEntity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _HitShapeLabel;

    // World time at which the trajectory crossed the (rewound) hit shape
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Time _HitWorldTime;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FVector _HitLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FVector _HitNormal = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_TargetEntity);
    CK_PROPERTY_GET(_HitShapeLabel);
    CK_PROPERTY_GET(_HitWorldTime);
    CK_PROPERTY_GET(_HitLocation);
    CK_PROPERTY_GET(_HitNormal);

public:
    CK_PROPERTY_SET(_TargetEntity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_LagComp_RewindHit, _HitShapeLabel, _HitWorldTime, _HitLocation, _HitNormal);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Recording configuration. RecordInterval/RetentionPeriod of zero seconds (the default) defer to
 * the global CVars Ck.LagComp.RecordInterval / Ck.LagComp.RetainSeconds.
 */
USTRUCT(BlueprintType)
struct CKLAGCOMPENSATION_API FCk_RewindHistory_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RewindHistory_Spec);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_LagComp_HitShape> _HitShapes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _RecordInterval;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _RetentionPeriod;

public:
    CK_PROPERTY_GET(_HitShapes);
    CK_PROPERTY(_RecordInterval);
    CK_PROPERTY(_RetentionPeriod);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_RewindHistory_Spec, _HitShapes);
};

// --------------------------------------------------------------------------------------------------------------------
