#pragma once

#include "CkGroundNav/Backend/CkGroundNav_GeometryBackend.h"

#include "CkJolt/Query/CkJoltOccupancy_Session.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * The production geometry surface: the physics world, reached ONLY through CkJolt's JPH-free query
     * session. No Jolt type is named here or anywhere else in this module — that separation is what lets
     * every bake stage be tested against the stub with no physics at all.
     *
     * A session that cannot resolve its physics world is a legitimate state, so callers MUST gate on
     * Get_IsValid() and bake the tile as Unbuilt rather than as empty.
     *
     * Game thread only, whole type.
     */
    class CKGROUNDNAV_API FCk_GroundNav_GeometryBackend_Jolt final : public ICk_GroundNav_GeometryBackend
    {
    public:
        FCk_GroundNav_GeometryBackend_Jolt() = default;

        explicit FCk_GroundNav_GeometryBackend_Jolt(
            const UObject* InWorldContextObject);

    public:
        auto Get_IsValid() const -> bool override;

        auto Get_HasGeometryInBounds(
            const FBox& InBounds) const -> bool override;

        auto Get_StaticBodiesInBounds(
            const FBox& InBounds,
            TArray<FCk_GroundNav_BodyRef>& OutBodies) const -> int32 override;

        auto Get_TrianglesInBounds(
            const FBox& InBounds,
            FCk_GroundNav_GeometryBatch& OutBatch) const -> int32 override;

        auto Get_WorldRevision() const -> uint64 override;

    private:
        ck::jolt::FCk_Jolt_QuerySession _Session;
    };
}

// --------------------------------------------------------------------------------------------------------------------
