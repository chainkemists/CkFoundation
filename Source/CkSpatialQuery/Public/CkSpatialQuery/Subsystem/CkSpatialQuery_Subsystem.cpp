#include "CkSpatialQuery_Subsystem.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkSpatialQuery/CkSpatialQuery_Log.h"
#include "CkSpatialQuery/CkSpatialQuery_Utils.h"
#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"
#include "CkSpatialQuery/Probe/CkProbe_Utils.h"
#include "CkSpatialQuery/Settings/CkSpatialQuery_Settings.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Renderer/DebugRendererSimple.h>

// --------------------------------------------------------------------------------------------------------------------

namespace object_layers
{
    static constexpr JPH::ObjectLayer Non_Moving = 0;
    static constexpr JPH::ObjectLayer Moving = 1;
    static constexpr JPH::ObjectLayer Num_Layers = 2;
};

namespace broad_phase_layers
{
    static constexpr JPH::BroadPhaseLayer Non_Moving(0);
    static constexpr JPH::BroadPhaseLayer Moving(1);
    static constexpr JPH::uint Num_Layers(2);
};

namespace contact_surface
{
    auto Get_ContactPhysicalMaterial(const FCk_Handle_Probe& InProbe) -> UPhysicalMaterial*
    {
        const auto& SurfaceInfo = UCk_Utils_Probe_UE::Get_SurfaceInfo(InProbe);

        switch (const auto& PhysicalMaterialSource = SurfaceInfo.Get_PhysicalMaterialSource())
        {
            case ECk_PhysicalMaterialSource::Direct:
            {
                return SurfaceInfo.Get_PhysicalMaterial();
            }
            case ECk_PhysicalMaterialSource::Trace:
            {
                CK_TRIGGER_ENSURE(TEXT("Probe Physical Material Source [{}] is NOT supported yet"), PhysicalMaterialSource);
                return {};
            }
            default:
            {
                CK_INVALID_ENUM(PhysicalMaterialSource);
                return {};
            }
        }
    };
}

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
    {
        // Create a mapping table from object to broad phase layer
        _ObjectToBroadPhase[object_layers::Non_Moving] = broad_phase_layers::Non_Moving;
        _ObjectToBroadPhase[object_layers::Moving] = broad_phase_layers::Moving;
    }

    auto
        GetNumBroadPhaseLayers() const
            -> JPH::uint override
    {
        return broad_phase_layers::Num_Layers;
    }

    auto
        GetBroadPhaseLayer(
            JPH::ObjectLayer inLayer) const
            -> JPH::BroadPhaseLayer override
    {
        JPH_ASSERT(inLayer < broad_phase_layers::Num_Layers);
        return _ObjectToBroadPhase[inLayer];
    }

private:
    JPH::BroadPhaseLayer _ObjectToBroadPhase[object_layers::Num_Layers];
};

// --------------------------------------------------------------------------------------------------------------------

