#include "CkJoltOccupancy_Session.h"

#include "CkCore/Validation/CkIsValid_Defaults.h"

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Registry/CkRegistry.h"

#include "CkJolt/CkJoltShapeFactory.h"
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/CollisionLayers/CkJoltCollisionLayerTable.h"
#include "CkJolt/Query/CkJoltOccupancy_Utils.h"
#include "CkJolt/Query/CkJoltQuery_Data.h"
#include "CkJolt/StaticWorld/CkJoltStaticActor_Fragment.h"
#include "CkJolt/Subsystem/CkJolt_Subsystem.h"

#include <Engine/World.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Collision/Shape/DecoratedShape.h>
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

    // Jolt requires at least cGetTrianglesMinTrianglesRequested per call and writes 3 vertices per
    // triangle, so the scratch buffer is sized once per sweep rather than per body.
    constexpr auto TriangleBatchSize = 256;
    static_assert(TriangleBatchSize >= JPH::Shape::cGetTrianglesMinTrianglesRequested);

    // The ids this session vends are BodyID::GetIndexAndSequenceNumber widened for the JPH-free surface,
    // so narrowing is the exact inverse of how they were minted and not a lossy cast.
    static auto Conv_BodyId(uint64 InBodyId) -> JPH::BodyID
    {
        return JPH::BodyID{static_cast<JPH::uint32>(InBodyId)};
    }

    // A decorator says where a shape is and how big, never WHAT it is, so the kind of a body is the kind
    // of whatever sits at the bottom of its decorator chain.
    static auto Get_LeafShape(const JPH::Shape* InShape) -> const JPH::Shape*
    {
        const auto* Leaf = InShape;

        for (;;)
        {
            const auto SubType = Leaf->GetSubType();

            if (SubType == JPH::EShapeSubType::RotatedTranslated ||
                SubType == JPH::EShapeSubType::Scaled ||
                SubType == JPH::EShapeSubType::OffsetCenterOfMass)
            {
                Leaf = static_cast<const JPH::DecoratedShape*>(Leaf)->GetInnerShape();
                continue;
            }

            return Leaf;
        }
    }

    static auto Get_ShapeSubTypeName(const JPH::Shape* InShape) -> FString
    {
        const auto SubTypeIndex = static_cast<uint32>(InShape->GetSubType());

        if (SubTypeIndex >= JPH::NumSubShapeTypes)
        { return TEXT("UnknownShape"); }

        return FString{ANSI_TO_TCHAR(JPH::sSubShapeTypeNames[SubTypeIndex])};
    }

    static auto TryResolve_Handle(uint64 InUserData, const FCk_Handle& InTransientEntity) -> FCk_Handle
    {
        if (ck::Is_NOT_Valid(InTransientEntity) || InUserData == 0)
        { return {}; }

        const auto Entity = FCk_Entity{FCk_Entity::IdType{static_cast<FCk_Entity::IdType>(InUserData)}};

        if (NOT InTransientEntity.Get_RegistryView().IsValid(Entity))
        { return {}; }

        return InTransientEntity.Get_ValidHandle(Entity.Get_ID());
    }

    // The actionable half of a body's description: WHO it belongs to. A body carries its attribution
    // entity in its user data, and a baked static-world body carries its source actor's name on top of
    // that — the name a developer types into the outliner to find the asset at fault.
    static auto Get_BodyOwnerName(uint64 InUserData, const FCk_Handle& InTransientEntity) -> FString
    {
        const auto Entity = TryResolve_Handle(InUserData, InTransientEntity);

        if (ck::Is_NOT_Valid(Entity))
        { return {}; }

        const auto DebugName = UCk_Utils_Handle_UE::Get_DebugName(Entity).ToString();

        const auto SourceActorName = Entity.Has<ck::FFragment_JoltStaticActor_Current>()
            ? Entity.Get<ck::FFragment_JoltStaticActor_Current>().Get_SourceActorName()
            : FName{};

        if (SourceActorName.IsNone())
        { return DebugName; }

        if (DebugName.IsEmpty() || DebugName == SourceActorName.ToString())
        { return SourceActorName.ToString(); }

        return FString::Printf(TEXT("%s (%s)"), *DebugName, *SourceActorName.ToString());
    }

    // The per-body half of every triangle read, shared by the region sweep and the whole-body fetch so the
    // two can never disagree about winding, scale or batching. InScratchVertices belongs to the caller
    // precisely so a sweep over many bodies pays for it once.
    static auto DoAppend_BodyTriangles(
        const JPH::Body& InBody,
        const JPH::AABox& InQueryBox,
        TArray<JPH::Float3>& InScratchVertices,
        ck::jolt::FCk_Jolt_TriangleSoup& OutSoup) -> int32
    {
        const auto* Shape = InBody.GetShape();

        if (ck::Is_NOT_Valid(Shape, ck::IsValid_Policy_NullptrOnly{}))
        { return 0; }

        auto Context = JPH::Shape::GetTrianglesContext{};
        Shape->GetTrianglesStart(Context, InQueryBox,
            InBody.GetCenterOfMassPosition(), InBody.GetRotation(), JPH::Vec3::sReplicate(1.0f));

        auto TrianglesAppended = 0;

        for (;;)
        {
            const auto NumTriangles = Shape->GetTrianglesNext(Context, TriangleBatchSize, InScratchVertices.GetData());

            if (NumTriangles == 0)
            { break; }

            const auto FirstVertex = OutSoup._Vertices.Num();

            OutSoup._Vertices.Reserve(FirstVertex + (NumTriangles * 3));
            OutSoup._Indices.Reserve(OutSoup._Indices.Num() + (NumTriangles * 3));

            for (auto VertexIndex = 0; VertexIndex < NumTriangles * 3; ++VertexIndex)
            {
                OutSoup._Vertices.Emplace(ck::jolt::Conv(InScratchVertices[VertexIndex]));
                OutSoup._Indices.Emplace(FirstVertex + VertexIndex);
            }

            TrianglesAppended += NumTriangles;
        }

        return TrianglesAppended;
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

        auto TriangleVertices = TArray<JPH::Float3>{};
        TriangleVertices.SetNumUninitialized(ck_jolt_occupancy_session::TriangleBatchSize * 3);

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

            TrianglesAppended += ck_jolt_occupancy_session::DoAppend_BodyTriangles(
                Body, QueryBox, TriangleVertices, OutSoup);
        }

        return TrianglesAppended;
    }

    auto
        FCk_Jolt_QuerySession::
        Get_StaticBodyKind(
            uint64 InBodyId) const
        -> ECk_Jolt_StaticBodyKind
    {
        if (ck::Is_NOT_Valid(_Impl, ck::IsValid_Policy_NullptrOnly{}))
        { return ECk_Jolt_StaticBodyKind::NotHeld; }

        const auto PhysicsSystem = _Impl->_PhysicsSystem.Pin();

        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return ECk_Jolt_StaticBodyKind::NotHeld; }

        const auto Lock = JPH::BodyLockRead{PhysicsSystem->GetBodyLockInterface(),
            ck_jolt_occupancy_session::Conv_BodyId(InBodyId)};

        if (NOT Lock.Succeeded())
        { return ECk_Jolt_StaticBodyKind::NotHeld; }

        const auto& Body = Lock.GetBody();

        if (NOT Body.IsStatic())
        { return ECk_Jolt_StaticBodyKind::NotHeld; }

        const auto* Shape = Body.GetShape();

        if (ck::Is_NOT_Valid(Shape, ck::IsValid_Policy_NullptrOnly{}))
        { return ECk_Jolt_StaticBodyKind::NotHeld; }

        const auto* LeafShape = ck_jolt_occupancy_session::Get_LeafShape(Shape);

        return LeafShape->GetSubType() == JPH::EShapeSubType::HeightField
            ? ECk_Jolt_StaticBodyKind::Surface
            : ECk_Jolt_StaticBodyKind::Solid;
    }

    auto
        FCk_Jolt_QuerySession::
        Get_StaticBodyBounds(
            uint64 InBodyId) const
        -> FBox
    {
        if (ck::Is_NOT_Valid(_Impl, ck::IsValid_Policy_NullptrOnly{}))
        { return FBox{ForceInit}; }

        const auto PhysicsSystem = _Impl->_PhysicsSystem.Pin();

        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return FBox{ForceInit}; }

        const auto Lock = JPH::BodyLockRead{PhysicsSystem->GetBodyLockInterface(),
            ck_jolt_occupancy_session::Conv_BodyId(InBodyId)};

        if (NOT Lock.Succeeded())
        { return FBox{ForceInit}; }

        const auto& Body = Lock.GetBody();

        if (NOT Body.IsStatic())
        { return FBox{ForceInit}; }

        const auto& WorldBounds = Body.GetWorldSpaceBounds();

        return FBox{Conv(WorldBounds.mMin), Conv(WorldBounds.mMax)};
    }

    auto
        FCk_Jolt_QuerySession::
        Get_StaticBodyTriangles(
            uint64 InBodyId,
            FCk_Jolt_TriangleSoup& OutSoup) const
        -> int32
    {
        if (ck::Is_NOT_Valid(_Impl, ck::IsValid_Policy_NullptrOnly{}))
        { return 0; }

        const auto PhysicsSystem = _Impl->_PhysicsSystem.Pin();

        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return 0; }

        const auto Lock = JPH::BodyLockRead{PhysicsSystem->GetBodyLockInterface(),
            ck_jolt_occupancy_session::Conv_BodyId(InBodyId)};

        if (NOT Lock.Succeeded())
        { return 0; }

        const auto& Body = Lock.GetBody();

        if (NOT Body.IsStatic())
        { return 0; }

        // The body's own bounds are the query volume, so nothing of the mesh is clipped away. The margin is
        // there because GetTrianglesStart excludes on a float comparison, and a triangle lying exactly on a
        // bounds face is the one a closure check can least afford to lose.
        constexpr auto BoundsMarginUu = 1.0f;
        const auto& WorldBounds = Body.GetWorldSpaceBounds();
        const auto QueryBox = JPH::AABox{
            WorldBounds.mMin - JPH::Vec3::sReplicate(BoundsMarginUu),
            WorldBounds.mMax + JPH::Vec3::sReplicate(BoundsMarginUu)};

        auto TriangleVertices = TArray<JPH::Float3>{};
        TriangleVertices.SetNumUninitialized(ck_jolt_occupancy_session::TriangleBatchSize * 3);

        return ck_jolt_occupancy_session::DoAppend_BodyTriangles(Body, QueryBox, TriangleVertices, OutSoup);
    }

    auto
        FCk_Jolt_QuerySession::
        Get_StaticBodyDescription(
            uint64 InBodyId) const
        -> FString
    {
        const auto MakeNotHeldDescription = [&]() -> FString
        { return FString::Printf(TEXT("Jolt body %llu (not held)"), InBodyId); };

        if (ck::Is_NOT_Valid(_Impl, ck::IsValid_Policy_NullptrOnly{}))
        { return MakeNotHeldDescription(); }

        const auto PhysicsSystem = _Impl->_PhysicsSystem.Pin();

        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return MakeNotHeldDescription(); }

        const auto Lock = JPH::BodyLockRead{PhysicsSystem->GetBodyLockInterface(),
            ck_jolt_occupancy_session::Conv_BodyId(InBodyId)};

        if (NOT Lock.Succeeded())
        { return MakeNotHeldDescription(); }

        const auto& Body = Lock.GetBody();

        if (NOT Body.IsStatic())
        { return MakeNotHeldDescription(); }

        const auto* Shape = Body.GetShape();

        const auto ShapeName = ck::IsValid(Shape, ck::IsValid_Policy_NullptrOnly{})
            ? ck_jolt_occupancy_session::Get_ShapeSubTypeName(ck_jolt_occupancy_session::Get_LeafShape(Shape))
            : FString{TEXT("NoShape")};

        const auto* JoltSubsystem = _Impl->_JoltSubsystem.Get();

        const auto TransientEntity = ck::IsValid(JoltSubsystem)
            ? JoltSubsystem->Get_TransientEntity()
            : FCk_Handle{};

        const auto OwnerName = ck_jolt_occupancy_session::Get_BodyOwnerName(Body.GetUserData(), TransientEntity);

        const auto Name = OwnerName.IsEmpty()
            ? FString::Printf(TEXT("Jolt static body %llu"), InBodyId)
            : OwnerName;

        const auto& WorldBounds = Body.GetWorldSpaceBounds();
        const auto Bounds = FBox{Conv(WorldBounds.mMin), Conv(WorldBounds.mMax)};
        const auto Centre = Bounds.GetCenter();
        const auto Extent = Bounds.GetExtent();

        return FString::Printf(TEXT("%s [%s] @ (%d, %d, %d) +/- (%d, %d, %d)"),
            *Name, *ShapeName,
            static_cast<int32>(FMath::RoundToInt(Centre.X)),
            static_cast<int32>(FMath::RoundToInt(Centre.Y)),
            static_cast<int32>(FMath::RoundToInt(Centre.Z)),
            static_cast<int32>(FMath::RoundToInt(Extent.X)),
            static_cast<int32>(FMath::RoundToInt(Extent.Y)),
            static_cast<int32>(FMath::RoundToInt(Extent.Z)));
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

        // The static-scene token ALONE. Every funnel that adds, removes, moves or re-types a Static body
        // bumps it, and so does the cooked static world, which is exactly the set of bodies the triangle
        // query above returns. The body-removed token is deliberately NOT folded in: it moves for every
        // motion type, so a despawning projectile on the far side of the level would report the static
        // geometry as changed — and a consumer building over several frames would never see a stable world.
        return JoltSubsystem->Get_StaticSceneRevision();
    }
}

// --------------------------------------------------------------------------------------------------------------------
