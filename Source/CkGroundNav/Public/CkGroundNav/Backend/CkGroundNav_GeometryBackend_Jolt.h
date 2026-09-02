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
     * Region fetches and the WHOLE-BODY fetch are two different costs against the same world: the region
     * form is what every tile bakes from, while Get_BodyTriangles walks one body's entire mesh unclipped
     * so the closure check sees edges no query box has cut. Only the closure check pays for the latter,
     * once per body per build.
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

        auto Get_BodyKind(
            const FCk_GroundNav_BodyRef& InBody) const -> ECk_GroundNav_BodyKind override;

        auto Get_BodyBounds(
            const FCk_GroundNav_BodyRef& InBody) const -> FBox override;

        auto Get_BodyTriangles(
            const FCk_GroundNav_BodyRef& InBody,
            FCk_GroundNav_GeometryBatch& OutBatch) const -> int32 override;

        auto Get_BodyDescription(
            const FCk_GroundNav_BodyRef& InBody) const -> FString override;

    private:
        ck::jolt::FCk_Jolt_QuerySession _Session;
    };
}

// --------------------------------------------------------------------------------------------------------------------
