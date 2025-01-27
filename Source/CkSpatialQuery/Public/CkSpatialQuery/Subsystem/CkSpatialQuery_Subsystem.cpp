#include "CkSpatialQuery_Subsystem.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

// Jolt includes
#include "CkSpatialQuery/CkSpatialQuery_Log.h"
#include "CkSpatialQuery/CkSpatialQuery_Utils.h"
#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"
#include "CkSpatialQuery/Probe/CkProbe_Utils.h"

#include "Jolt/Core/JobSystemSingleThreaded.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

// --------------------------------------------------------------------------------------------------------------------

namespace layers
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

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
    {
        // Create a mapping table from object to broad phase layer
        _ObjectToBroadPhase[layers::Non_Moving] = broad_phase_layers::Non_Moving;
        _ObjectToBroadPhase[layers::Moving] = broad_phase_layers::Moving;
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
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return _ObjectToBroadPhase[inLayer];
    }

private:
    JPH::BroadPhaseLayer _ObjectToBroadPhase[layers::Num_Layers];
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
            case layers::Non_Moving: return inLayer2 == broad_phase_layers::Moving;
            case layers::Moving: return true;
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
            case layers::Non_Moving: return inObject2 == layers::Moving;
            case layers::Moving: return true;
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
            auto ContactPoints = TArray<FVector>{};
            ContactPoints.Reserve(inManifold.mRelativeContactPointsOn1.size());

            for (const auto& ContactPoint : inManifold.mRelativeContactPointsOn1)
            {
                ContactPoints.Emplace(ck::jolt::Conv(ContactPoint + inManifold.mBaseOffset));
            }

            UCk_Utils_Probe_UE::Request_BeginOverlap(Body1,
                FCk_Request_Probe_BeginOverlap{Body2, ContactPoints, ck::jolt::Conv(inManifold.mWorldSpaceNormal)});
        }

        if (ck::IsValid(Body2) && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body2, Body1))
        {
            auto ContactPoints = TArray<FVector>{};
            ContactPoints.Reserve(inManifold.mRelativeContactPointsOn1.size());

            for (const auto& ContactPoint : inManifold.mRelativeContactPointsOn2)
            {
                ContactPoints.Emplace(ck::jolt::Conv(ContactPoint + inManifold.mBaseOffset));
            }

            UCk_Utils_Probe_UE::Request_BeginOverlap(Body2,
                FCk_Request_Probe_BeginOverlap{Body1, ContactPoints, ck::jolt::Conv(-inManifold.mWorldSpaceNormal)});
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
            auto ContactPoints = TArray<FVector>{};
            ContactPoints.Reserve(inManifold.mRelativeContactPointsOn1.size());

            for (const auto& ContactPoint : inManifold.mRelativeContactPointsOn1)
            {
                ContactPoints.Emplace(ck::jolt::Conv(ContactPoint + inManifold.mBaseOffset));
            }

            UCk_Utils_Probe_UE::Request_OverlapPersisted(Body1,
                FCk_Request_Probe_OverlapUpdated{Body2, ContactPoints, ck::jolt::Conv(inManifold.mWorldSpaceNormal)});
        }

        if (ck::IsValid(Body2) && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body2, Body1))
        {
            auto ContactPoints = TArray<FVector>{};
            ContactPoints.Reserve(inManifold.mRelativeContactPointsOn1.size());

            for (const auto& ContactPoint : inManifold.mRelativeContactPointsOn2)
            {
                ContactPoints.Emplace(ck::jolt::Conv(ContactPoint + inManifold.mBaseOffset));
            }

            UCk_Utils_Probe_UE::Request_OverlapPersisted(Body2,
                FCk_Request_Probe_OverlapUpdated{Body1, ContactPoints, ck::jolt::Conv(-inManifold.mWorldSpaceNormal)});
        }

        _BodyToHandle.Add(inBody1.GetID().GetIndexAndSequenceNumber(), Body1Entity);
        _BodyToHandle.Add(inBody2.GetID().GetIndexAndSequenceNumber(), Body2Entity);
    }

    auto
        OnContactRemoved(
            const JPH::SubShapeIDPair& inSubShapePair)
            -> void override
    {
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

        if (ck::IsValid(Body1) && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body1, Body2))
        {
            UCk_Utils_Probe_UE::Request_EndOverlap(Body1, FCk_Request_Probe_EndOverlap{Body2});
        }

        if (ck::IsValid(Body2) && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body2, Body1))
        {
            UCk_Utils_Probe_UE::Request_EndOverlap(Body2, FCk_Request_Probe_EndOverlap{Body1});
        }

        _BodyToHandle.Remove(inSubShapePair.GetBody1ID().GetIndexAndSequenceNumber());
        _BodyToHandle.Remove(inSubShapePair.GetBody2ID().GetIndexAndSequenceNumber());

        return;
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
        ck::spatialquery::Warning(TEXT("Body activated"));
    }

    auto
        OnBodyDeactivated(
            const JPH::BodyID& inBodyID,
            uint64 inBodyUserData)
            -> void override
    {
        ck::spatialquery::Warning(TEXT("Body deactivated"));
    }
};

// --------------------------------------------------------------------------------------------------------------------

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
}

auto
    UCk_SpatialQuery_Subsystem::
    Tick(
        float InDeltaTime)
        -> void
{
    Super::Tick(InDeltaTime);

    _PhysicsSystem->Update(1.0f / 60.0f, 1, &*_TempAllocator, _JobSystem);
}

auto
    UCk_SpatialQuery_Subsystem::
    Deinitialize()
        -> void
{
    delete _JobSystem;
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
