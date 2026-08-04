#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>

// --------------------------------------------------------------------------------------------------------------------

// ReSharper disable once CppInconsistentNaming
namespace JPH
{
    class PhysicsSystem;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    /// Occupancy primitives: boolean "is there geometry here" and broadphase body gathering, for callers
    /// that issue thousands of queries per frame (volumetric navigation bakes, coverage grids).
    ///
    /// They deliberately do LESS than UCk_Utils_JoltQuery_UE: any-hit collection with an early-out instead
    /// of collecting every hit, NO ECS entity attribution, NO per-call subsystem resolution, and the caller
    /// owns both the query shape and the filters. The JPH-free consumer surface over these lives in
    /// CkJoltOccupancy_Session.h — prefer that unless you already hold a JPH::PhysicsSystem.
    ///
    /// GAME THREAD ONLY, as with every other Jolt query in this module: safe off-thread only under a future
    /// step-barrier contract — not yet provided.

    /// True when any body passing the filters overlaps the axis-aligned box. Builds a JPH::BoxShape per
    /// call — for repeated queries at one size, hoist the shape and use the JPH::Ref overload below.
    /// Game thread only (off-thread only under a future step-barrier contract — not yet provided).
    CKJOLT_API auto Get_IsBoxOccupied(
        const JPH::PhysicsSystem& InPhysicsSystem,
        const FVector& InCenter,
        const FVector& InHalfExtents,
        const JPH::BroadPhaseLayerFilter& InBroadPhaseFilter,
        const JPH::ObjectLayerFilter& InObjectFilter) -> bool;

    /// True when any body passing the filters overlaps the caller-held box shape placed (unrotated) at
    /// InCenter. The shape's half-extents ARE the query box — boxes need no axis correction. A null shape
    /// reads as unoccupied (shape creation already ensured at its own call site).
    /// Game thread only (off-thread only under a future step-barrier contract — not yet provided).
    CKJOLT_API auto Get_IsBoxOccupied(
        const JPH::PhysicsSystem& InPhysicsSystem,
        const JPH::Ref<JPH::Shape>& InBoxShape,
        const FVector& InCenter,
        const JPH::BroadPhaseLayerFilter& InBroadPhaseFilter,
        const JPH::ObjectLayerFilter& InObjectFilter) -> bool;

    /// True when any body passing the filters stands between InFrom and InTo. A line-of-sight test, not a
    /// raycast: any-hit with early-out, no fraction, no normal, no body identity. A segment whose start
    /// point is already inside a convex body reads as BLOCKED — that is Jolt's convex-as-solid default and
    /// it is the answer a visibility test wants, since a viewer buried in geometry can see nothing.
    /// Game thread only (off-thread only under a future step-barrier contract — not yet provided).
    CKJOLT_API auto Get_IsSegmentBlocked(
        const JPH::PhysicsSystem& InPhysicsSystem,
        const FVector& InFrom,
        const FVector& InTo,
        const JPH::BroadPhaseLayerFilter& InBroadPhaseFilter,
        const JPH::ObjectLayerFilter& InObjectFilter) -> bool;

    /// Every body whose BOUNDING BOX overlaps InWorldBounds — broadphase only, no shape test, so the
    /// result is conservative. OutBodyIds is reset before filling, so callers can reuse one array.
    /// Game thread only (off-thread only under a future step-barrier contract — not yet provided).
    CKJOLT_API auto Get_BodiesInAABox(
        const JPH::PhysicsSystem& InPhysicsSystem,
        const FBox& InWorldBounds,
        const JPH::BroadPhaseLayerFilter& InBroadPhaseFilter,
        const JPH::ObjectLayerFilter& InObjectFilter,
        TArray<JPH::BodyID>& OutBodyIds) -> void;
}

// --------------------------------------------------------------------------------------------------------------------
