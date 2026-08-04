#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>
#include <Templates/PimplPtr.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Jolt_Subsystem;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    /// One box query volume of a fixed size, built ONCE and reused across many occupancy queries. Callers
    /// that probe a grid keep one probe per distinct cell size instead of rebuilding a shape per query.
    ///
    /// Move-only value type; the physics shape is held behind an opaque impl so consumers outside this
    /// module never name a Jolt type.
    ///
    /// Game thread only, whole type (off-thread only under a future step-barrier contract — not yet
    /// provided).
    class CKJOLT_API FCk_Jolt_BoxProbe
    {
    public:
        CK_GENERATED_BODY(FCk_Jolt_BoxProbe);

    public:
        friend class FCk_Jolt_QuerySession;

    public:
        /// Default-constructed probes are INVALID and every query against them reads as unoccupied.
        FCk_Jolt_BoxProbe();

        /// Half the box's edge lengths, in UE centimeters — the same convention as
        /// FCk_Jolt_ShapeDimensions. Any clearance inflation belongs in the value passed here.
        /// Game thread only (off-thread only under a future step-barrier contract — not yet provided).
        explicit FCk_Jolt_BoxProbe(
            const FVector& InHalfExtents);

        auto
        Get_IsValid() const -> bool;

    private:
        FVector _HalfExtents = FVector::ZeroVector;

        struct FImpl;
        TPimplPtr<FImpl> _Impl;

    public:
        CK_PROPERTY_GET(_HalfExtents);
    };

    // ----------------------------------------------------------------------------------------------------------------

    /// The Jolt world's occupancy surface WITHOUT any Jolt type in sight: resolve once, query thousands of
    /// times. Construction pins nothing permanently — it resolves the Jolt subsystem, keeps a weak
    /// reference to its physics world, and builds the static-domain query filters up front, so a query
    /// costs one narrowphase call and no subsystem lookups.
    ///
    /// Queries are restricted to the STATIC body domain: baked level geometry plus Static-motion-type
    /// bodies. Both are immovable world obstacles, which is what a coverage or navigation bake wants.
    ///
    /// Every query returns "empty" (unoccupied / no bodies) while the session is invalid — a world with no
    /// Jolt subsystem is a legitimate state, so callers MUST gate on Get_IsValid() and report their own
    /// failure rather than treating an unresolved session as an empty world.
    ///
    /// Sessions are cheap to build and NOT meant to be cached across level transitions: validity follows
    /// the physics world that produced it.
    ///
    /// Game thread only, whole type (off-thread only under a future step-barrier contract — not yet
    /// provided).
    class CKJOLT_API FCk_Jolt_QuerySession
    {
    public:
        CK_GENERATED_BODY(FCk_Jolt_QuerySession);

    public:
        /// Default-constructed sessions are INVALID.
        FCk_Jolt_QuerySession();

        /// Resolves the Jolt subsystem from the world context; an unresolvable one yields an INVALID
        /// session rather than an ensure — worlds without a Jolt subsystem are legitimate.
        /// Game thread only (off-thread only under a future step-barrier contract — not yet provided).
        explicit FCk_Jolt_QuerySession(
            const UObject* InWorldContextObject);

        /// Game thread only (off-thread only under a future step-barrier contract — not yet provided).
        explicit FCk_Jolt_QuerySession(
            UCk_Jolt_Subsystem* InJoltSubsystem);

        auto
        Get_IsValid() const -> bool;

        /// True when any static geometry overlaps the probe's box placed at InCenter. The hot-loop form:
        /// no allocation, first-hit early-out, no entity resolution.
        /// Game thread only (off-thread only under a future step-barrier contract — not yet provided).
        auto
        Get_IsBoxOccupied(
            const FCk_Jolt_BoxProbe& InProbe,
            const FVector& InCenter) const -> bool;

        /// Same test for a one-off size. Builds a box shape per call — use the probe overload in a loop.
        /// Game thread only (off-thread only under a future step-barrier contract — not yet provided).
        auto
        Get_IsBoxOccupied(
            const FVector& InCenter,
            const FVector& InHalfExtents) const -> bool;

        /// True when any static geometry stands between InFrom and InTo. A line-of-sight test, not a
        /// raycast: any-hit with early-out, and the answer carries no hit position, no normal and no body
        /// identity. A segment whose start point is already inside a convex body reads as BLOCKED.
        /// Game thread only (off-thread only under a future step-barrier contract — not yet provided).
        auto
        Get_IsSegmentBlocked(
            const FVector& InFrom,
            const FVector& InTo) const -> bool;

        /// Every static body whose BOUNDING BOX overlaps InWorldBounds — broadphase only, so the result is
        /// conservative and cheap; ideal as a whole-region early-out. The ids are opaque handles onto Jolt
        /// bodies: compare and pass them back, never decode them. OutBodyIds is reset before filling.
        /// Game thread only (off-thread only under a future step-barrier contract — not yet provided).
        auto
        Get_BodiesInAABox(
            const FBox& InWorldBounds,
            TArray<uint64>& OutBodyIds) const -> void;

    private:
        struct FImpl;
        TPimplPtr<FImpl> _Impl;
    };
}

// --------------------------------------------------------------------------------------------------------------------
