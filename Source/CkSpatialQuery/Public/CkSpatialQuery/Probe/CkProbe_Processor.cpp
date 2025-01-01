#include "CkProbe_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkNet/CkNet_Utils.h"

#include "CkSpatialQuery/CkSpatialQuery_Log.h"
#include "CkSpatialQuery/Subsystem/CkSpatialQuery_Subsystem.h"

#include "Jolt/Jolt.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Physics/Body/Body.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Body/BodyInterface.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"

// --------------------------------------------------------------------------------------------------------------------

struct EdynStruct
{
    CK_GENERATED_BODY(EdynStruct);

    entt::entity _Entity;

    CK_DEFINE_CONSTRUCTORS(EdynStruct, _Entity);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FProcessor_Probe_Setup::
    FProcessor_Probe_Setup(
        const RegistryType& InRegistry,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : TProcessor(InRegistry)
        , _PhysicsSystem(InPhysicsSystem) {}

    auto
        FProcessor_Probe_Setup::
        DoTick(
            TimeType InDeltaT)
            -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Probe_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent)
            -> void
    {
        const auto& HalfExtents = InParams.Get_Params().Get_HalfExtents();
        const auto& EntityPosition = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentLocation(InHandle);

        using namespace JPH;
        using namespace jolt_bridge;

        auto Settings = BoxShapeSettings{ToVec3(HalfExtents)};
        Settings.SetEmbedded();

        auto ShapeResult = Settings.Create();
        auto Shape = ShapeResult.Get();

        auto ShapeSettings = BodyCreationSettings{
            Shape,
            ToVec3(EntityPosition),
            Quat::sIdentity(),
            EMotionType::Kinematic,
            ObjectLayer{1}
        };
        ShapeSettings.mIsSensor = true;

        auto PhysicsSystem = _PhysicsSystem.Pin();
        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        InCurrent._RigidBody = BodyInterface.CreateBody(ShapeSettings);
        InCurrent._RigidBody->SetUserData(static_cast<uint64>(InHandle.Get_Entity().Get_ID()));
        InCurrent._RigidBody->SetCollideKinematicVsNonDynamic(true);
        BodyInterface.AddBody(InCurrent._RigidBody->GetID(), EActivation::Activate);
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_Probe_UpdateTransform::
    FProcessor_Probe_UpdateTransform(
        const RegistryType& InRegistry,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : TProcessor(InRegistry)
        , _PhysicsSystem(InPhysicsSystem) {}

    auto
        FProcessor_Probe_UpdateTransform::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent)
            -> void
    {
        using namespace jolt_bridge;

        auto EntityPosition = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentLocation(InHandle);
        auto EntityRotation = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentRotation(InHandle);

        auto EntityRotationQuat = FQuat{EntityRotation};
        auto Rot = JPH::Quat{(float)EntityRotationQuat.X, (float)EntityRotationQuat.Y, (float)EntityRotationQuat.Z, (float)EntityRotationQuat.W};

        auto PhysicsSystem = _PhysicsSystem.Pin();
        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        BodyInterface.MoveKinematic(InCurrent._RigidBody->GetID(), ToVec3(EntityPosition), Rot, InDeltaT.Get_Seconds());

        ck::spatialquery::Log(TEXT("Actual Position in Jolt:[{}] [{}]"), InCurrent._RigidBody->GetID().GetIndexAndSequenceNumber(),
            ToVec3(BodyInterface.GetPosition(InCurrent._RigidBody->GetID())));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Probe_HandleRequests::
        DoTick(
            TimeType InDeltaT)
            -> void
    {
        _TransientEntity.Clear<FTag_Probe_Updated>();

        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Probe_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent,
            FFragment_Probe_Requests& InRequestsComp) const
            -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](
            FFragment_Probe_Requests& InRequests)
            {
                algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](
                    const auto& InRequest)
                    {
                        DoHandleRequest(InHandle, InParams, InCurrent, InRequest);
                    }));
            });
    }

    auto
        FProcessor_Probe_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent,
            const FCk_Request_Probe_ExampleRequest& InRequest)
            -> void
    {
        // Add request handling logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Probe_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent) const
            -> void
    {
        // Add teardown logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Probe_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Probe_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Probe_Rep>& InRepComp) const
            -> void
    {
        UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_Probe_Rep>(InHandle, [&](
            UCk_Fragment_Probe_Rep* InLocalRepComp)
            {
                // Add replication logic here
            });
    }
}

// --------------------------------------------------------------------------------------------------------------------
