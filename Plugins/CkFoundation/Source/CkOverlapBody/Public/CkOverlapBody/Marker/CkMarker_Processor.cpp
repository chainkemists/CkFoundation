#include "CkMarker_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkOverlapBody/Marker/CkMarker_Utils.h"

#include "CkOverlapBody/MarkerAndSensor/CkMarkerAndSensor_Utils.h"
#include "CkOverlapBody/Settings/CkOverlapBody_Settings.h"

#include "CkPhysics/CkPhysics_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FCk_Processor_Marker_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMarkerEntity,
            FCk_Fragment_Marker_Current& InCurrentComp,
            const FCk_Fragment_Marker_Params& InParamsComp) const
        -> void
    {
        InMarkerEntity.Remove<MarkedDirtyBy>();

        const auto& markerAttachedEntity = InParamsComp.Get_Params().Get_EntityAttachedTo();
        const auto& markerAttachedEntityAndActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails(markerAttachedEntity);

        CK_ENSURE_IF_NOT(ck::IsValid(markerAttachedEntityAndActor.Get_Actor()),
            TEXT("Marker Entity [{}] is Attached to Entity [{}] that does NOT have a valid Actor"),
            InMarkerEntity,
            markerAttachedEntity)
        { return; }

        InCurrentComp._AttachedEntityAndActor = markerAttachedEntityAndActor;

        const auto& params      = InParamsComp.Get_Params();
        const auto& shapeParams = params.Get_ShapeParams();

        switch (const auto& shapeType = shapeParams.Get_ShapeDimensions().Get_ShapeType())
        {
            case ECk_ShapeType::Box:
            {
                UCk_Utils_MarkerAndSensor_UE::Add_MarkerOrSensor_ActorComponent<UCk_Marker_ActorComponent_Box_UE, ECk_ShapeType::Box>(InMarkerEntity, markerAttachedEntityAndActor, params);
                break;
            }
            case ECk_ShapeType::Sphere:
            {
                UCk_Utils_MarkerAndSensor_UE::Add_MarkerOrSensor_ActorComponent<UCk_Marker_ActorComponent_Sphere_UE, ECk_ShapeType::Sphere>(InMarkerEntity, markerAttachedEntityAndActor, params);
                break;
            }
            case ECk_ShapeType::Capsule:
            {
                UCk_Utils_MarkerAndSensor_UE::Add_MarkerOrSensor_ActorComponent<UCk_Marker_ActorComponent_Capsule_UE, ECk_ShapeType::Capsule>(InMarkerEntity, markerAttachedEntityAndActor, params);
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

        if (useBoneTransformOrNot == ECk_Marker_BoneTransform_UsagePolicy::IgnoreBoneTransform)
        { return; }

        CK_ENSURE_IF_NOT(useBoneRotation == ECk_Marker_BoneTransform_RotationPolicy::UseBoneRotation ||
                         useBonePosition == ECk_Marker_BoneTransform_PositionPolicy::UseBonePosition,
            TEXT("Marker Entity [{}] is set to use a Bone Transform, but its Attachment policies are [UseBoneRotation: {} | UseBonePosition: {}]"),
            InMarkerEntity,
            useBoneRotation,
            useBonePosition)
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(attachmentParams.Get_BoneName()),
            TEXT("Marker Entity [{}] uses Attachment policies [UseBoneRotation: {} | UseBonePosition: {}] but has an INVALID BoneName specified"),
            InMarkerEntity,
            useBoneRotation,
            useBonePosition)
        { return; }

        const auto& attachedActorSkeletalMeshComponent = markerAttachedEntityAndActor.Get_Actor()->FindComponentByClass<USkeletalMeshComponent>();

        CK_ENSURE_IF_NOT(ck::IsValid(attachedActorSkeletalMeshComponent),
            TEXT("Marker Entity [{}] cannot use Attachment policies [UseBoneRotation: {} | UseBonePosition: {}] because it is attached to an Actor that has NO SkeletalMesh"),
            InMarkerEntity,
            useBoneRotation,
            useBonePosition)
        { return; }

        UCk_Utils_Marker_UE::Request_MarkMarker_AsNeedToUpdateTransform(InMarkerEntity);
    }

    // --------------------------------------------------------

    auto
        FCk_Processor_Marker_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMarkerEntity,
            FCk_Fragment_Marker_Current& InCurrentComp,
            const FCk_Fragment_Marker_Params& InParamsComp,
            FCk_Fragment_Marker_Requests& InRequestsComp) const
        -> void
    {
        ck::algo::ForEachRequest(InRequestsComp._Requests,
        [&](const FCk_Fragment_Marker_Requests::RequestType& InRequest) -> void
        {
            const auto& currentEnableDisable = InCurrentComp.Get_EnableDisable();
            const auto& newEnableDisable = InRequest.Get_EnableDisable();

            if (currentEnableDisable == newEnableDisable)
            { return; }

            InCurrentComp._EnableDisable = newEnableDisable;
            const auto& collisionEnabled = newEnableDisable == ECk_EnableDisable::Enable
                                             ? ECollisionEnabled::QueryOnly
                                             : ECollisionEnabled::NoCollision;

            const auto& params     = InParamsComp.Get_Params();
            const auto& markerName = params.Get_MarkerName();
            const auto& marker     = InCurrentComp.Get_Marker().Get();

            UCk_Utils_Physics_UE::Request_SetGenerateOverlapEvents(marker, newEnableDisable);
            UCk_Utils_Physics_UE::Request_SetCollisionEnabled(marker, collisionEnabled);

            // TODO: Fire Marker EnableDisable signal
        });

        InMarkerEntity.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FCk_Processor_Marker_UpdateTransform::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMarkerEntity,
            FCk_Fragment_Marker_Current& InCurrentComp,
            const FCk_Fragment_Marker_Params& InParamsComp) const
        -> void
    {
        // TODO: Extract this in a common function to avoid code duplication between similar system for Sensor
        const auto& markerAttachedEntityAndActor = InCurrentComp.Get_AttachedEntityAndActor();
        const auto& markerAttachedActor          = markerAttachedEntityAndActor.Get_Actor();

        CK_ENSURE_IF_NOT(ck::IsValid(markerAttachedActor),
            TEXT("Cannot update Marker [{}] Transform because its Attached Actor is INVALID"),
            InMarkerEntity)
        { return; }

        const auto& marker = InCurrentComp.Get_Marker().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(marker), TEXT("Invalid Marker Actor Component stored for Marker Entity [{}]"), InMarkerEntity)
        { return; }

        const auto& boneTransform = [&]() -> TOptional<FTransform>
        {
            const auto& params           = InParamsComp.Get_Params();
            const auto& attachmentParams = params.Get_AttachmentParams();
            const auto& boneName         = attachmentParams.Get_BoneName();

            const auto& attachedActorSkeletalMeshComponent = markerAttachedActor->FindComponentByClass<USkeletalMeshComponent>();

            const auto& socketBoneName = attachedActorSkeletalMeshComponent->GetSocketBoneName(boneName);
            const auto& boneIndex      = attachedActorSkeletalMeshComponent->GetBoneIndex(socketBoneName);

            CK_ENSURE_IF_NOT(boneIndex != INDEX_NONE,
                TEXT("Marker Entity [{}] cannot update its Transform according to Bone [{}] because its Attached Actor [{}] does NOT have it in its Skeletal Mesh"),
                InMarkerEntity,
                boneName,
                markerAttachedEntityAndActor)
            { return {}; }

            const auto attachedActorTransform  = markerAttachedActor->GetTransform();
            auto skeletonTransform = attachedActorSkeletalMeshComponent->GetBoneTransform(boneIndex);

            switch(const auto& useBoneRotation  = attachmentParams.Get_UseBoneRotation())
            {
                case ECk_Marker_BoneTransform_RotationPolicy::None:
                {
                    skeletonTransform.CopyRotation(attachedActorTransform);
                    break;
                }
                case ECk_Marker_BoneTransform_RotationPolicy::UseBoneRotation:
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
                case ECk_Marker_BoneTransform_PositionPolicy::None:
                {
                    skeletonTransform.CopyTranslation(attachedActorTransform);
                    break;
                }
                case ECk_Marker_BoneTransform_PositionPolicy::UseBonePosition:
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

        marker->SetWorldTransform(*boneTransform);
        marker->AddLocalTransform(InParamsComp.Get_Params().Get_RelativeTransform());
    }

    // --------------------------------------------------------------------------------------------------------------------

    FCk_Processor_Marker_DebugPreviewAll::
        FCk_Processor_Marker_DebugPreviewAll(
            FCk_Registry& InRegistry)
        : _Registry(InRegistry)
    {
    }

    auto
        FCk_Processor_Marker_DebugPreviewAll::
        Tick(
            FCk_Time)
        -> void
    {
        if (NOT UCk_Utils_OverlapBody_UserSettings_UE::Get_DebugPreviewAllMarkers())
        { return; }

        _Registry.View<FCk_Fragment_Marker_Current, FCk_Fragment_Marker_Params>().ForEach(
        [&](FCk_Entity InMarkerEntity, const FCk_Fragment_Marker_Current& InMarkerCurrent, const FCk_Fragment_Marker_Params& InMarkerParams)
        {
            if (ck::Is_NOT_Valid(InMarkerCurrent.Get_Marker()))
            { return; }

            const auto& markerName = InMarkerParams.Get_Params().Get_MarkerName();
            const auto& outerForDebugDraw = InMarkerCurrent.Get_AttachedEntityAndActor().Get_Actor().Get();

            UCk_Utils_Marker_UE::PreviewSingleMarker<ECk_FragmentQuery_Policy::CurrentEntity>(outerForDebugDraw, FCk_Handle{ InMarkerEntity, _Registry }, markerName);
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------
