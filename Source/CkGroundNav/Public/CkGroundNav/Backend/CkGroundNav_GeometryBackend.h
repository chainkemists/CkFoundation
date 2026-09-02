#pragma once

#include "CkGroundNav/Bake/CkGroundNav_GeometryBatch.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * Opaque identity of one piece of world geometry, minted by the backend that returned it. The bake
     * never decodes one — it counts them, compares them, and hands them back. Zero is the never-a-body
     * sentinel.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_BodyRef
    {
    public:
        uint64 _Value = 0;

    public:
        auto Get_IsValid() const -> bool { return _Value != 0; }
        auto operator==(const FCk_GroundNav_BodyRef&) const -> bool = default;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * What a body's triangles are a description OF.
     *
     * The bake sees faces and never a body's interior, so a solid is known to be solid only through its
     * faces — which is why a Solid body must be CLOSED (every edge shared by exactly two triangles). A
     * wall with no underside presents no face in the column beneath it and bakes as open ground.
     *
     * A Surface (a heightfield, terrain) is open by construction and legitimately so: it has no interior
     * to describe, its steep parts rasterize as unwalkable spans on slope alone, and a hole in it is a
     * hole in the world. It is exempt from the closure contract.
     */
    enum class ECk_GroundNav_BodyKind : uint8
    {
        Solid,
        Surface
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The ONLY world-geometry surface the bake sees. Everything a bake learns about the world arrives
     * through these calls, which is what keeps the ground field free of any physics backend and
     * lets a hand-authored box list stand in for a whole level in a test.
     *
     * STATIC GEOMETRY ONLY. A bake is a statement about immovable world obstacles; a moving obstacle is
     * served by runtime markup, never by the bake.
     *
     * Game thread only, whole type (off-thread only under a future step-barrier contract — not yet
     * provided).
     */
    class CKGROUNDNAV_API ICk_GroundNav_GeometryBackend
    {
    public:
        virtual ~ICk_GroundNav_GeometryBackend() = default;

    public:
        /**
         * Is the backend able to answer at all? A backend that cannot reach its physics world is a
         * legitimate state: the tile bakes as Unbuilt, and NEVER as Built-with-zero-plates, because those
         * two are indistinguishable to every downstream query and only one of them is honest.
         */
        virtual auto
        Get_IsValid() const -> bool = 0;

        /**
         * Cheap conservative prefilter: does any static geometry overlap InBounds at all? A whole tile
         * that answers false needs no collection pass and bakes as empty free space.
         */
        virtual auto
        Get_HasGeometryInBounds(
            const FBox& InBounds) const -> bool = 0;

        /**
         * Every static body whose BOUNDING BOX overlaps InBounds — broadphase only, so the answer is
         * conservative and cheap. Returns the number appended; OutBodies is reset before filling.
         */
        virtual auto
        Get_StaticBodiesInBounds(
            const FBox& InBounds,
            TArray<FCk_GroundNav_BodyRef>& OutBodies) const -> int32 = 0;

        /**
         * Every world-space triangle of the static geometry overlapping InBounds, APPENDED to OutBatch;
         * returns the number of triangles appended.
         *
         * Bounds-scoped rather than per-body: the physics surface underneath answers per region, so a
         * per-body fetch would re-enter the broadphase once per body to collect what one sweep already
         * has. Body enumeration stays available above for identity and counting.
         */
        virtual auto
        Get_TrianglesInBounds(
            const FBox& InBounds,
            FCk_GroundNav_GeometryBatch& OutBatch) const -> int32 = 0;

        /**
         * A token that changes whenever any static geometry the backend could return changes. It MAY
         * change for geometry outside any bounds ever queried — it is a statement about the backend's
         * whole world, not about a region.
         *
         * A sliced build compares it across slices and FAILS CLOSED on a mismatch, because tiles baked
         * against two different worlds agree on nothing at their shared seam: the portals derived from
         * their stubs are the one structure with no local evidence that it is wrong.
         */
        virtual auto
        Get_WorldRevision() const -> uint64 = 0;

        /** Whether a body is a solid that must be closed, or a surface that is exempt. */
        virtual auto
        Get_BodyKind(
            const FCk_GroundNav_BodyRef& InBody) const -> ECk_GroundNav_BodyKind = 0;

        /** The body's world-space bounds. Invalid (ForceInit) for a body the backend no longer holds. */
        virtual auto
        Get_BodyBounds(
            const FCk_GroundNav_BodyRef& InBody) const -> FBox = 0;

        /**
         * EVERY world-space triangle of ONE body, APPENDED to OutBatch; returns the number appended.
         *
         * Unclipped on purpose: the closure check needs the whole body, because a body clipped to a
         * region has cut edges that look exactly like real holes. This is the one per-body fetch, and it
         * runs once per body per build, never per tile.
         */
        virtual auto
        Get_BodyTriangles(
            const FCk_GroundNav_BodyRef& InBody,
            FCk_GroundNav_GeometryBatch& OutBatch) const -> int32 = 0;

        /**
         * A human-readable name for one body, for a diagnostic a developer has to act on: the owning
         * actor or entity where there is one, the shape and bounds where there is not. Never empty for a
         * body the backend holds.
         */
        virtual auto
        Get_BodyDescription(
            const FCk_GroundNav_BodyRef& InBody) const -> FString = 0;
    };
}

// --------------------------------------------------------------------------------------------------------------------
