#pragma once

#include "CoreMinimal.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include <NativeGameplayTags.h>

#include "CkCrowdAvoidanceVolume_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

CKCROWD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Crowd_AvoidanceVolume);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKCROWD_API FCk_Handle_CrowdAvoidanceVolume : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_CrowdAvoidanceVolume);
};
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_CrowdAvoidanceVolume);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_CrowdAvoidanceVolume_DebugState : uint8
{
    PendingSetup,
    PendingNavigationConfirmation,
    Confirmed,
    Invalid,
    Retiring
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_CrowdAvoidanceVolume_DebugState);

// --------------------------------------------------------------------------------------------------------------------

// Authored route policy. Recast stores an area ID per poly, so each policy maps to a finite nav-area
// class; a volume cannot supply an arbitrary per-instance numeric traversal cost.
UENUM(BlueprintType)
enum class ECk_CrowdAvoidanceVolume_TraversalPolicy : uint8
{
    // First plan excludes this volume; the permissive retry may traverse its finite-cost area.
    AvoidIfPossible,

    // Every Crowd Recast phase excludes this volume. Agents already inside still use escape handling.
    HardExclude,

    // Never excludes this volume; its finite-cost area simply discourages traversal.
    CostOnly
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_CrowdAvoidanceVolume_TraversalPolicy);

// --------------------------------------------------------------------------------------------------------------------

// Detached diagnostic data. Each record is a copied value view of either a live avoidance volume or a
// world-scoped retirement record; it never retains an ECS handle, registry, fragment reference, or UObject.
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_CrowdAvoidanceVolume_DebugSnapshot
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_CrowdAvoidanceVolume_DebugSnapshot);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int64 _VolumeIdentity = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FName _VolumeDebugName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FTransform _YawWorldTransform = FTransform::Identity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _PhysicalWorldHalfExtents = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _InfluenceWorldHalfExtents = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _PaintedWorldHalfExtents = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _SecondsSincePaint = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int64 _ConfirmationSerial = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int64 _NavigationRevisionAtUnregister = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_CrowdAvoidanceVolume_DebugState _State = ECk_CrowdAvoidanceVolume_DebugState::PendingSetup;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _HasValidGeometry = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_CrowdAvoidanceVolume_TraversalPolicy _TraversalPolicy =
        ECk_CrowdAvoidanceVolume_TraversalPolicy::AvoidIfPossible;

public:
    CK_PROPERTY_GET(_VolumeIdentity);
    CK_PROPERTY_GET(_VolumeDebugName);
    CK_PROPERTY_GET(_YawWorldTransform);
    CK_PROPERTY_GET(_PhysicalWorldHalfExtents);
    CK_PROPERTY_GET(_InfluenceWorldHalfExtents);
    CK_PROPERTY_GET(_PaintedWorldHalfExtents);
    CK_PROPERTY_GET(_SecondsSincePaint);
    CK_PROPERTY_GET(_ConfirmationSerial);
    CK_PROPERTY_GET(_NavigationRevisionAtUnregister);
    CK_PROPERTY_GET(_State);
    CK_PROPERTY_GET(_HasValidGeometry);
    CK_PROPERTY_GET(_TraversalPolicy);

public:
    FCk_CrowdAvoidanceVolume_DebugSnapshot() = default;

    FCk_CrowdAvoidanceVolume_DebugSnapshot(
        int64 InVolumeIdentity,
        FName InVolumeDebugName,
        FTransform InYawWorldTransform,
        FVector InPhysicalWorldHalfExtents,
        FVector InInfluenceWorldHalfExtents,
        FVector InPaintedWorldHalfExtents,
        float InSecondsSincePaint,
        int64 InConfirmationSerial,
        int64 InNavigationRevisionAtUnregister,
        ECk_CrowdAvoidanceVolume_DebugState InState,
        bool InHasValidGeometry,
        ECk_CrowdAvoidanceVolume_TraversalPolicy InTraversalPolicy)
        : _VolumeIdentity(InVolumeIdentity)
        , _VolumeDebugName(InVolumeDebugName)
        , _YawWorldTransform(MoveTemp(InYawWorldTransform))
        , _PhysicalWorldHalfExtents(MoveTemp(InPhysicalWorldHalfExtents))
        , _InfluenceWorldHalfExtents(MoveTemp(InInfluenceWorldHalfExtents))
        , _PaintedWorldHalfExtents(MoveTemp(InPaintedWorldHalfExtents))
        , _SecondsSincePaint(InSecondsSincePaint)
        , _ConfirmationSerial(InConfirmationSerial)
        , _NavigationRevisionAtUnregister(InNavigationRevisionAtUnregister)
        , _State(InState)
        , _HasValidGeometry(InHasValidGeometry)
        , _TraversalPolicy(InTraversalPolicy)
    {}
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Fragment_CrowdAvoidanceVolume_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_CrowdAvoidanceVolume_ParamsData);

private:
    // The physical footprint. The owning Transform is the box centre; only its yaw is meaningful
    // to the ground-plane solver.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _HalfExtents = FVector{50.0, 50.0, 100.0};

    // Extra XY reach for the query-only probe. This never changes collision or navigation; it only
    // gives the local sampler time to choose a path around the physical footprint.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", AllowPrivateAccess = true))
    float _InfluenceRange = 400.0f;

    // XY expansion painted into Recast so centre-line paths remain outside ordinary crowd-agent
    // bodies. Projects with larger agents can raise this authored clearance per volume.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", AllowPrivateAccess = true))
    float _PathPlanningClearance = 50.0f;

    // This selects one of CkCrowd's finite nav-area classes. It is intentionally not an arbitrary
    // numeric cost: Recast paints only an area ID per poly, and DefaultCost belongs to that class.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_CrowdAvoidanceVolume_TraversalPolicy _TraversalPolicy =
        ECk_CrowdAvoidanceVolume_TraversalPolicy::AvoidIfPossible;

public:
    CK_PROPERTY(_HalfExtents);
    CK_PROPERTY(_InfluenceRange);
    CK_PROPERTY(_PathPlanningClearance);
    CK_PROPERTY(_TraversalPolicy);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_CrowdAvoidanceVolume_ParamsData, _HalfExtents, _InfluenceRange);
};

// --------------------------------------------------------------------------------------------------------------------
