#pragma once

#include "CoreMinimal.h"
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

public:
    CK_PROPERTY(_HalfExtents);
    CK_PROPERTY(_InfluenceRange);
    CK_PROPERTY(_PathPlanningClearance);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_CrowdAvoidanceVolume_ParamsData, _HalfExtents, _InfluenceRange);
};

// --------------------------------------------------------------------------------------------------------------------
