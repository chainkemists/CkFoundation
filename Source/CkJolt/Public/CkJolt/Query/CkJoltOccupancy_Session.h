#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>
#include <Templates/PimplPtr.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Jolt_Subsystem;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    /// A batch of world-space triangles read back out of the physics world, in unreal units.
    ///
    /// Indices are triples into _Vertices; _Indices.Num() is therefore always a multiple of three.
    /// Vertices are NOT welded across bodies or across shapes — a consumer that needs a welded mesh
    /// welds it itself, because the bake this exists for rasterizes triangles independently and
    /// would pay for welding it never reads.
    ///
    /// Deliberately a plain value type and NOT reflected: it routinely holds hundreds of thousands
    /// of vertices, and nothing about it belongs in a details panel or on the wire.
    struct CKJOLT_API FCk_Jolt_TriangleSoup
    {
    public:
        TArray<FVector> _Vertices;
        TArray<int32>   _Indices;

    public:
        auto Get_TriangleCount() const -> int32 { return _Indices.Num() / 3; }
        auto Get_IsEmpty() const -> bool { return _Indices.IsEmpty(); }

        auto Reset() -> void
        {
            _Vertices.Reset();
            _Indices.Reset();
        }
    };

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

    /// What ONE static body's collision geometry is a description OF, for a consumer that must treat a
    /// closed solid and an open surface differently. A consumer reading only faces knows a solid is solid
    /// through a CLOSED set of them, while a heightfield has no interior to describe and is legitimately
    /// open — so the two owe different guarantees and cannot share one answer.
    ///
    /// NotHeld is the honest answer for a body this session cannot read at all: not held by the physics
    /// world, not lockable, or not static. It is deliberately distinct from a body that reads fine and
    /// happens to contribute no geometry.
    enum class ECk_Jolt_StaticBodyKind : uint8
    {
        NotHeld,
        Solid,
        Surface
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

        /// Every world-space TRIANGLE of the static bodies overlapping InWorldBounds, appended to OutSoup;
        /// returns the number of triangles appended. The whole point of this call is that a consumer can
        /// rasterize real surface geometry without ever naming a Jolt type.
        ///
        /// STATIC BODIES ONLY, matching the broadphase enumeration above: a bake is a statement about
        /// immovable world geometry, and a moving obstacle is a steering problem, not a bake input.
        ///
        /// Triangles are clipped to nothing — a body that merely OVERLAPS the bounds contributes all of
        /// its triangles that Jolt reports for that box, so the result is conservative at the edges and
        /// the caller is responsible for discarding what falls outside its own region.
        ///
        /// OutSoup is APPENDED to, not reset, so a caller can accumulate several regions into one batch.
        /// Game thread only (off-thread only under a future step-barrier contract - not yet provided).
        auto
        Get_StaticTrianglesInAABox(
            const FBox& InWorldBounds,
            FCk_Jolt_TriangleSoup& OutSoup) const -> int32;

        /// Whether one static body is a closed solid or an open surface, decided by its LEAF shape: a
        /// decorator (rotate-translate, scale, offset-centre-of-mass) says where and how big a shape is
        /// and never what KIND it is, and a compound is a solid whatever it was compounded from.
        /// Game thread only (off-thread only under a future step-barrier contract — not yet provided).
        auto
        Get_StaticBodyKind(
            uint64 InBodyId) const -> ECk_Jolt_StaticBodyKind;

        /// One static body's world-space bounds, so a consumer holding an id can scope work to that body
        /// without re-entering the broadphase to find it again. Invalid (ForceInit) when the kind above
        /// reads NotHeld.
        /// Game thread only (off-thread only under a future step-barrier contract — not yet provided).
        auto
        Get_StaticBodyBounds(
            uint64 InBodyId) const -> FBox;

        /// EVERY world-space triangle of ONE static body, APPENDED to OutSoup; returns the number appended.
        ///
        /// UNCLIPPED, unlike the region form above, and that is the whole reason this call exists: a mesh
        /// clipped to a query box acquires cut edges that are indistinguishable from real holes, so anything
        /// asking whether a body is CLOSED has to see the body whole. The query volume is the body's own
        /// bounds with a small margin, so no float-edge comparison can drop a boundary triangle either.
        ///
        /// Correspondingly expensive — it walks one body's entire mesh — so it belongs on a per-build path
        /// and never inside a per-tile or per-frame loop. 0 when the kind above reads NotHeld.
        /// Game thread only (off-thread only under a future step-barrier contract — not yet provided).
        auto
        Get_StaticBodyTriangles(
            uint64 InBodyId,
            FCk_Jolt_TriangleSoup& OutSoup) const -> int32;

        /// A name for one static body that a developer can act on: the attribution entity and the source
        /// actor where the body carries them, and always the leaf shape's type plus the body's centre and
        /// extent, which is all a body with no ECS attribution can offer. Never empty.
        ///
        /// DIAGNOSTIC ONLY — the text is not stable across runs and nothing may key off it.
        /// Game thread only (off-thread only under a future step-barrier contract — not yet provided).
        auto
        Get_StaticBodyDescription(
            uint64 InBodyId) const -> FString;

        /// A token that changes whenever the static geometry this session can return changes: static body
        /// added, removed or re-typed, cooked static-world cell loaded or unloaded, probe churn.
        ///
        /// WORLD-WIDE, NOT REGION-SCOPED. It moves for a change anywhere in the world, including geometry
        /// no query of this session would ever have returned. A consumer comparing it across time learns
        /// only that SOMETHING changed — never what, and never where.
        ///
        /// STATIC ONLY. Dynamic and kinematic bodies coming and going do not move it — they are never
        /// returned by Get_StaticTrianglesInAABox, and a token that moved for every despawn would never hold
        /// still long enough for a consumer to build over several frames. The counter is monotonically
        /// increasing, so it can never return to a value it has already reported. Zero on an INVALID session
        /// (and, harmlessly, on a valid world nothing has changed in yet — validity is a separate question
        /// callers already have to ask).
        /// Game thread only (off-thread only under a future step-barrier contract — not yet provided).
        auto
        Get_StaticWorldRevision() const -> uint64;

    private:
        struct FImpl;
        TPimplPtr<FImpl> _Impl;
    };
}

// --------------------------------------------------------------------------------------------------------------------
