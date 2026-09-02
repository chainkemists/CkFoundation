#include "CkJoltOccupancy_Session.h"

#include "CkCore/Validation/CkIsValid_Defaults.h"

#include "CkJolt/CkJoltShapeFactory.h"
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/CollisionLayers/CkJoltCollisionLayerTable.h"
#include "CkJolt/Query/CkJoltOccupancy_Utils.h"
#include "CkJolt/Query/CkJoltQuery_Data.h"
#include "CkJolt/Subsystem/CkJolt_Subsystem.h"

#include <Engine/World.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>
#include <Jolt/Geometry/AABox.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_occupancy_session
{
    // The subsystem half of ck_jolt_query_utils::Get_QueryContext — resolved ONCE per session instead of
    // once per query, and without the ECS world (occupancy never attributes hits to entities).
    static auto TryGet_JoltSubsystem(const UObject* InWorldContextObject) -> UCk_Jolt_Subsystem*
    {
        if (ck::Is_NOT_Valid(InWorldContextObject))
        { return nullptr; }

        auto* World = InWorldContextObject->GetWorld();
        if (ck::Is_NOT_Valid(World))
        { return nullptr; }

        return World->GetSubsystem<UCk_Jolt_Subsystem>();
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    struct FCk_Jolt_BoxProbe::FImpl
    {
        explicit FImpl(
            const JPH::Ref<JPH::Shape>& InShape)
            : _Shape(InShape)
        { }

        JPH::Ref<JPH::Shape> _Shape;
    };

    // ----------------------------------------------------------------------------------------------------------------

    FCk_Jolt_BoxProbe::
        FCk_Jolt_BoxProbe() = default;

    FCk_Jolt_BoxProbe::
        FCk_Jolt_BoxProbe(
            const FVector& InHalfExtents)
        : _HalfExtents(InHalfExtents)
    {
        const auto BoxShape = CreateShape_FromDimensions(
            FCk_Jolt_ShapeDimensions{ECk_Jolt_ShapeType::Box}.Set_HalfExtents(InHalfExtents),
            TEXT("JoltOccupancy_BoxProbe"));

        if (BoxShape == nullptr)
        { return; }

        _Impl = MakePimpl<FImpl>(BoxShape);
    }

    auto
        FCk_Jolt_BoxProbe::
        Get_IsValid() const
        -> bool
    {
        return ck::IsValid(_Impl, ck::IsValid_Policy_NullptrOnly{});
    }

    // ----------------------------------------------------------------------------------------------------------------

    struct FCk_Jolt_QuerySession::FImpl
    {
        FImpl(
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem,
            UCk_Jolt_Subsystem* InJoltSubsystem,
            const FCk_Jolt_CollisionLayerTable& InLayerTable)
            : _PhysicsSystem(InPhysicsSystem)
            , _JoltSubsystem(InJoltSubsystem)
            , _ObjectFilter(InLayerTable)
        { }

        // Weak, so a session never keeps the physics world alive past its subsystem. The layer table the
        // filters reference is destroyed AFTER the PhysicsSystem in UCk_Jolt_Subsystem::Deinitialize, so a
        // successful Pin() is also proof that the filters' table reference is still live.
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;

        // Weak for the same reason, and only ever read for the world's change tokens — every query goes
        // through the pinned PhysicsSystem above and never re-enters the subsystem.
        TWeakObjectPtr<UCk_Jolt_Subsystem> _JoltSubsystem;

        FCk_Jolt_StaticBroadPhaseQueryFilter _BroadPhaseFilter;
        FCk_Jolt_StaticOccupancyFilter _ObjectFilter;
    };

    // ----------------------------------------------------------------------------------------------------------------

    FCk_Jolt_QuerySession::
        FCk_Jolt_QuerySession() = default;

    FCk_Jolt_QuerySession::
        FCk_Jolt_QuerySession(
            const UObject* InWorldContextObject)
        : FCk_Jolt_QuerySession(ck_jolt_occupancy_session::TryGet_JoltSubsystem(InWorldContextObject))
    { }

    FCk_Jolt_QuerySession::
        FCk_Jolt_QuerySession(
            UCk_Jolt_Subsystem* InJoltSubsystem)
    {
        if (ck::Is_NOT_Valid(InJoltSubsystem))
        { return; }

        const auto PhysicsSystem = InJoltSubsystem->Get_PhysicsSystem().Pin();

        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return; }

        _Impl = MakePimpl<FImpl>(TWeakPtr<JPH::PhysicsSystem>{PhysicsSystem}, InJoltSubsystem,
            InJoltSubsystem->Get_LayerTable());
    }

    auto
        FCk_Jolt_QuerySession::
        Get_IsValid() const
        -> bool
    {
        return ck::IsValid(_Impl, ck::IsValid_Policy_NullptrOnly{}) && ck::IsValid(_Impl->_PhysicsSystem);
    }

    auto
        FCk_Jolt_QuerySession::
        Get_IsBoxOccupied(
            const FCk_Jolt_BoxProbe& InProbe,
            const FVector& InCenter) const
        -> bool
    {
        if (ck::Is_NOT_Valid(_Impl, ck::IsValid_Policy_NullptrOnly{}) || NOT InProbe.Get_IsValid())
        { return false; }

        const auto PhysicsSystem = _Impl->_PhysicsSystem.Pin();

        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return false; }

        return ck::jolt::Get_IsBoxOccupied(*PhysicsSystem, InProbe._Impl->_Shape, InCenter,
            _Impl->_BroadPhaseFilter, _Impl->_ObjectFilter);
    }

    auto
        FCk_Jolt_QuerySession::
        Get_IsBoxOccupied(
            const FVector& InCenter,
            const FVector& InHalfExtents) const
        -> bool
    {
        if (ck::Is_NOT_Valid(_Impl, ck::IsValid_Policy_NullptrOnly{}))
        { return false; }

        const auto PhysicsSystem = _Impl->_PhysicsSystem.Pin();

        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return false; }

        return ck::jolt::Get_IsBoxOccupied(*PhysicsSystem, InCenter, InHalfExtents,
            _Impl->_BroadPhaseFilter, _Impl->_ObjectFilter);
    }

    auto
        FCk_Jolt_QuerySession::
        Get_IsSegmentBlocked(
            const FVector& InFrom,
            const FVector& InTo) const
        -> bool
    {
        if (ck::Is_NOT_Valid(_Impl, ck::IsValid_Policy_NullptrOnly{}))
        { return false; }

        const auto PhysicsSystem = _Impl->_PhysicsSystem.Pin();

        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return false; }

        return ck::jolt::Get_IsSegmentBlocked(*PhysicsSystem, InFrom, InTo,
            _Impl->_BroadPhaseFilter, _Impl->_ObjectFilter);
    }

    auto
        FCk_Jolt_QuerySession::
        Get_BodiesInAABox(
            const FBox& InWorldBounds,
            TArray<uint64>& OutBodyIds) const
        -> void
    {
        OutBodyIds.Reset();

        if (ck::Is_NOT_Valid(_Impl, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        const auto PhysicsSystem = _Impl->_PhysicsSystem.Pin();

        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return; }

        auto BodyIds = TArray<JPH::BodyID>{};
        ck::jolt::Get_BodiesInAABox(*PhysicsSystem, InWorldBounds,
            _Impl->_BroadPhaseFilter, _Impl->_ObjectFilter, BodyIds);

        OutBodyIds.Reserve(BodyIds.Num());

        for (const auto& BodyId : BodyIds)
        { OutBodyIds.Emplace(BodyId.GetIndexAndSequenceNumber()); }
    }

    auto
        FCk_Jolt_QuerySession::
        Get_StaticTrianglesInAABox(
            const FBox& InWorldBounds,
            FCk_Jolt_TriangleSoup& OutSoup) const
        -> int32
    {
        if (ck::Is_NOT_Valid(_Impl, ck::IsValid_Policy_NullptrOnly{}))
        { return 0; }

        const auto PhysicsSystem = _Impl->_PhysicsSystem.Pin();

        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return 0; }

        auto BodyIds = TArray<JPH::BodyID>{};
        ck::jolt::Get_BodiesInAABox(*PhysicsSystem, InWorldBounds,
            _Impl->_BroadPhaseFilter, _Impl->_ObjectFilter, BodyIds);

        if (BodyIds.IsEmpty())
        { return 0; }

        const auto QueryBox = JPH::AABox{Conv(InWorldBounds.Min), Conv(InWorldBounds.Max)};
        const auto& BodyLockInterface = PhysicsSystem->GetBodyLockInterface();

        // Jolt requires at least cGetTrianglesMinTrianglesRequested per call and writes 3 vertices per
        // triangle, so the scratch buffer is sized once for the whole sweep rather than per body.
        constexpr auto BatchSize = 256;
        static_assert(BatchSize >= JPH::Shape::cGetTrianglesMinTrianglesRequested);

        auto TriangleVertices = TArray<JPH::Float3>{};
        TriangleVertices.SetNumUninitialized(BatchSize * 3);

        auto TrianglesAppended = 0;

        for (const auto& BodyId : BodyIds)
        {
            const auto Lock = JPH::BodyLockRead{BodyLockInterface, BodyId};

            if (NOT Lock.Succeeded())
            { continue; }

            const auto& Body = Lock.GetBody();

            // Static-only is the contract, and it is enforced here rather than trusted from the caller's
            // filters: a kinematic body that slipped through the object filter would bake a floor that
            // then drives away, which is invisible until an agent walks through the hole it leaves.
            if (NOT Body.IsStatic())
            { continue; }

            const auto* Shape = Body.GetShape();

            if (ck::Is_NOT_Valid(Shape, ck::IsValid_Policy_NullptrOnly{}))
            { continue; }

            auto Context = JPH::Shape::GetTrianglesContext{};
            Shape->GetTrianglesStart(Context, QueryBox,
                Body.GetCenterOfMassPosition(), Body.GetRotation(), JPH::Vec3::sReplicate(1.0f));

            for (;;)
            {
                const auto NumTriangles = Shape->GetTrianglesNext(Context, BatchSize, TriangleVertices.GetData());

                if (NumTriangles == 0)
                { break; }

                const auto FirstVertex = OutSoup._Vertices.Num();

                OutSoup._Vertices.Reserve(FirstVertex + (NumTriangles * 3));
                OutSoup._Indices.Reserve(OutSoup._Indices.Num() + (NumTriangles * 3));

                for (auto VertexIndex = 0; VertexIndex < NumTriangles * 3; ++VertexIndex)
                {
                    OutSoup._Vertices.Emplace(Conv(TriangleVertices[VertexIndex]));
                    OutSoup._Indices.Emplace(FirstVertex + VertexIndex);
                }

                TrianglesAppended += NumTriangles;
            }
        }

        return TrianglesAppended;
    }

    auto
        FCk_Jolt_QuerySession::
        Get_StaticWorldRevision() const
        -> uint64
    {
        if (ck::Is_NOT_Valid(_Impl, ck::IsValid_Policy_NullptrOnly{}))
        { return 0; }

        const auto* JoltSubsystem = _Impl->_JoltSubsystem.Get();

        if (ck::Is_NOT_Valid(JoltSubsystem))
        { return 0; }

        // SUM rather than a hash: both counters only ever increase, so the sum increases whenever either
        // one does and no pair of distinct states can mix down to the same number. A hash could collide;
        // a shift-and-xor is not monotone at all.
        return JoltSubsystem->Get_StaticSceneRevision() + JoltSubsystem->Get_BodyRemovedRevision();
    }
}

// --------------------------------------------------------------------------------------------------------------------
