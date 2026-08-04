#include "CkJoltOccupancy_Utils.h"

#include "CkCore/Validation/CkIsValid_Defaults.h"

#include "CkJolt/CkJoltShapeFactory.h"
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/Query/CkJoltQuery_Data.h"

#include <Jolt/Geometry/AABox.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/RayCast.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    auto
        Get_IsBoxOccupied(
            const JPH::PhysicsSystem& InPhysicsSystem,
            const FVector& InCenter,
            const FVector& InHalfExtents,
            const JPH::BroadPhaseLayerFilter& InBroadPhaseFilter,
            const JPH::ObjectLayerFilter& InObjectFilter)
        -> bool
    {
        const auto BoxShape = CreateShape_FromDimensions(
            FCk_Jolt_ShapeDimensions{ECk_Jolt_ShapeType::Box}.Set_HalfExtents(InHalfExtents),
            TEXT("JoltOccupancy_Box"));

        return Get_IsBoxOccupied(InPhysicsSystem, BoxShape, InCenter, InBroadPhaseFilter, InObjectFilter);
    }

    auto
        Get_IsBoxOccupied(
            const JPH::PhysicsSystem& InPhysicsSystem,
            const JPH::Ref<JPH::Shape>& InBoxShape,
            const FVector& InCenter,
            const JPH::BroadPhaseLayerFilter& InBroadPhaseFilter,
            const JPH::ObjectLayerFilter& InObjectFilter)
        -> bool
    {
        if (InBoxShape == nullptr)
        { return false; }

        // A box's center of mass IS its center, so a plain translation is the center-of-mass transform.
        const auto Transform = JPH::RMat44::sTranslation(Conv(InCenter));

        auto Collector = JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector>{};

        InPhysicsSystem.GetNarrowPhaseQuery().CollideShape(
            InBoxShape.GetPtr(), JPH::Vec3::sReplicate(1.0f), Transform, JPH::CollideShapeSettings{},
            JPH::RVec3::sZero(), Collector, InBroadPhaseFilter, InObjectFilter);

        return Collector.HadHit();
    }

    auto
        Get_IsSegmentBlocked(
            const JPH::PhysicsSystem& InPhysicsSystem,
            const FVector& InFrom,
            const FVector& InTo,
            const JPH::BroadPhaseLayerFilter& InBroadPhaseFilter,
            const JPH::ObjectLayerFilter& InObjectFilter)
        -> bool
    {
        const auto Ray = JPH::RRayCast{Conv(InFrom), Conv(InTo - InFrom)};

        auto Collector = JPH::AnyHitCollisionCollector<JPH::CastRayCollector>{};

        InPhysicsSystem.GetNarrowPhaseQuery().CastRay(
            Ray, JPH::RayCastSettings{}, Collector, InBroadPhaseFilter, InObjectFilter);

        return Collector.HadHit();
    }

    auto
        Get_BodiesInAABox(
            const JPH::PhysicsSystem& InPhysicsSystem,
            const FBox& InWorldBounds,
            const JPH::BroadPhaseLayerFilter& InBroadPhaseFilter,
            const JPH::ObjectLayerFilter& InObjectFilter,
            TArray<JPH::BodyID>& OutBodyIds)
        -> void
    {
        OutBodyIds.Reset();

        if (ck::Is_NOT_Valid(InWorldBounds))
        { return; }

        const auto Box = JPH::AABox{Conv(InWorldBounds.Min), Conv(InWorldBounds.Max)};

        auto Collector = JPH::AllHitCollisionCollector<JPH::CollideShapeBodyCollector>{};

        InPhysicsSystem.GetBroadPhaseQuery().CollideAABox(Box, Collector, InBroadPhaseFilter, InObjectFilter);

        OutBodyIds.Reserve(Collector.mHits.size());

        for (const auto& BodyId : Collector.mHits)
        { OutBodyIds.Emplace(BodyId); }
    }
}

// --------------------------------------------------------------------------------------------------------------------
