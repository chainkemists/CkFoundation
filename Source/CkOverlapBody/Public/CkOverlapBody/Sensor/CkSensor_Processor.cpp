#include "CkSensor_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Utils.h"
#include "CkOverlapBody/CkOverlapBody_Log.h"
#include "CkOverlapBody/Marker/CkMarker_Utils.h"
#include "CkOverlapBody/MarkerAndSensor/CkMarkerAndSensor_Utils.h"
#include "CkOverlapBody/Sensor/CkSensor_Utils.h"
#include "CkOverlapBody/Settings/CkOverlapBody_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Sensor_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InSensorEntity,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp) const
        -> void
    {
        InSensorEntity.Remove<MarkedDirtyBy>();

        const auto& owningEntity = UCk_Utils_OwningEntity::Get_StoredEntity(InSensorEntity);
        const auto& sensorAttachedEntityAndActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails(owningEntity);
        const auto& sensorAttachedEntity = sensorAttachedEntityAndActor.Get_Handle();

        CK_ENSURE_IF_NOT(ck::IsValid(sensorAttachedEntityAndActor.Get_Actor()),
            TEXT("Sensor Entity [{}] is attached to Entity [{}] that does NOT have a valid Actor"),
            InSensorEntity,
            sensorAttachedEntity)
        { return; }

        InCurrentComp._AttachedEntityAndActor = sensorAttachedEntityAndActor;

        const auto& params      = InParamsComp.Get_Params();
        const auto& shapeParams = params.Get_ShapeParams();

        switch (const auto& shapeType = shapeParams.Get_ShapeDimensions().Get_ShapeType())
        {
            case ECk_ShapeType::Box:
            {
                UCk_Utils_MarkerAndSensor_UE::Add_MarkerOrSensor_ActorComponent<UCk_Sensor_ActorComponent_Box_UE, ECk_ShapeType::Box>(InSensorEntity, sensorAttachedEntityAndActor, params);
                break;
            }
            case ECk_ShapeType::Sphere:
            {
                UCk_Utils_MarkerAndSensor_UE::Add_MarkerOrSensor_ActorComponent<UCk_Sensor_ActorComponent_Sphere_UE, ECk_ShapeType::Sphere>(InSensorEntity, sensorAttachedEntityAndActor, params);
                break;
            }
            case ECk_ShapeType::Capsule:
            {
                UCk_Utils_MarkerAndSensor_UE::Add_MarkerOrSensor_ActorComponent<UCk_Sensor_ActorComponent_Capsule_UE, ECk_ShapeType::Capsule>(InSensorEntity, sensorAttachedEntityAndActor, params);
                break;
            }
            default:
            {
                CK_INVALID_ENUM(shapeType);
                break;
            }
        }

        const auto& attachmentParams      = params.Get_AttachmentParams();
        const auto& useBoneTransformOrNot = attachmentParams.Get_UseBoneTransformOrNot();
        const auto& useBoneRotation       = attachmentParams.Get_UseBoneRotation();
        const auto& useBonePosition       = attachmentParams.Get_UseBonePosition();

        if (useBoneTransformOrNot == ECk_Sensor_BoneTransform_UsagePolicy::IgnoreBoneTransform)
        { return; }

        CK_ENSURE_IF_NOT(useBoneRotation == ECk_Sensor_BoneTransform_RotationPolicy::UseBoneRotation ||
                         useBonePosition == ECk_Sensor_BoneTransform_PositionPolicy::UseBonePosition,
            TEXT("Sensor Entity [{}] is set to use a Bone Transform, but its Attachment policies are [UseBoneRotation: {} | UseBonePosition: {}]"),
            InSensorEntity,
            useBoneRotation,
            useBonePosition)
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(attachmentParams.Get_BoneName()),
            TEXT("Sensor Entity [{}] uses Attachment policies [UseBoneRotation: {} | UseBonePosition: {}] but has an INVALID BoneName specified"),
            InSensorEntity,
            useBoneRotation,
            useBonePosition)
        { return; }

        const auto& attachedActorSkeletalMeshComponent = sensorAttachedEntityAndActor.Get_Actor()->FindComponentByClass<USkeletalMeshComponent>();

        CK_ENSURE_IF_NOT(ck::IsValid(attachedActorSkeletalMeshComponent),
            TEXT("Sensor Entity [{}] cannot use Attachment policies [UseBoneRotation: {} | UseBonePosition: {}] because it is attached to an Actor that has NO SkeletalMesh"),
            InSensorEntity,
            useBoneRotation,
            useBonePosition)
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
            FFragment_Sensor_Requests& InRequestsComp) const
        -> void
    {
        algo::ForEachRequest(InRequestsComp._EnableDisableRequest,
        [&](const FFragment_Sensor_Requests::EnableDisableRequestType& InRequest) -> void
        {
            DoHandleRequest(InHandle, InCurrentComp, InParamsComp, InRequest);
        });

        algo::ForEachRequest(InRequestsComp._BeginOrEndOverlapRequests, ck::Visitor(
        [&](const auto& InRequestVariant)
        {
            DoHandleRequest(InHandle, InCurrentComp, InParamsComp, InRequestVariant);
        }));

        InHandle.Remove<MarkedDirtyBy>();
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
            UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails_FromActor(Sensor->GetOwner())
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
            Get_EnableDisable(OverlappedMarkerEntity, OverlappedMarkerName);

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

        CK_ENSURE_IF_NOT(ck::IsValid(OverlappedMarkerEntity),
            TEXT("Sensor [Name: {} | Entity: {}] received EndOverlap with Marker [{}] that has an INVALID Entity"),
            SensorName,
            InSensorEntity,
            OverlappedMarkerName)
        { return; }

        if (NOT SensorFilteringInfo.Contains(OverlappedMarkerName))
        { return; }

        if (NOT InCurrentComp._CurrentMarkerOverlaps.Get_HasOverlapWithMarker(OverlappedMarkerDetails))
        { return; }

        const auto& OverlappedMarkerEnableDisable = UCk_Utils_Marker_UE::Get_EnableDisable(OverlappedMarkerEntity, OverlappedMarkerName);

        CK_ENSURE_IF_NOT(OverlappedMarkerEnableDisable == ECk_EnableDisable::Enable,
            TEXT("Sensor [Name: {} | Entity: {}] received EndOverlap with Marker [Name: {} | Entity: {}] that is DISABLED"),
            SensorName,
            InSensorEntity,
            OverlappedMarkerName,
            OverlappedMarkerEntity)
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

        const auto& sensorOwningEntity = UCk_Utils_OwningEntity::Get_StoredEntity(InSensorEntity);

        UUtils_Signal_OnSensorEndOverlap::Broadcast(InSensorEntity, MakePayload(
            sensorOwningEntity, OnEndOverlapPayload));
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

        const auto& sensor = InCurrentComp.Get_Sensor().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(sensor), TEXT("Invalid Sensor Actor Component stored for Sensor Entity [{}]"), InSensorEntity)
        { return; }

        const auto& BoneTransform = [&]() -> TOptional<FTransform>
        {
            const auto& Params           = InParamsComp.Get_Params();
            const auto& AttachmentParams = Params.Get_AttachmentParams();
            const auto& BoneName         = AttachmentParams.Get_BoneName();

            const auto& AttachedActorSkeletalMeshComponent = SensorAttachedActor->FindComponentByClass<USkeletalMeshComponent>();

            const auto& SocketBoneName = AttachedActorSkeletalMeshComponent->GetSocketBoneName(BoneName);
            const auto& BoneIndex      = AttachedActorSkeletalMeshComponent->GetBoneIndex(SocketBoneName);

            CK_ENSURE_IF_NOT(BoneIndex != INDEX_NONE,
                TEXT("Sensor Entity [{}] cannot update its Transform according to Bone [{}] because its Attached Actor [{}] does NOT have it in its Skeletal Mesh"),
                InSensorEntity,
                BoneName,
                SensorAttachedEntityAndActor)
            { return {}; }

            const auto AttachedActorTransform  = SensorAttachedActor->GetTransform();
            auto SkeletonTransform = AttachedActorSkeletalMeshComponent->GetBoneTransform(BoneIndex);

            switch(const auto& UseBoneRotation  = AttachmentParams.Get_UseBoneRotation())
            {
                case ECk_Sensor_BoneTransform_RotationPolicy::None:
                {
                    SkeletonTransform.CopyRotation(AttachedActorTransform);
                    break;
                }
                case ECk_Sensor_BoneTransform_RotationPolicy::UseBoneRotation:
                {
                    break;
                }
                default:
                {
                    CK_INVALID_ENUM(UseBoneRotation);
                    break;
                }
            }

            switch(const auto& UseBonePosition  = AttachmentParams.Get_UseBonePosition())
            {
                case ECk_Sensor_BoneTransform_PositionPolicy::None:
                {
                    SkeletonTransform.CopyTranslation(AttachedActorTransform);
                    break;
                }
                case ECk_Sensor_BoneTransform_PositionPolicy::UseBonePosition:
                {
                    break;
                }
                default:
                {
                    CK_INVALID_ENUM(UseBonePosition);
                    break;
                }
            }

            return SkeletonTransform;
        }();

        if (ck::Is_NOT_Valid(BoneTransform))
        { return; }

        sensor->SetWorldTransform(*BoneTransform);
        sensor->AddLocalTransform(InParamsComp.Get_Params().Get_RelativeTransform());
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

            const auto& SensorName = InSensorParams.Get_Params().Get_SensorName();
            const auto& OuterForDebugDraw = InSensorCurrent.Get_AttachedEntityAndActor().Get_Actor().Get();

            UCk_Utils_Sensor_UE::PreviewSensor(OuterForDebugDraw, FCk_Handle{ InSensorEntity, _Registry } ,SensorName);
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------