class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    auto
        ShouldCollide(
            const JPH::ObjectLayer inLayer1,
            const JPH::BroadPhaseLayer inLayer2) const
            -> bool override
    {
        switch (inLayer1)
        {
            case object_layers::Non_Moving: return inLayer2 == broad_phase_layers::Moving;
            case object_layers::Moving: return true;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

class CkObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool
        ShouldCollide(
            const JPH::ObjectLayer inObject1,
            const JPH::ObjectLayer inObject2) const override
    {
        switch (inObject1)
        {
            case object_layers::Non_Moving: return inObject2 == object_layers::Moving;
            case object_layers::Moving: return true;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

class CkContactListener : public JPH::ContactListener
{
public:
    CK_GENERATED_BODY(CkContactListener);

public:
    // See: ContactListener
    auto
        OnContactValidate(
            const JPH::Body& inBody1,
            const JPH::Body& inBody2,
            JPH::RVec3Arg inBaseOffset,
            const JPH::CollideShapeResult& inCollisionResult)
            -> JPH::ValidateResult override
    {
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    auto
        OnContactAdded(
            const JPH::Body& inBody1,
            const JPH::Body& inBody2,
            const JPH::ContactManifold& inManifold,
            JPH::ContactSettings& ioSettings)
            -> void override
    {
        ck::spatialquery::VeryVerbose(TEXT("Body [{}] and Body [{}] and SUB-SHAPE [{}] and SUB-SHAPE [{}] NEW Contact"),
            inBody1.GetID().GetIndex(), inBody2.GetID().GetIndex(),
            inManifold.mSubShapeID1.GetValue(), inManifold.mSubShapeID2.GetValue());

        auto Body1Entity = _TransientEntity.Get_ValidHandle(FCk_Entity::IdType{
            static_cast<uint32>(inBody1.GetUserData())
        });
        auto Body2Entity = _TransientEntity.Get_ValidHandle(FCk_Entity::IdType{
            static_cast<uint32>(inBody2.GetUserData())
        });

        auto Body1 = UCk_Utils_Probe_UE::Cast(Body1Entity);
        auto Body2 = UCk_Utils_Probe_UE::Cast(Body2Entity);

        if (ck::IsValid(Body1) && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body1, Body2))
        {
            const auto& ContactPoints = ck::algo::Transform<TArray<FVector>>(inManifold.mRelativeContactPointsOn1.begin(),
                inManifold.mRelativeContactPointsOn1.end(),
            [&](const auto& ContactPoint)
            {
                return ck::jolt::Conv(ContactPoint + inManifold.mBaseOffset);
            });

            UCk_Utils_Probe_UE::Request_BeginOverlap(Body1,
                FCk_Request_Probe_BeginOverlap{
                    Body2,
                    ContactPoints,
                    ck::jolt::Conv(-inManifold.mWorldSpaceNormal),
                    contact_surface::Get_ContactPhysicalMaterial(Body2)
                });
        }

        if (ck::IsValid(Body2)) //&& UCk_Utils_Probe_UE::Get_CanOverlapWith(Body2, Body1))
        {
            const auto& ContactPoints = ck::algo::Transform<TArray<FVector>>(inManifold.mRelativeContactPointsOn2.begin(),
                inManifold.mRelativeContactPointsOn2.end(),
            [&](const auto& ContactPoint)
            {
                return ck::jolt::Conv(ContactPoint + inManifold.mBaseOffset);
            });

            UCk_Utils_Probe_UE::Request_BeginOverlap(Body2,
                FCk_Request_Probe_BeginOverlap{Body1, ContactPoints,
                    ck::jolt::Conv(inManifold.mWorldSpaceNormal), contact_surface::Get_ContactPhysicalMaterial(Body1)});
        }

        _BodyToHandle.Add(inBody1.GetID().GetIndexAndSequenceNumber(), Body1Entity);
        _BodyToHandle.Add(inBody2.GetID().GetIndexAndSequenceNumber(), Body2Entity);
    }

    auto
        OnContactPersisted(
            const JPH::Body& inBody1,
            const JPH::Body& inBody2,
            const JPH::ContactManifold& inManifold,
            JPH::ContactSettings& ioSettings)
            -> void override
    {
        ck::spatialquery::VeryVerbose(TEXT("Body [{}] and Body [{}] and SUB-SHAPE [{}] and SUB-SHAPE [{}] PERSISTED Contact"),
            inBody1.GetID().GetIndex(), inBody2.GetID().GetIndex(),
            inManifold.mSubShapeID1.GetValue(), inManifold.mSubShapeID2.GetValue());

       auto Body1Entity = _TransientEntity.Get_ValidHandle(FCk_Entity::IdType{
            static_cast<uint32>(inBody1.GetUserData())
        });
        auto Body2Entity = _TransientEntity.Get_ValidHandle(FCk_Entity::IdType{
            static_cast<uint32>(inBody2.GetUserData())
        });

        auto Body1 = UCk_Utils_Probe_UE::Cast(Body1Entity);
        auto Body2 = UCk_Utils_Probe_UE::Cast(Body2Entity);

        if (ck::IsValid(Body1) && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body1, Body2))
        {
            const auto& ContactPoints = ck::algo::Transform<TArray<FVector>>(inManifold.mRelativeContactPointsOn1.begin(),
                inManifold.mRelativeContactPointsOn1.end(),
            [&](const auto& ContactPoint)
            {
                return ck::jolt::Conv(ContactPoint + inManifold.mBaseOffset);
            });

            UCk_Utils_Probe_UE::Request_OverlapUpdated(Body1,
                FCk_Request_Probe_OverlapUpdated{Body2, ContactPoints, ck::jolt::Conv(-inManifold.mWorldSpaceNormal),
                    contact_surface::Get_ContactPhysicalMaterial(Body2)});
        }

        if (ck::IsValid(Body2)) //&& UCk_Utils_Probe_UE::Get_CanOverlapWith(Body2, Body1))
        {
            const auto& ContactPoints = ck::algo::Transform<TArray<FVector>>(inManifold.mRelativeContactPointsOn2.begin(),
                inManifold.mRelativeContactPointsOn2.end(),
            [&](const auto& ContactPoint)
            {
                return ck::jolt::Conv(ContactPoint + inManifold.mBaseOffset);
            });

            UCk_Utils_Probe_UE::Request_OverlapUpdated(Body2,
                FCk_Request_Probe_OverlapUpdated{Body1, ContactPoints, ck::jolt::Conv(inManifold.mWorldSpaceNormal),
                    contact_surface::Get_ContactPhysicalMaterial(Body1)});
        }

        _BodyToHandle.Add(inBody1.GetID().GetIndexAndSequenceNumber(), Body1Entity);
        _BodyToHandle.Add(inBody2.GetID().GetIndexAndSequenceNumber(), Body2Entity);
    }

    auto
        OnContactRemoved(
            const JPH::SubShapeIDPair& inSubShapePair)
            -> void override
    {
        ck::spatialquery::VeryVerbose(TEXT("Body [{}] and Body [{}] and SUB-SHAPE [{}] and SUB-SHAPE [{}] REMOVED Contact"),
            inSubShapePair.GetBody1ID().GetIndex(), inSubShapePair.GetBody2ID().GetIndex(),
            inSubShapePair.GetSubShapeID1().GetValue(), inSubShapePair.GetSubShapeID2().GetValue());

        auto MaybeBody1Handle = _BodyToHandle.Find(inSubShapePair.GetBody1ID().GetIndexAndSequenceNumber());

        if (ck::Is_NOT_Valid(MaybeBody1Handle, ck::IsValid_Policy_NullptrOnly{}))
        {
            return;
        }

        auto MaybeBody2Handle = _BodyToHandle.Find(inSubShapePair.GetBody2ID().GetIndexAndSequenceNumber());

        if (ck::Is_NOT_Valid(MaybeBody2Handle, ck::IsValid_Policy_NullptrOnly{}))
        {
            return;
        }

        auto Body1 = UCk_Utils_Probe_UE::Cast(*MaybeBody1Handle);
        auto Body2 = UCk_Utils_Probe_UE::Cast(*MaybeBody2Handle);

        if (ck::IsValid(Body1))// && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body1, Body2))
        {
            UCk_Utils_Probe_UE::Request_EndOverlap(Body1, FCk_Request_Probe_EndOverlap{Body2});
        }

        if (ck::IsValid(Body2)) //&& UCk_Utils_Probe_UE::Get_CanOverlapWith(Body2, Body1))
        {
            UCk_Utils_Probe_UE::Request_EndOverlap(Body2, FCk_Request_Probe_EndOverlap{Body1});
        }

        _BodyToHandle.Remove(inSubShapePair.GetBody1ID().GetIndexAndSequenceNumber());
        _BodyToHandle.Remove(inSubShapePair.GetBody2ID().GetIndexAndSequenceNumber());
    }

private:
    FCk_Handle _TransientEntity;

    TMap<uint32, FCk_Handle> _BodyToHandle;

public:
    CK_DEFINE_CONSTRUCTORS(CkContactListener, _TransientEntity);
};

// --------------------------------------------------------------------------------------------------------------------

class CkBodyActivationListener : public JPH::BodyActivationListener
{
public:
    auto
        OnBodyActivated(
            const JPH::BodyID& inBodyID,
            uint64 inBodyUserData)
            -> void override
    {
        ck::spatialquery::Log(TEXT("Body [{}] just ACTIVATED"), inBodyID.GetIndex());
    }

    auto
        OnBodyDeactivated(
            const JPH::BodyID& inBodyID,
            uint64 inBodyUserData)
            -> void override
    {
        ck::spatialquery::Log(TEXT("Body [{}] just DE-ACTIVATED"), inBodyID.GetIndex());
    }
};

// --------------------------------------------------------------------------------------------------------------------

class CkJoltDebugger : public JPH::DebugRendererSimple
{
    auto
    DrawLine(
        JPH::RVec3Arg inFrom,
        JPH::RVec3Arg inTo,
        JPH::ColorArg inColor)
    -> void override
    {
        if (ck::Is_NOT_Valid(_World))
        { return; }

        UCk_Utils_DebugDraw_UE::DrawDebugLine(_World.Get(), FCk_LogCategory{NAME_None},
            ECk_LogVerbosity::Log, ck::jolt::Conv(inFrom), ck::jolt::Conv(inTo), ck::jolt::Conv(inColor));
    }

    auto
    DrawText3D(
        JPH::RVec3Arg inPosition,
        const JPH::string_view& inString,
        JPH::ColorArg inColor = JPH::Color::sWhite,
        float inHeight = 0.5f)
    -> void
    {
        if (ck::Is_NOT_Valid(_World))
        { return; }

        UCk_Utils_DebugDraw_UE::DrawDebugString(_World.Get(), FCk_LogCategory{NAME_None},
            ECk_LogVerbosity::Log, ck::jolt::Conv(inPosition), FString{static_cast<int32>(inString.length()), inString.data()}, nullptr,
            ck::jolt::Conv(inColor));
    }

public:
    TWeakObjectPtr<UWorld> _World;
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck_spatialquery_subsystem
{
    auto
        CustomTraceFunction(
            const char* inFMT,
            ...)
        -> void
    {
        va_list List;
        va_start(List, inFMT);
        char Buffer[1024];
        vsnprintf(Buffer, sizeof(Buffer), inFMT, List);
        va_end(List);

        ck::spatialquery::Log(TEXT("Jolt Trace: [{}]"), FString{Buffer});
    }

    auto
        CustomAssertFunction(
            const char* inExpression,
            const char* inMessage,
            const char* inFile,
            JPH::uint inLine)
        -> bool
    {
        CK_TRIGGER_ENSURE(TEXT("Jolt FAILED [{}] with Message [{}].\n{}:{}"), FString{inExpression}, FString{inMessage},
            FString{inFile}, inLine);
        return false;
    }
}

auto
    UCk_SpatialQuery_Subsystem::
    Initialize(
        FSubsystemCollectionBase& InCollection)
        -> void
{
    Super::Initialize(InCollection);
    _EcsWorldSubsystem = InCollection.InitializeDependency<UCk_EcsWorld_Subsystem_UE>();

    using namespace JPH;

    RegisterDefaultAllocator();
    Factory::sInstance = new Factory{};
    RegisterTypes();

    constexpr uint MaxBodies = 65536;
    constexpr uint NumBodyMutexes = 0;
    constexpr uint MaxBodyPairs = 65536;
    constexpr uint MaxContactConstraints = 10240;

    constexpr int MaxPhysicsJobs = 2048;
    constexpr int MaxPhysicsBarriers = 8;

    // TODO: Drive through Project settings
    _TempAllocator = MakePimpl<TempAllocatorImpl>(10 * 1024 * 1024); // 10MB
    //_JobSystem = new JobSystemThreadPool(MaxPhysicsJobs, MaxPhysicsBarriers, thread::hardware_concurrency() - 1);
    _JobSystem = new JPH::JobSystemSingleThreaded(MaxPhysicsJobs);

    constexpr auto Alignment = alignof(JobSystemThreadPool);

    _BroadPhaseLayerInterface = MakePimpl<BPLayerInterfaceImpl>();
    _ObjectVsBroadPhaseLayerFilter = MakePimpl<ObjectVsBroadPhaseLayerFilterImpl>();
    _ObjectVsObjectFilter = MakePimpl<CkObjectLayerPairFilterImpl>();

    _PhysicsSystem = MakeShared<PhysicsSystem>();
    _PhysicsSystem->Init(MaxBodies, NumBodyMutexes, MaxBodyPairs, MaxContactConstraints, *_BroadPhaseLayerInterface,
        *_ObjectVsBroadPhaseLayerFilter, *_ObjectVsObjectFilter);

    _BodyActivationListener = MakePimpl<CkBodyActivationListener>();
    _PhysicsSystem->SetBodyActivationListener(_BodyActivationListener.Get());

    _ContactListener = MakePimpl<CkContactListener>(_EcsWorldSubsystem->Get_TransientEntity());
    _PhysicsSystem->SetContactListener(&*_ContactListener);

#if JPH_DEBUG_RENDERER
    if (ck::Is_NOT_Valid(JPH::DebugRenderer::sInstance, ck::IsValid_Policy_NullptrOnly{}))
    {
        _Debugger = MakePimpl<CkJoltDebugger>();
        _Debugger->_World = GetWorld();
    }
#endif

    JPH::Trace = ck_spatialquery_subsystem::CustomTraceFunction;
    JPH::AssertFailed = ck_spatialquery_subsystem::CustomAssertFunction;
}

auto
    UCk_SpatialQuery_Subsystem::
    Tick(
        float InDeltaTime)
        -> void
{
    Super::Tick(InDeltaTime);

    if (GetWorld()->IsPaused())
    { return; }

    _PhysicsSystem->Update(InDeltaTime, 1, &*_TempAllocator, _JobSystem);

#if JPH_DEBUG_RENDERER
    // Named constants for clear initialization
    constexpr auto DrawGetSupportFeatures = false;
    constexpr auto DrawSupportDirection = false;
    constexpr auto DrawGetSupportingFace = false;
    constexpr auto DrawShape = true;
    constexpr auto DrawShapeWireframe = true;
    constexpr auto DrawShapeColor = JPH::BodyManager::EShapeColor::MotionTypeColor;
    constexpr auto DrawBoundingBox = true;
    constexpr auto DrawCenterOfMassTransform = false;
    constexpr auto DrawWorldTransform = true;
    constexpr auto DrawVelocity = true;
    constexpr auto DrawMassAndInertia = false;
    constexpr auto DrawSleepStats = false;
    constexpr auto DrawSoftBodyVertices = false;
    constexpr auto DrawSoftBodyVertexVelocities = false;
    constexpr auto DrawSoftBodyEdgeConstraints = false;
    constexpr auto DrawSoftBodyBendConstraints = false;
    constexpr auto DrawSoftBodyVolumeConstraints = false;
    constexpr auto DrawSoftBodySkinConstraints = false;
    constexpr auto DrawSoftBodyLraConstraints = false;
    constexpr auto DrawSoftBodyPredictedBounds = false;
    constexpr auto DrawSoftBodyConstraintColor = JPH::ESoftBodyConstraintColor::ConstraintType;

    // Usage example:
    constexpr auto DrawSettings = JPH::BodyManager::DrawSettings
    {
        DrawGetSupportFeatures,
        DrawSupportDirection,
        DrawGetSupportingFace,
        DrawShape,
        DrawShapeWireframe,
        DrawShapeColor,
        DrawBoundingBox,
        DrawCenterOfMassTransform,
        DrawWorldTransform,
        DrawVelocity,
        DrawMassAndInertia,
        DrawSleepStats,
        DrawSoftBodyVertices,
        DrawSoftBodyVertexVelocities,
        DrawSoftBodyEdgeConstraints,
        DrawSoftBodyBendConstraints,
        DrawSoftBodyVolumeConstraints,
        DrawSoftBodySkinConstraints,
        DrawSoftBodyLraConstraints,
        DrawSoftBodyPredictedBounds,
        DrawSoftBodyConstraintColor
    };

    if (ck::IsValid(_Debugger, ck::IsValid_Policy_NullptrOnly{}) &&
        UCk_Utils_SpatialQuery_Settings::Get_DebugPreviewAllProbesUsingJolt())
    { _PhysicsSystem->DrawBodies(DrawSettings, _Debugger.Get()); }
#endif
}

auto
    UCk_SpatialQuery_Subsystem::
    Deinitialize()
        -> void
{
    _ContactListener.Reset();
    _BodyActivationListener.Reset();
    _PhysicsSystem.Reset();
    _ObjectVsObjectFilter.Reset();
    _ObjectVsBroadPhaseLayerFilter.Reset();
    _BroadPhaseLayerInterface.Reset();
    delete _JobSystem;
    _TempAllocator.Reset();

    Super::Deinitialize();
}

auto
    UCk_SpatialQuery_Subsystem::
    Get_PhysicsSystem() const
        -> TWeakPtr<JPH::PhysicsSystem>
{
    return _PhysicsSystem;
}

// --------------------------------------------------------------------------------------------------------------------
