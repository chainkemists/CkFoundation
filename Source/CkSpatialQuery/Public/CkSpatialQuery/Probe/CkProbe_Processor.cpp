#include "CkProbe_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkNet/CkNet_Utils.h"

#include "CkShapes/Capsule/CkShapeCapsule_Utils.h"
#include "CkShapes/Cylinder/CkShapeCylinder_Utils.h"
#include "CkShapes/Public/CkShapes/Box/CkShapeBox_Fragment_Data.h"
#include "CkShapes/Public/CkShapes/Box/CkShapeBox_Utils.h"
#include "CkShapes/Sphere/CkShapeSphere_Utils.h"

#include "CkSpatialQuery/CkSpatialQuery_Log.h"
#include "CkSpatialQuery/CkSpatialQuery_Utils.h"
#include "CkSpatialQuery/Subsystem/CkSpatialQuery_Subsystem.h"

#include "Jolt/Jolt.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Physics/Body/Body.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Body/BodyInterface.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "Jolt/Physics/Collision/Shape/CylinderShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"

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
        using namespace JPH;
        const auto& EntityPosition = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentLocation(InHandle);

        if (auto BoxEntity = UCk_Utils_ShapeBox_UE::Cast(InHandle); ck::IsValid(BoxEntity))
        {
            const auto BoxParams = UCk_Utils_ShapeBox_UE::Get_ShapeData(BoxEntity);

            const auto& HalfExtents = BoxParams.Get_Extent();

            const auto Settings = BoxShapeSettings{jolt::Conv(HalfExtents), BoxParams.Get_ConvexRadius()};
            Settings.SetEmbedded();

            auto ShapeResult = Settings.Create();
            auto Shape = ShapeResult.Get();

            auto ShapeSettings = BodyCreationSettings{
                Shape,
                jolt::Conv(EntityPosition),
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
        else if (auto SphereEntity = UCk_Utils_ShapeSphere_UE::Cast(InHandle); ck::IsValid(SphereEntity))
        {
            const auto Params = UCk_Utils_ShapeSphere_UE::Get_ShapeData(SphereEntity);

            const auto& Radius = Params.Get_Radius();

            const auto Settings = SphereShapeSettings{Radius};
            Settings.SetEmbedded();

            auto ShapeResult = Settings.Create();
            auto Shape = ShapeResult.Get();

            auto ShapeSettings = BodyCreationSettings{
                Shape,
                jolt::Conv(EntityPosition),
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
        else if (auto CapsuleEntity = UCk_Utils_ShapeCapsule_UE::Cast(InHandle); ck::IsValid(CapsuleEntity))
        {
            const auto Params = UCk_Utils_ShapeCapsule_UE::Get_ShapeData(CapsuleEntity);

            const auto HalfHeight = Params.Get_HalfHeight();
            const auto Radius = Params.Get_Radius();

            const auto Settings = CapsuleShapeSettings{HalfHeight, Radius};
            Settings.SetEmbedded();

            auto ShapeResult = Settings.Create();
            auto Shape = ShapeResult.Get();

            auto ShapeSettings = BodyCreationSettings{
                Shape,
                jolt::Conv(EntityPosition),
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
        else if (auto CylinderEntity = UCk_Utils_ShapeCylinder_UE::Cast(InHandle); ck::IsValid(CylinderEntity))
        {
            const auto Params = UCk_Utils_ShapeCylinder_UE::Get_ShapeData(CylinderEntity);

            const auto& HalfHeight = Params.Get_HalfHeight();
            const auto& Radius = Params.Get_Radius();
            const auto& ConvexRadius = Params.Get_ConvexRadius();

            const auto Settings = CylinderShapeSettings{HalfHeight, Radius, ConvexRadius};
            Settings.SetEmbedded();

            auto ShapeResult = Settings.Create();
            auto Shape = ShapeResult.Get();

            auto ShapeSettings = BodyCreationSettings{
                Shape,
                jolt::Conv(EntityPosition),
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
        else
        {
            CK_TRIGGER_ENSURE(TEXT("No Shape found on Probe [{}]. Without a shape, a Probe cannot report overlaps"),
                InHandle);
        }
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
        auto EntityPosition = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentLocation(InHandle);
        auto EntityRotation = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentRotation(InHandle);

        auto EntityRotationQuat = FQuat{EntityRotation};
        auto Rot = jolt::Conv(EntityRotationQuat);

        auto PhysicsSystem = _PhysicsSystem.Pin();
        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        BodyInterface.MoveKinematic(InCurrent._RigidBody->GetID(), jolt::Conv(EntityPosition), Rot,
            InDeltaT.Get_Seconds());

        spatialquery::Log(TEXT("Actual Position in Jolt:[{}] [{}]"),
            InCurrent._RigidBody->GetID().GetIndexAndSequenceNumber(),
            jolt::Conv(BodyInterface.GetPosition(InCurrent._RigidBody->GetID())));
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
    }

    auto
        FProcessor_Probe_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_Requests& InRequestsComp) const
            -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](
            FFragment_Probe_Requests& InRequests)
            {
                algo::ForEachRequest(InRequests._Requests, Visitor([&](
                    const auto& InRequest)
                    {
                        DoHandleRequest(InHandle, InRequest);
                    }));
            });

        InHandle.Remove<MarkedDirtyBy>();
    }

    auto
        FProcessor_Probe_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_Probe_BeginOverlap& InRequest)
            -> void
    {
        const auto Payload = FCk_Probe_Payload_OnBeginOverlap{
            InRequest.Get_OtherEntity(),
            InRequest.Get_ContactPoints(),
            InRequest.Get_ContactNormal()
        };

        UUtils_Signal_OnProbeBeginOverlap::Broadcast(InHandle, MakePayload(InHandle, Payload));
    }

    auto
        FProcessor_Probe_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_Probe_OverlapPersisted& InRequest)
            -> void
    {
        const auto Payload = FCk_Probe_Payload_OnOverlapPersisted{
            InRequest.Get_OtherEntity(),
            InRequest.Get_ContactPoints(),
            InRequest.Get_ContactNormal()
        };

        UUtils_Signal_OnProbeOverlapPersisted::Broadcast(InHandle, MakePayload(InHandle, Payload));
    }

    auto
        FProcessor_Probe_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_Probe_EndOverlap& InRequest)
            -> void
    {
        UUtils_Signal_OnProbeEndOverlap::Broadcast(InHandle,
            MakePayload(InHandle, FCk_Probe_Payload_OnEndOverlap{InRequest.Get_OtherEntity()}));
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
