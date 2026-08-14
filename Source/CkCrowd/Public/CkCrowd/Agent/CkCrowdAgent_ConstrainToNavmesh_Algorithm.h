#pragma once

#include "CoreMinimal.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::ck_crowd_agent_constrain_to_navmesh_algorithm
{
    // Crowd-agent transforms use a feet anchor. Once navigation returns the surface location
    // reached by the requested XY walk, the complete offset must land those feet on that surface.
    // Passing the integrator's free-space Z through here lets turn-limited vertical momentum carry
    // an agent below the projection extent, permanently disabling the navmesh constraint.
    inline auto ResolveSurfaceOffset(
        const FVector& InFeetLocation,
        const FVector& InSurfaceLocation) -> FVector
    {
        const auto InputsAreValid =
            NOT InFeetLocation.ContainsNaN() &&
            NOT InSurfaceLocation.ContainsNaN();
        CK_ENSURE_IF_NOT(
            InputsAreValid,
            TEXT("Invalid crowd navmesh constraint inputs (feet [{}], surface [{}])"),
            InFeetLocation,
            InSurfaceLocation)
        { return FVector::ZeroVector; }

        return InSurfaceLocation - InFeetLocation;
    }
}

// --------------------------------------------------------------------------------------------------------------------
