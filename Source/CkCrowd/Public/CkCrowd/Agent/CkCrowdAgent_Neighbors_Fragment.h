#pragma once

#include "CkCrowd/Agent/CkCrowdAgent_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkSpatialQuery/Probe/CkProbe_Fragment_Data.h"

#include <GameplayTagContainer.h>
#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

// Probe filter tag: untagged props/walls/projectiles never surface as neighbors even when Jolt
// reports a geometric overlap.
CKCROWD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Crowd_Agent);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Also Setup's own TExclude: the dirty-tag pump can fire multiple times before the registry
    // settles, and the Setup body must not run twice on one agent.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_HasProbe);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKCROWD_API FFragment_CrowdAgent_ProbeRef
    {
    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_ProbeRef);
        friend class FProcessor_CrowdAgent_Setup;
        friend class FProcessor_CrowdAgent_NeighborSync;

    private:
        FCk_Handle_Probe _ProbeChild;

    public:
        CK_PROPERTY_GET(_ProbeChild);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Rebuilt every frame: sorted by distance ascending, capped at _MaxNeighborsForSteering.
    struct CKCROWD_API FFragment_CrowdAgent_NeighborCache
    {
    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_NeighborCache);
        friend class FProcessor_CrowdAgent_NeighborSync;

    private:
        TArray<FCk_CrowdAgent_Neighbor> _Neighbors;

    public:
        CK_PROPERTY_GET(_Neighbors);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKCROWD_API FFragment_CrowdAgent_SeparationForce
    {
    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_SeparationForce);
        friend class FProcessor_CrowdAgent_Separation;

    private:
        FVector _Force = FVector::ZeroVector;

    public:
        CK_PROPERTY_GET(_Force);
    };
}

// --------------------------------------------------------------------------------------------------------------------
