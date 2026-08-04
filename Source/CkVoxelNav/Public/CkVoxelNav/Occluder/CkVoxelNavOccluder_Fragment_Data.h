#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkVoxelNavOccluder_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKVOXELNAV_API FCk_Handle_VoxelNavOccluder : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_VoxelNavOccluder); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_VoxelNavOccluder);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVOXELNAV_API FCk_Fragment_VoxelNavOccluder_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_VoxelNavOccluder_ParamsData);

private:
    /** The occluder's extents in its OWN space, half-width per axis. The feature tracks the entity's
     *  transform and derives world bounds from these, so an occluder needs no physics body, no primitive
     *  component and no actor of its own to be tracked.
     *
     *  Deliberately authored rather than measured off a component: the box a repair dirties should be the
     *  region the obstacle blocks NAVIGATION in, which for a thin or oddly-shaped mesh is a very different
     *  thing from its render bounds. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _HalfExtentsUu = FVector{50.0};

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _MovementThresholdOverride = ECk_EnableDisable::Disable;

    /** How far this occluder's world bounds must move before it dirties anything, when it overrides the
     *  project default. Below the threshold nothing happens at all: a repair that re-probes a region to
     *  change no voxel costs exactly as much as one that changes it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _MovementThresholdUuOverride = 10.0f;

public:
    CK_PROPERTY_GET(_HalfExtentsUu);
    CK_PROPERTY(_MovementThresholdOverride);
    CK_PROPERTY(_MovementThresholdUuOverride);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_VoxelNavOccluder_ParamsData, _HalfExtentsUu);
};

// --------------------------------------------------------------------------------------------------------------------
