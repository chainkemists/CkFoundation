#include "CkSensor_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Utils.h"

#include "CkOverlapBody/CkOverlapBody_Log.h"
#include "CkOverlapBody/Marker/CkMarker_Utils.h"
#include "CkOverlapBody/MarkerAndSensor/CkMarkerAndSensor_Utils.h"
#include "CkOverlapBody/Sensor/CkSensor_Utils.h"
#include "CkOverlapBody/Settings/CkOverlapBody_Settings.h"

#include <Components/SkeletalMeshComponent.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Sensor_Setup::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Sensor_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InSensorEntity,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp) const
        -> void
    {
        const auto& SensorLifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InSensorEntity);
        const auto& SensorAttachedEntityAndActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails(SensorLifetimeOwner);
        const auto& SensorAttachedEntity = SensorAttachedEntityAndActor.Get_Handle();

        constexpr auto IncludePendingKill = true;
        CK_ENSURE_IF_NOT(ck::IsValid(SensorAttachedEntityAndActor.Get_Actor().Get(IncludePendingKill), ck::IsValid_Policy_IncludePendingKill{}),
            TEXT("Sensor Entity [{}] is attached to Entity [{}] that does NOT have a valid Actor even though we have an OwningActor Fragment"),
            InSensorEntity,
            SensorAttachedEntity)
        { return; }

        if (ck::Is_NOT_Valid(SensorAttachedEntityAndActor.Get_Actor()))
        { return; }

        InCurrentComp._AttachedEntityAndActor = SensorAttachedEntityAndActor;

        const auto& Params      = InParamsComp.Get_Params();
        const auto& ShapeParams = Params.Get_ShapeParams();
        const auto& PhysicsParams = Params.Get_PhysicsParams();

        CK_ENSURE_IF_NOT
        (
            PhysicsParams.Get_CollisionProfileName().Name != TEXT("NoCollision"),
            TEXT("Sensor [{}] added to Entity [{}] has Collision Profile set to [{}]. Sensor will NOT work properly!"),
            Params.Get_SensorName(),
            SensorLifetimeOwner,
            PhysicsParams.Get_CollisionProfileName()
        ) {};

        switch (const auto& ShapeType = ShapeParams.Get_ShapeDimensions().Get_ShapeType())
        {
            case ECk_ShapeType::Box:
            {
                UCk_Utils_MarkerAndSensor_UE::Add_MarkerOrSensor_ActorComponent<UCk_Sensor_ActorComponent_Box_UE, ECk_ShapeType::Box>(InSensorEntity, SensorAttachedEntityAndActor, Params);
                break;
            }
            case ECk_ShapeType::Sphere:
            {
                UCk_Utils_MarkerAndSensor_UE::Add_MarkerOrSensor_ActorComponent<UCk_Sensor_ActorComponent_Sphere_UE, ECk_ShapeType::Sphere>(InSensorEntity, SensorAttachedEntityAndActor, Params);
                break;
            }
            case ECk_ShapeType::Capsule:
            {
                UCk_Utils_MarkerAndSensor_UE::Add_MarkerOrSensor_ActorComponent<UCk_Sensor_ActorComponent_Capsule_UE, ECk_ShapeType::Capsule>(InSensorEntity, SensorAttachedEntityAndActor, Params);
                break;
            }
            default:
            {
                CK_INVALID_ENUM(ShapeType);
                break;
            }
        }

        const auto& AttachmentParams = Params.Get_AttachmentParams();
        const auto& UseBoneTransform = EnumHasAnyFlags(AttachmentParams.Get_AttachmentPolicy(), ECk_Sensor_AttachmentPolicy::UseBonePosition | ECk_Sensor_AttachmentPolicy::UseBoneRotation);

        if (NOT UseBoneTransform)
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(AttachmentParams.Get_BoneName()),
            TEXT("Sensor Entity [{}] uses Attachment Policy [UseBonePosition: {}, UseBoneRotation: {}] but has an INVALID BoneName specified"),
            InSensorEntity,
            EnumHasAnyFlags(AttachmentParams.Get_AttachmentPolicy(), ECk_Sensor_AttachmentPolicy::UseBonePosition),
            EnumHasAnyFlags(AttachmentParams.Get_AttachmentPolicy(), ECk_Sensor_AttachmentPolicy::UseBoneRotation))
        { return; }

        const auto& AttachedActorSkeletalMeshComponent = SensorAttachedEntityAndActor.Get_Actor()->FindComponentByClass<USkeletalMeshComponent>();

        CK_ENSURE_IF_NOT(ck::IsValid(AttachedActorSkeletalMeshComponent),
            TEXT("Sensor Entity [{}] cannot use Attachment Policy [UseBonePosition: {}, UseBoneRotation: {}] because it is attached to an Actor that has NO SkeletalMesh"),
            InSensorEntity,
            EnumHasAnyFlags(AttachmentParams.Get_AttachmentPolicy(), ECk_Sensor_AttachmentPolicy::UseBonePosition),
            EnumHasAnyFlags(AttachmentParams.Get_AttachmentPolicy(), ECk_Sensor_AttachmentPolicy::UseBoneRotation))
        { return; }

        UCk_Utils_Sensor_UE::Request_MarkSensor_AsNeedToUpdateTransform(InSensorEntity);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Sensor_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp,
            const FFragment_Sensor_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](const FFragment_Sensor_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._EnableDisableRequest,
            [&](const FFragment_Sensor_Requests::EnableDisableRequestType& InRequest) -> void
            {
                DoHandleRequest(InHandle, InCurrentComp, InParamsComp, InRequest);
            }, policy::DontResetContainer{});

            algo::ForEachRequest(InRequests._BeginOrEndOverlapRequests, ck::Visitor(
            [&](const auto& InRequestVariant)
            {
                DoHandleRequest(InHandle, InCurrentComp, InParamsComp, InRequestVariant);
            }), policy::DontResetContainer{});
        });
    }

    auto
        FProcessor_Sensor_HandleRequests::
        DoHandleRequest(
            HandleType InSensorEntity,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_EnableDisable& InRequest)
            -> void
    {
        const auto& Sensor = InCurrentComp.Get_Sensor().Get();

        const auto SensorBasicDetails =  FCk_Sensor_BasicDetails
        {
            InParamsComp.Get_Params().Get_SensorName(),
            InSensorEntity,
            InCurrentComp.Get_AttachedEntityAndActor()
        };

        // TODO: this is repeated in this file, consolidate
        const auto& DoManuallyTriggerAllEndOverlaps = [&]() -> void
        {
            for (const auto& OverlapKvp : InCurrentComp.Get_CurrentMarkerOverlaps().Get_Overlaps())
            {
                const auto& MarkerDetails = OverlapKvp.Key;
                const auto& OverlapDetails = OverlapKvp.Value.Get_OverlapDetails();

                const auto& OnEndOverlapPayload = FCk_Sensor_Payload_OnEndOverlap
                {
                    SensorBasicDetails,
                    MarkerDetails,
                    FCk_Sensor_EndOverlap_UnrealDetails
                    {
                        OverlapDetails.Get_OverlappedComponent().Get(),
                        OverlapDetails.Get_OtherActor().Get(),
                        OverlapDetails.Get_OtherComp().Get()
                    }
                };

                UUtils_Signal_OnSensorEndOverlap::Broadcast(InSensorEntity, MakePayload(
                    InCurrentComp.Get_AttachedEntityAndActor().Get_Handle(), OnEndOverlapPayload));
            }
        };

        const auto& CurrentEnableDisable = InCurrentComp.Get_EnableDisable();
        const auto& NewEnableDisable     = InRequest.Get_EnableDisable();

        if (CurrentEnableDisable == NewEnableDisable)
        { return; }

        InCurrentComp._EnableDisable = NewEnableDisable;

        const auto& CollisionEnabled = NewEnableDisable == ECk_EnableDisable::Enable
                                         ? ECollisionEnabled::QueryOnly
                                         : ECollisionEnabled::NoCollision;

        const auto& Params     = InParamsComp.Get_Params();
        const auto& SensorName = Params.Get_SensorName();

        CK_ENSURE_IF_NOT(ck::IsValid(Sensor), TEXT("Entity [{}] has an Invalid Sensor stored!"), InSensorEntity)
        { return; }

        if (NewEnableDisable == ECk_EnableDisable::Disable)
        {
            DoManuallyTriggerAllEndOverlaps();
            InCurrentComp._CurrentMarkerOverlaps = {};
        }

        UCk_Utils_Physics_UE::Request_SetGenerateOverlapEvents(Sensor, NewEnableDisable);
        UCk_Utils_Physics_UE::Request_SetCollisionEnabled(Sensor, CollisionEnabled);

        UUtils_Signal_OnSensorEnableDisable::Broadcast(InSensorEntity, MakePayload(
            InCurrentComp.Get_AttachedEntityAndActor().Get_Handle(), SensorName, NewEnableDisable));
    }

    auto
        FProcessor_Sensor_HandleRequests::
        DoHandleRequest(
            HandleType InSensorEntity,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_OnBeginOverlap& InRequest)
            -> void
    {
        const auto& OverlappedMarkerDetails = InRequest.Get_MarkerDetails();
        const auto& OverlappedMarkerName    = OverlappedMarkerDetails.Get_MarkerName();
        const auto& OverlappedMarkerEntity  = OverlappedMarkerDetails.Get_MarkerEntity();

        const auto& SensorName           = InRequest.Get_SensorDetails().Get_SensorName();
        const auto& SensorFilteringInfo  = InParamsComp.Get_Params().Get_FilteringParams().Get_MarkerNames();

        CK_ENSURE_IF_NOT(ck::IsValid(OverlappedMarkerEntity),
            TEXT("Sensor [Name: {} | Entity: {}] received EndOverlap with Marker [{}] that has an INVALID Entity"),
            SensorName,
            InSensorEntity,
            OverlappedMarkerName)
        { return; }

        if (NOT SensorFilteringInfo.Contains(OverlappedMarkerName))
        { return; }

        if (InCurrentComp._CurrentMarkerOverlaps.Get_HasOverlapWithMarker(OverlappedMarkerDetails))
        { return; }

        const auto& OverlappedMarkerEnableDisable = UCk_Utils_Marker_UE::
            Get_EnableDisable(OverlappedMarkerEntity);

        CK_ENSURE_IF_NOT(OverlappedMarkerEnableDisable == ECk_EnableDisable::Enable,
            TEXT("Sensor [Name: {} | Entity: {}] received BeginOverlap with Marker [Name: {} | Entity: {}] that is DISABLED"),
            SensorName,
            InSensorEntity,
            OverlappedMarkerName,
            OverlappedMarkerEntity)
        { return; }

        overlap_body::VeryVerbose
        (
            TEXT("Sensor [Name: {} | Entity: {}] BeginOverlap with Marker [Name: {} | Entity: {}]"),
            SensorName,
            InSensorEntity,
            OverlappedMarkerName,
            OverlappedMarkerEntity
        );

        const auto& OverlapDetails = InRequest.Get_OverlapDetails();

        InCurrentComp._CurrentMarkerOverlaps.Process_Add(FCk_Sensor_MarkerOverlapInfo{ OverlappedMarkerDetails, OverlapDetails });

        const auto& OnBeginOverlapPayload = FCk_Sensor_Payload_OnBeginOverlap
        {
            InRequest.Get_SensorDetails(),
            OverlappedMarkerDetails,
            OverlapDetails
        };

        UUtils_Signal_OnSensorBeginOverlap::Broadcast(InSensorEntity, MakePayload(
            InCurrentComp.Get_AttachedEntityAndActor().Get_Handle(), OnBeginOverlapPayload));
    }

    auto
        FProcessor_Sensor_HandleRequests::
        DoHandleRequest(
            HandleType InSensorEntity,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_OnEndOverlap& InRequest)
            -> void
    {
        const auto& OverlappedMarkerDetails = InRequest.Get_MarkerDetails();
        const auto& OverlappedMarkerName    = OverlappedMarkerDetails.Get_MarkerName();
        const auto& OverlappedMarkerEntity  = OverlappedMarkerDetails.Get_MarkerEntity();

        const auto& SensorName           = InRequest.Get_SensorDetails().Get_SensorName();
        const auto& SensorFilteringInfo  = InParamsComp.Get_Params().Get_FilteringParams().Get_MarkerNames();

        CK_ENSURE_IF_NOT(ck::IsValid(OverlappedMarkerEntity, ck::IsValid_Policy_IncludePendingKill{}),
            TEXT("Sensor [Name: {} | Entity: {}] received EndOverlap with Marker [{}] that has an INVALID Entity"),
            SensorName,
            InSensorEntity,
            OverlappedMarkerName)
        { return; }

        if (NOT SensorFilteringInfo.Contains(OverlappedMarkerName))
        { return; }

        if (NOT InCurrentComp._CurrentMarkerOverlaps.Get_HasOverlapWithMarker(OverlappedMarkerDetails))
        { return; }

        overlap_body::VeryVerbose
        (
            TEXT("Sensor [Name: {} | Entity: {}] EndOverlap with Marker [Name: {} | Entity: {}]"),
            SensorName,
            InSensorEntity,
            OverlappedMarkerName,
            OverlappedMarkerEntity
        );

        const auto& OverlapDetails = InRequest.Get_OverlapDetails();

        InCurrentComp._CurrentMarkerOverlaps.Process_RemoveOverlapWithMarker(OverlappedMarkerDetails);

        const auto& OnEndOverlapPayload = FCk_Sensor_Payload_OnEndOverlap
        {
            InRequest.Get_SensorDetails(),
            OverlappedMarkerDetails,
            OverlapDetails
        };

        const auto& SensorLifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InSensorEntity);

        UUtils_Signal_OnSensorEndOverlap::Broadcast(InSensorEntity, MakePayload(
            SensorLifetimeOwner, OnEndOverlapPayload));
    }

    auto
        FProcessor_Sensor_HandleRequests::
        DoHandleRequest(
            HandleType InSensorEntity,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_OnBeginOverlap_NonMarker& InRequest)
            -> void
    {
        const auto& OverlapDetails = InRequest.Get_OverlapDetails();
        const auto& NonMarkerOverlapInfo = FCk_Sensor_NonMarkerOverlapInfo{OverlapDetails};

        if (InCurrentComp._CurrentNonMarkerOverlaps.Get_Overlaps().Contains(NonMarkerOverlapInfo))
        { return; }

        overlap_body::VeryVerbose
        (
            TEXT("Sensor [Name: {} | Entity: {}] BeginOverlap with Non-Marker [{}]"),
            InRequest.Get_SensorDetails().Get_SensorName(),
            InSensorEntity,
            OverlapDetails
        );

        InCurrentComp._CurrentNonMarkerOverlaps.Process_Add(NonMarkerOverlapInfo);

        const auto& OnBeginOverlapPayload = FCk_Sensor_Payload_OnBeginOverlap_NonMarker
        {
            InRequest.Get_SensorDetails(),
            OverlapDetails
        };

        UUtils_Signal_OnSensorBeginOverlap_NonMarker::Broadcast(InSensorEntity, MakePayload(
            InCurrentComp.Get_AttachedEntityAndActor().Get_Handle(), OnBeginOverlapPayload));
    }

    auto
        FProcessor_Sensor_HandleRequests::
        DoHandleRequest(
            HandleType InSensorEntity,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_OnEndOverlap_NonMarker& InRequest)
            -> void
    {
        const auto& OverlapDetails = InRequest.Get_OverlapDetails();
        const auto& NonMarkerOverlapInfo = FCk_Sensor_NonMarkerOverlapInfo
        {
            FCk_Sensor_BeginOverlap_UnrealDetails
            {
                OverlapDetails.Get_OverlappedComponent().Get(),
                OverlapDetails.Get_OtherActor().Get(),
                OverlapDetails.Get_OtherComp().Get()
            }
        };

        if (NOT InCurrentComp._CurrentNonMarkerOverlaps.Get_Overlaps().Contains(NonMarkerOverlapInfo))
        { return; }

        overlap_body::VeryVerbose
        (
            TEXT("Sensor [Name: {} | Entity: {}] EndOverlap with Non-Marker [{}]"),
            InRequest.Get_SensorDetails().Get_SensorName(),
            InSensorEntity,
            OverlapDetails
        );

        InCurrentComp._CurrentNonMarkerOverlaps.Process_Remove(NonMarkerOverlapInfo);

        const auto& OnEndOverlapPayload = FCk_Sensor_Payload_OnEndOverlap_NonMarker
        {
            InRequest.Get_SensorDetails(),
            OverlapDetails
        };

        UUtils_Signal_OnSensorEndOverlap_NonMarker::Broadcast(InSensorEntity, MakePayload(
            InCurrentComp.Get_AttachedEntityAndActor().Get_Handle(), OnEndOverlapPayload));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Sensor_UpdateTransform::
        ForEachEntity(
            TimeType,
            HandleType                      InSensorEntity,
            const FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params&  InParamsComp)
            -> void
    {
        // TODO: Extract this in a common function to avoid code duplication between similar system for Marker
        const auto& SensorAttachedEntityAndActor = InCurrentComp.Get_AttachedEntityAndActor();
        const auto& SensorAttachedActor          = SensorAttachedEntityAndActor.Get_Actor();

        CK_ENSURE_IF_NOT(ck::IsValid(SensorAttachedActor),
            TEXT("Cannot update Sensor [{}] Transform because its Attached Actor is INVALID"),
            InSensorEntity)
        { return; }

        const auto& Sensor = InCurrentComp.Get_Sensor().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(Sensor), TEXT("Invalid Sensor Actor Component stored for Sensor Entity [{}]"), InSensorEntity)
        { return; }

        const auto& BoneTransform = [&]() -> TOptional<FTransform>
        {
            const auto& Params           = InParamsComp.Get_Params();
            const auto& AttachmentParams = Params.Get_AttachmentParams();
            const auto& BoneName         = AttachmentParams.Get_BoneName();

            CK_ENSURE_IF_NOT(UCk_Utils_Actor_UE::Get_DoesBoneExistInSkeletalMesh(SensorAttachedActor.Get(), BoneName),
                TEXT("Sensor Entity [{}] cannot update its Transform according to Bone [{}] because its Attached Actor [{}] does NOT have it in its Skeletal Mesh"),
                InSensorEntity,
                BoneName,
                SensorAttachedEntityAndActor)
            { return {}; }

            const auto& AttachedActorSkeletalMeshComponent = SensorAttachedActor->FindComponentByClass<USkeletalMeshComponent>();
            const auto& AttachedActorTransform             = SensorAttachedActor->GetTransform();
            const auto& AttachmentPolicy                   = AttachmentParams.Get_AttachmentPolicy();
            const auto& SocketBoneName                     = AttachedActorSkeletalMeshComponent->GetSocketBoneName(BoneName);
            const auto& BoneIndex                          = AttachedActorSkeletalMeshComponent->GetBoneIndex(SocketBoneName);
            auto SkeletonTransform                         = AttachedActorSkeletalMeshComponent->GetBoneTransform(BoneIndex);

            if (NOT EnumHasAnyFlags(AttachmentPolicy, ECk_Sensor_AttachmentPolicy::UseBoneRotation))
            {
                SkeletonTransform.CopyRotation(AttachedActorTransform);
            }

            if (NOT EnumHasAnyFlags(AttachmentPolicy, ECk_Sensor_AttachmentPolicy::UseBonePosition))
            {
                SkeletonTransform.CopyTranslation(AttachedActorTransform);
            }

            return SkeletonTransform;
        }();

        if (ck::Is_NOT_Valid(BoneTransform))
        { return; }

        Sensor->SetWorldTransform(*BoneTransform);
        Sensor->AddLocalTransform(InParamsComp.Get_Params().Get_RelativeTransform());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Sensor_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InSensorEntity,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp) const
        -> void
    {
        if (InCurrentComp.Get_EnableDisable() == ECk_EnableDisable::Disable)
        { return; }

        InCurrentComp._EnableDisable = ECk_EnableDisable::Disable;

        // Since we are in the teardown, we are ok if the sensor object is pending kill
        const auto& Params     = InParamsComp.Get_Params();
        const auto& SensorName = Params.Get_SensorName();
        const auto& Sensor     = InCurrentComp.Get_Sensor().Get(true);

        UCk_Utils_Physics_UE::Request_SetGenerateOverlapEvents(Sensor, ECk_EnableDisable::Disable);
        UCk_Utils_Physics_UE::Request_SetCollisionEnabled(Sensor, ECollisionEnabled::NoCollision);

        const auto SensorBasicDetails =  FCk_Sensor_BasicDetails
        {
            InParamsComp.Get_Params().Get_SensorName(),
            InSensorEntity,
            InCurrentComp.Get_AttachedEntityAndActor()
        };

        const auto& DoManuallyTriggerAllEndOverlaps = [&]() -> void
        {
            for (const auto& OverlapKvp : InCurrentComp.Get_CurrentMarkerOverlaps().Get_Overlaps())
            {
                const auto& MarkerDetails = OverlapKvp.Key;
                const auto& OverlapDetails = OverlapKvp.Value.Get_OverlapDetails();

                const auto& OnEndOverlapPayload = FCk_Sensor_Payload_OnEndOverlap
                {
                    SensorBasicDetails,
                    MarkerDetails,
                    FCk_Sensor_EndOverlap_UnrealDetails
                    {
                        OverlapDetails.Get_OverlappedComponent().Get(),
                        OverlapDetails.Get_OtherActor().Get(),
                        OverlapDetails.Get_OtherComp().Get()
                    }
                };

                UUtils_Signal_OnSensorEndOverlap::Broadcast(InSensorEntity, MakePayload(
                    InCurrentComp.Get_AttachedEntityAndActor().Get_Handle(), OnEndOverlapPayload));
            }
        };
        DoManuallyTriggerAllEndOverlaps();
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_Sensor_DebugPreviewAll::
        FProcessor_Sensor_DebugPreviewAll(
        const FCk_Registry& InRegistry)
        : _Registry(InRegistry)
    {
    }

    auto
        FProcessor_Sensor_DebugPreviewAll::
        Tick(
            FCk_Time)
        -> void
    {
        if (NOT UCk_Utils_OverlapBody_UserSettings_UE::Get_DebugPreviewAllSensors())
        { return; }

        _Registry.View<FFragment_Sensor_Current, FFragment_Sensor_Params>().ForEach(
        [&](FCk_Entity InSensorEntity, const FFragment_Sensor_Current& InSensorCurrent, const FFragment_Sensor_Params& InSensorParams)
        {
            if (ck::Is_NOT_Valid(InSensorCurrent.Get_Sensor()))
            { return; }

            const auto& OuterForDebugDraw = InSensorCurrent.Get_AttachedEntityAndActor().Get_Actor().Get();

            const auto SensorHandle = UCk_Utils_Sensor_UE::CastChecked(FCk_Handle{InSensorEntity, _Registry});
            UCk_Utils_Sensor_UE::Preview(OuterForDebugDraw, SensorHandle);
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------