#include "CkSensor_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkOverlapBody/CkOverlapBody_Log.h"
#include "CkOverlapBody/Marker/CkMarker_Utils.h"
#include "CkOverlapBody/MarkerAndSensor/CkMarkerAndSensor_Utils.h"
#include "CkOverlapBody/Sensor/CkSensor_Utils.h"
#include "CkOverlapBody/Settings/CkOverlapBody_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FCk_Processor_Sensor_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InSensorEntity,
            FCk_Fragment_Sensor_Current& InCurrentComp,
            const FCk_Fragment_Sensor_Params& InParamsComp) const
        -> void
    {
        InSensorEntity.Remove<MarkedDirtyBy>();

        const auto& sensorAttachedEntity = InParamsComp.Get_Params().Get_EntityAttachedTo();
        const auto& sensorAttachedEntityAndActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails(sensorAttachedEntity);

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
        FCk_Processor_Sensor_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FCk_Fragment_Sensor_Current& InCurrentComp,
            const FCk_Fragment_Sensor_Params& InParamsComp,
            FCk_Fragment_Sensor_Requests& InRequestsComp) const
        -> void
    {
        algo::ForEachRequest(InRequestsComp._EnableDisableRequest,
        [&](const FCk_Fragment_Sensor_Requests::EnableDisableRequestType& InRequest) -> void
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
        FCk_Processor_Sensor_HandleRequests::
        DoHandleRequest(
            HandleType InSensorEntity,
            FCk_Fragment_Sensor_Current& InCurrentComp,
            const FCk_Fragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_EnableDisable& InRequest) const
        -> void
    {
        const auto& DoManuallyTriggerAllEndOverlaps = [&](const AActor* InSensorAttachedActor) -> void
        {
            const auto sensorBasicDetails =  FCk_Sensor_BasicDetails
            {
                InParamsComp.Get_Params().Get_SensorName(),
                InSensorEntity,
                UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails_FromActor(InSensorAttachedActor)
            };

            for (const auto& overlapKvp : InCurrentComp.Get_CurrentMarkerOverlaps().Get_Overlaps())
            {
                const auto& markerDetails = overlapKvp.Key;
                const auto& overlapDetails = overlapKvp.Value.Get_OverlapDetails();

                const auto& onEndOverlapPayload = FCk_Sensor_Payload_OnEndOverlap
                {
                    sensorBasicDetails,
                    markerDetails,
                    FCk_Sensor_EndOverlap_UnrealDetails
                    {
                        overlapDetails.Get_OverlappedComponent().Get(),
                        overlapDetails.Get_OtherActor().Get(),
                        overlapDetails.Get_OtherComp().Get()
                    }
                };

                // TODO: Fire Sensor OnEndOverlap signal
            }
        };

        const auto& currentEnableDisable = InCurrentComp.Get_EnableDisable();
        const auto& newEnableDisable     = InRequest.Get_EnableDisable();

        if (currentEnableDisable == newEnableDisable)
        { return; }

        InCurrentComp._EnableDisable = newEnableDisable;

        const auto& collisionEnabled = newEnableDisable == ECk_EnableDisable::Enable
                                         ? ECollisionEnabled::QueryOnly
                                         : ECollisionEnabled::NoCollision;

        const auto& params     = InParamsComp.Get_Params();
        const auto& sensorName = params.Get_SensorName();
        const auto& sensor     = InCurrentComp.Get_Sensor().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(sensor), TEXT("Entity [{}] has an Invalid Sensor stored!"), InSensorEntity)
        { return; }

        if (newEnableDisable == ECk_EnableDisable::Disable)
        {
            DoManuallyTriggerAllEndOverlaps(sensor->GetOwner());
            InCurrentComp._CurrentMarkerOverlaps = {};
        }

        UCk_Utils_Physics_UE::Request_SetGenerateOverlapEvents(sensor, newEnableDisable);
        UCk_Utils_Physics_UE::Request_SetCollisionEnabled(sensor, collisionEnabled);

        // TODO: Fire Sensor EnableDisable signal
    }

    auto
        FCk_Processor_Sensor_HandleRequests::
        DoHandleRequest(
            HandleType InSensorEntity,
            FCk_Fragment_Sensor_Current& InCurrentComp,
            const FCk_Fragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_OnBeginOverlap& InRequest) const
        -> void
    {
        const auto& overlappedMarkerDetails = InRequest.Get_MarkerDetails();
        const auto& overlappedMarkerName    = overlappedMarkerDetails.Get_MarkerName();
        const auto& overlappedMarkerEntity  = overlappedMarkerDetails.Get_MarkerEntity();

        const auto& sensorName           = InRequest.Get_SensorDetails().Get_SensorName();
        const auto& sensorFilteringInfo  = InParamsComp.Get_Params().Get_FilteringParams().Get_MarkerNames();

        CK_ENSURE_IF_NOT(ck::IsValid(overlappedMarkerEntity),
            TEXT("Sensor [Name: {} | Entity: {}] received EndOverlap with Marker [{}] that has an INVALID Entity"),
            sensorName,
            InSensorEntity,
            overlappedMarkerName)
        { return; }

        if (NOT sensorFilteringInfo.Contains(overlappedMarkerName))
        { return; }

        if (InCurrentComp._CurrentMarkerOverlaps.Get_HasOverlapWithMarker(overlappedMarkerDetails))
        { return; }

        const auto& overlappedMarkerEnableDisable = UCk_Utils_Marker_UE::Get_EnableDisable<ECk_FragmentQuery_Policy::CurrentEntity>(overlappedMarkerEntity, overlappedMarkerName);

        CK_ENSURE_IF_NOT(overlappedMarkerEnableDisable == ECk_EnableDisable::Enable,
            TEXT("Sensor [Name: {} | Entity: {}] received BeginOverlap with Marker [Name: {} | Entity: {}] that is DISABLED"),
            sensorName,
            InSensorEntity,
            overlappedMarkerName,
            overlappedMarkerEntity)
        { return; }

        overlap_body::VeryVerbose
        (
            TEXT("Sensor [Name: {} | Entity: {}] BeginOverlap with Marker [Name: {} | Entity: {}]"),
            sensorName,
            InSensorEntity,
            overlappedMarkerName,
            overlappedMarkerEntity
        );

        const auto& overlapDetails = InRequest.Get_OverlapDetails();

        InCurrentComp._CurrentMarkerOverlaps.Process_Add(FCk_Sensor_MarkerOverlapInfo{ overlappedMarkerDetails, overlapDetails });

        const auto& onBeginOverlapPayload = FCk_Sensor_Payload_OnBeginOverlap
        {
            InRequest.Get_SensorDetails(),
            overlappedMarkerDetails,
            overlapDetails
        };

        // TODO: Fire Sensor OnBeginOverlap signal
    }

    auto
        FCk_Processor_Sensor_HandleRequests::
        DoHandleRequest(
            HandleType InSensorEntity,
            FCk_Fragment_Sensor_Current& InCurrentComp,
            const FCk_Fragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_OnEndOverlap& InRequest) const
        -> void
    {
        const auto& overlappedMarkerDetails = InRequest.Get_MarkerDetails();
        const auto& overlappedMarkerName    = overlappedMarkerDetails.Get_MarkerName();
        const auto& overlappedMarkerEntity  = overlappedMarkerDetails.Get_MarkerEntity();

        const auto& sensorName           = InRequest.Get_SensorDetails().Get_SensorName();
        const auto& sensorFilteringInfo  = InParamsComp.Get_Params().Get_FilteringParams().Get_MarkerNames();

        CK_ENSURE_IF_NOT(ck::IsValid(overlappedMarkerEntity),
            TEXT("Sensor [Name: {} | Entity: {}] received EndOverlap with Marker [{}] that has an INVALID Entity"),
            sensorName,
            InSensorEntity,
            overlappedMarkerName)
        { return; }

        if (NOT sensorFilteringInfo.Contains(overlappedMarkerName))
        { return; }

        if (NOT InCurrentComp._CurrentMarkerOverlaps.Get_HasOverlapWithMarker(overlappedMarkerDetails))
        { return; }

        const auto& overlappedMarkerEnableDisable = UCk_Utils_Marker_UE::Get_EnableDisable<ECk_FragmentQuery_Policy::CurrentEntity>(overlappedMarkerEntity, overlappedMarkerName);

        CK_ENSURE_IF_NOT(overlappedMarkerEnableDisable == ECk_EnableDisable::Enable,
            TEXT("Sensor [Name: {} | Entity: {}] received EndOverlap with Marker [Name: {} | Entity: {}] that is DISABLED"),
            sensorName,
            InSensorEntity,
            overlappedMarkerName,
            overlappedMarkerEntity)
        { return; }

        overlap_body::VeryVerbose
        (
            TEXT("Sensor [Name: {} | Entity: {}] EndOverlap with Marker [Name: {} | Entity: {}]"),
            sensorName,
            InSensorEntity,
            overlappedMarkerName,
            overlappedMarkerEntity
        );

        const auto& overlapDetails = InRequest.Get_OverlapDetails();

        InCurrentComp._CurrentMarkerOverlaps.Process_RemoveOverlapWithMarker(overlappedMarkerDetails);

        const auto& onEndOverlapPayload = FCk_Sensor_Payload_OnEndOverlap
        {
            InRequest.Get_SensorDetails(),
            overlappedMarkerDetails,
            overlapDetails
        };

        // TODO: Fire Sensor OnEndOverlap signal
    }

    auto
        FCk_Processor_Sensor_HandleRequests::
        DoHandleRequest(
            HandleType InSensorEntity,
            FCk_Fragment_Sensor_Current& InCurrentComp,
            const FCk_Fragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_OnBeginOverlap_NonMarker& InRequest) const
        -> void
    {
        const auto& overlapDetails = InRequest.Get_OverlapDetails();
        const auto& nonMarkerOverlapInfo = FCk_Sensor_NonMarkerOverlapInfo{overlapDetails};

        if (InCurrentComp._CurrentNonMarkerOverlaps.Get_Overlaps().Contains(nonMarkerOverlapInfo))
        { return; }

        overlap_body::VeryVerbose
        (
            TEXT("Sensor [Name: {} | Entity: {}] BeginOverlap with Non-Marker [{}]"),
            InRequest.Get_SensorDetails().Get_SensorName(),
            InSensorEntity,
            overlapDetails
        );

        InCurrentComp._CurrentNonMarkerOverlaps.Process_Add(nonMarkerOverlapInfo);

        const auto& onBeginOverlapPayload = FCk_Sensor_Payload_OnBeginOverlap_NonMarker
        {
            InRequest.Get_SensorDetails(),
            overlapDetails
        };

        // TODO: Fire Sensor OnBeginOverlap_NonMarker signal
    }

    auto
        FCk_Processor_Sensor_HandleRequests::
        DoHandleRequest(
            HandleType InSensorEntity,
            FCk_Fragment_Sensor_Current& InCurrentComp,
            const FCk_Fragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_OnEndOverlap_NonMarker& InRequest) const
        -> void
    {
        const auto& overlapDetails = InRequest.Get_OverlapDetails();
        const auto& nonMarkerOverlapInfo = FCk_Sensor_NonMarkerOverlapInfo
        {
            FCk_Sensor_BeginOverlap_UnrealDetails
            {
                overlapDetails.Get_OverlappedComponent().Get(),
                overlapDetails.Get_OtherActor().Get(),
                overlapDetails.Get_OtherComp().Get()
            }
        };

        if (NOT InCurrentComp._CurrentNonMarkerOverlaps.Get_Overlaps().Contains(nonMarkerOverlapInfo))
        { return; }

        overlap_body::VeryVerbose
        (
            TEXT("Sensor [Name: {} | Entity: {}] EndOverlap with Non-Marker [{}]"),
            InRequest.Get_SensorDetails().Get_SensorName(),
            InSensorEntity,
            overlapDetails
        );

        InCurrentComp._CurrentNonMarkerOverlaps.Process_Remove(nonMarkerOverlapInfo);

        const auto& onEndOverlapPayload = FCk_Sensor_Payload_OnEndOverlap_NonMarker
        {
            InRequest.Get_SensorDetails(),
            overlapDetails
        };

        // TODO: Fire Sensor OnEndOverlap_NonMarker signal
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FCk_Processor_Sensor_UpdateTransform::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InSensorEntity,
            FCk_Fragment_Sensor_Current& InCurrentComp,
            const FCk_Fragment_Sensor_Params& InParamsComp) const
        -> void
    {
        // TODO: Extract this in a common function to avoid code duplication between similar system for Marker
        const auto& sensorAttachedEntityAndActor = InCurrentComp.Get_AttachedEntityAndActor();
        const auto& sensorAttachedActor          = sensorAttachedEntityAndActor.Get_Actor();

        CK_ENSURE_IF_NOT(ck::IsValid(sensorAttachedActor),
            TEXT("Cannot update Sensor [{}] Transform because its Attached Actor is INVALID"),
            InSensorEntity)
        { return; }

        const auto& sensor = InCurrentComp.Get_Sensor().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(sensor), TEXT("Invalid Sensor Actor Component stored for Sensor Entity [{}]"), InSensorEntity)
        { return; }

        const auto& boneTransform = [&]() -> TOptional<FTransform>
        {
            const auto& params           = InParamsComp.Get_Params();
            const auto& attachmentParams = params.Get_AttachmentParams();
            const auto& boneName         = attachmentParams.Get_BoneName();

            const auto& attachedActorSkeletalMeshComponent = sensorAttachedActor->FindComponentByClass<USkeletalMeshComponent>();

            const auto& socketBoneName = attachedActorSkeletalMeshComponent->GetSocketBoneName(boneName);
            const auto& boneIndex      = attachedActorSkeletalMeshComponent->GetBoneIndex(socketBoneName);

            CK_ENSURE_IF_NOT(boneIndex != INDEX_NONE,
                TEXT("Sensor Entity [{}] cannot update its Transform according to Bone [{}] because its Attached Actor [{}] does NOT have it in its Skeletal Mesh"),
                InSensorEntity,
                boneName,
                sensorAttachedEntityAndActor)
            { return {}; }

            const auto attachedActorTransform  = sensorAttachedActor->GetTransform();
            auto skeletonTransform = attachedActorSkeletalMeshComponent->GetBoneTransform(boneIndex);

            switch(const auto& useBoneRotation  = attachmentParams.Get_UseBoneRotation())
            {
                case ECk_Sensor_BoneTransform_RotationPolicy::None:
                {
                    skeletonTransform.CopyRotation(attachedActorTransform);
                    break;
                }
                case ECk_Sensor_BoneTransform_RotationPolicy::UseBoneRotation:
                {
                    break;
                }
                default:
                {
                    CK_INVALID_ENUM(useBoneRotation);
                    break;
                }
            }

            switch(const auto& useBonePosition  = attachmentParams.Get_UseBonePosition())
            {
                case ECk_Sensor_BoneTransform_PositionPolicy::None:
                {
                    skeletonTransform.CopyTranslation(attachedActorTransform);
                    break;
                }
                case ECk_Sensor_BoneTransform_PositionPolicy::UseBonePosition:
                {
                    break;
                }
                default:
                {
                    CK_INVALID_ENUM(useBonePosition);
                    break;
                }
            }

            return skeletonTransform;
        }();

        if (ck::Is_NOT_Valid(boneTransform))
        { return; }

        sensor->SetWorldTransform(*boneTransform);
        sensor->AddLocalTransform(InParamsComp.Get_Params().Get_RelativeTransform());
    }

    // --------------------------------------------------------------------------------------------------------------------

    FCk_Processor_Sensor_DebugPreviewAll::
        FCk_Processor_Sensor_DebugPreviewAll(
            FCk_Registry& InRegistry)
        : _Registry(InRegistry)
    {
    }

    auto
        FCk_Processor_Sensor_DebugPreviewAll::
        Tick(
            FCk_Time)
        -> void
    {
        if (NOT UCk_Utils_OverlapBody_UserSettings_UE::Get_DebugPreviewAllSensors())
        { return; }

        _Registry.View<FCk_Fragment_Sensor_Current, FCk_Fragment_Sensor_Params>().ForEach(
        [&](FCk_Entity InSensorEntity, const FCk_Fragment_Sensor_Current& InSensorCurrent, const FCk_Fragment_Sensor_Params& InSensorParams)
        {
            if (ck::Is_NOT_Valid(InSensorCurrent.Get_Sensor()))
            { return; }

            const auto& sensorName = InSensorParams.Get_Params().Get_SensorName();
            const auto& outerForDebugDraw = InSensorCurrent.Get_AttachedEntityAndActor().Get_Actor().Get();

            UCk_Utils_Sensor_UE::PreviewSingleSensor<ECk_FragmentQuery_Policy::CurrentEntity>(outerForDebugDraw, FCk_Handle{ InSensorEntity, _Registry } ,sensorName);
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------
