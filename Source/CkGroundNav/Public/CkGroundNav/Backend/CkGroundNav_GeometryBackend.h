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
     * The ONLY world-geometry surface the bake sees. Everything a bake learns about the world arrives
     * through these three calls, which is what keeps the ground field free of any physics backend and
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
    };
}

// --------------------------------------------------------------------------------------------------------------------
