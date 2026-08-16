#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include <GameplayTagContainer.h>
#include <NativeGameplayTags.h>

#include "CkCrowdAgent_Avoidance_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Designer opt-in, honoured on the agent or anywhere in its lifetime-owner chain: forces sampling on.
CKCROWD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_CrowdAvoidance_AlwaysSample);

// Designer opt-out, same chain: forces force-only behaviour whatever the neighbor count.
CKCROWD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_CrowdAvoidance_NeverSample);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AvoidancePolicy : uint8
{
    UseProjectDefault,  // honour _AvoidanceSampleTrigger + tags
    ForceOnly,          // never sample (overrides project + zone tag)
    SamplingAlways,     // always sample (overrides everything)
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Fragment_CrowdAgent_AvoidancePolicy
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_CrowdAgent_AvoidancePolicy);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    ECk_AvoidancePolicy _Policy = ECk_AvoidancePolicy::UseProjectDefault;

public:
    CK_PROPERTY(_Policy);
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_CrowdAgent_AvoidancePolicy, _Policy);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_CrowdAgent_AvoidancePolicy = FCk_Fragment_CrowdAgent_AvoidancePolicy;

    // --------------------------------------------------------------------------------------------------------------------

    // The navmesh boundary walls near an agent, cached exactly the way dtCrowd caches them in
    // dtLocalBoundary (DetourCrowd/DetourLocalBoundary.cpp): the nearest MAX_LOCAL_SEGS = 8 wall
    // segments inside the collision query range. FProcessor_CrowdAgent_AvoidanceSample refreshes it
    // once the agent has travelled a quarter of that range away from _Centre, which is dtCrowd's own
    // trigger (DetourCrowd.cpp:1284-1286). _Valid is false until a query has succeeded, so a failed
    // one is retried on the next sampling frame rather than leaving the agent wall-blind until it
    // has moved a quarter of the range.
    struct CKCROWD_API FFragment_CrowdAgent_LocalBoundary
    {
    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_LocalBoundary);

        friend class FProcessor_CrowdAgent_AvoidanceSample;

        // dtLocalBoundary::MAX_LOCAL_SEGS
        static constexpr auto MaxSegments = 8;

        // Endpoints are in the source polygon's vertex winding, which is what makes the outward
        // normal derivable - see MakeWallOutwardNormal in CkCrowdAgent_AvoidanceSample_Algorithm.h.
        struct FSegment
        {
            FVector _Start = FVector::ZeroVector;
            FVector _End = FVector::ZeroVector;
        };

        using SegmentsType = TArray<FSegment, TInlineAllocator<MaxSegments>>;

    private:
        FVector _Centre = FVector::ZeroVector;
        SegmentsType _Segments;
        bool _Valid = false;

        // Time since the last successful refresh. dtLocalBoundary also refreshes when its cached poly
        // refs stop validating (a tile rebuild); FindEdges hands back no refs to validate, so a
        // stationary agent whose surroundings were repainted under it (fixture paint, markup tiles)
        // would otherwise keep a wall-less cache forever. A time cap bounds that staleness instead.
        float _SecondsSinceRefresh = 0.0f;

    public:
        CK_PROPERTY_GET(_Centre);
        CK_PROPERTY_GET(_Segments);
        CK_PROPERTY_GET(_Valid);
        CK_PROPERTY_GET(_SecondsSinceRefresh);
    };
}
