#pragma once

#include "CkCrowd/Agent/CkCrowdAgent_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkSpatialQuery/Probe/CkProbe_Fragment_Data.h"

#include <GameplayTagContainer.h>
#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

// Gameplay tag stamped on every crowd-agent probe (via _ProbeName + _Filter). Mutually filters
// crowd agents only — props, walls, projectiles do not have this tag and so do not show up in
// neighbor queries even though Jolt may report a geometric overlap.
CKCROWD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Crowd_Agent);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Marks an agent that has had its probe child entity spawned + SceneNode-parented. Set by
    // FProcessor_CrowdAgent_Setup once it has done the structural setup work; a TExclude on this
    // tag prevents the Setup body from running twice on the same agent (the dirty-tag pump can fire
    // multiple times before the registry settles).
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_HasProbe);

    // --------------------------------------------------------------------------------------------------------------------

    // Stores the probe child entity's typesafe handle so NeighborSync can read its overlaps without
    // having to walk Get_LifetimeDependents every frame. Populated by Setup; read-only thereafter.
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

    // Per-frame trimmed list of nearby agents, sorted by distance ascending. Capped at
    // _MaxNeighborsForSteering. Written by FProcessor_CrowdAgent_NeighborSync; read by Separation
    // and any gameplay code that wants to know "is anyone near me".
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

    // Output of the separation force solver. The steering view and gameplay code ReadOnly-depend on it.
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
