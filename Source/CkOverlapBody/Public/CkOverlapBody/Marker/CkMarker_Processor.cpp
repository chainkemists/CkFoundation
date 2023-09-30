#include "CkMarker_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Utils.h"
#include "CkOverlapBody/Marker/CkMarker_Utils.h"

#include "CkOverlapBody/MarkerAndSensor/CkMarkerAndSensor_Utils.h"
#include "CkOverlapBody/Settings/CkOverlapBody_Settings.h"

#include "CkPhysics/CkPhysics_Utils.h"

#include <Components/SkeletalMeshComponent.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Marker_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMarkerEntity,
            FFragment_Marker_Current& InCurrentComp,
            const FFragment_Marker_Params& InParamsComp) const
        -> void
    {
        InMarkerEntity.Remove<MarkedDirtyBy>();

        const auto& markerOwningEntity = UCk_Utils_OwningEntity::Get_StoredEntity(InMarkerEntity);
        const auto& markerAttachedEntityAndActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails(markerOwningEntity);
        const auto& markerAttachedEntity = markerAttachedEntityAndActor.Get_Handle();


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

        const auto& attachmentParams = params.Get_AttachmentParams();
        const auto& useBoneTransform = EnumHasAnyFlags(attachmentParams.Get_AttachmentPolicy(), ECk_Marker_AttachmentPolicy::UseBonePosition | ECk_Marker_AttachmentPolicy::UseBoneRotation);

        if (NOT useBoneTransform)
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(attachmentParams.Get_BoneName()),
            TEXT("Marker Entity [{}] uses Attachment Policy [UseBonePosition: {}, UseBoneRotation: {}] but has an INVALID BoneName specified"),
            InMarkerEntity,
            EnumHasAnyFlags(attachmentParams.Get_AttachmentPolicy(), ECk_Marker_AttachmentPolicy::UseBonePosition),
            EnumHasAnyFlags(attachmentParams.Get_AttachmentPolicy(), ECk_Marker_AttachmentPolicy::UseBoneRotation))
        { return; }

        const auto& attachedActorSkeletalMeshComponent = markerAttachedEntityAndActor.Get_Actor()->FindComponentByClass<USkeletalMeshComponent>();

        CK_ENSURE_IF_NOT(ck::IsValid(attachedActorSkeletalMeshComponent),
            TEXT("Marker Entity [{}] cannot use Attachment Policy [UseBonePosition: {}, UseBoneRotation: {}] because it is attached to an Actor that has NO SkeletalMesh"),
            InMarkerEntity,
            EnumHasAnyFlags(attachmentParams.Get_AttachmentPolicy(), ECk_Marker_AttachmentPolicy::UseBonePosition),
            EnumHasAnyFlags(attachmentParams.Get_AttachmentPolicy(), ECk_Marker_AttachmentPolicy::UseBoneRotation))
        { return; }

        UCk_Utils_Marker_UE::Request_MarkMarker_AsNeedToUpdateTransform(InMarkerEntity);
    }

    // --------------------------------------------------------

    auto
        FProcessor_Marker_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMarkerEntity,
            FFragment_Marker_Current& InCurrentComp,
            const FFragment_Marker_Params& InParamsComp,
            FFragment_Marker_Requests& InRequestsComp) const
        -> void
    {
        ck::algo::ForEachRequest(InRequestsComp._Requests,
        [&](const FFragment_Marker_Requests::RequestType& InRequest) -> void
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

            UUtils_Signal_OnMarkerEnableDisable::Broadcast(InMarkerEntity, MakePayload(InCurrentComp.Get_AttachedEntityAndActor().Get_Handle(), markerName, newEnableDisable));
        });

        InMarkerEntity.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Marker_UpdateTransform::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMarkerEntity,
            FFragment_Marker_Current& InCurrentComp,
            const FFragment_Marker_Params& InParamsComp) const
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

            const auto& attachmentPolicy = attachmentParams.Get_AttachmentPolicy();

            if (NOT EnumHasAnyFlags(attachmentPolicy, ECk_Marker_AttachmentPolicy::UseBoneRotation))
            {
                skeletonTransform.CopyRotation(attachedActorTransform);
            }

            if (NOT EnumHasAnyFlags(attachmentPolicy, ECk_Marker_AttachmentPolicy::UseBonePosition))
            {
                skeletonTransform.CopyTranslation(attachedActorTransform);
            }

            return skeletonTransform;
        }();

        if (ck::Is_NOT_Valid(boneTransform))
        { return; }

        marker->SetWorldTransform(*boneTransform);
        marker->AddLocalTransform(InParamsComp.Get_Params().Get_RelativeTransform());
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_Marker_DebugPreviewAll::
        FProcessor_Marker_DebugPreviewAll(
            FCk_Registry& InRegistry)
        : _Registry(InRegistry)
    {
    }

    auto
        FProcessor_Marker_DebugPreviewAll::
        Tick(
            FCk_Time)
        -> void
    {
        if (NOT UCk_Utils_OverlapBody_UserSettings_UE::Get_DebugPreviewAllMarkers())
        { return; }

        _Registry.View<FFragment_Marker_Current, FFragment_Marker_Params>().ForEach(
        [&](FCk_Entity InMarkerEntity, const FFragment_Marker_Current& InMarkerCurrent, const FFragment_Marker_Params& InMarkerParams)
        {
            if (ck::Is_NOT_Valid(InMarkerCurrent.Get_Marker()))
            { return; }

            const auto& markerName = InMarkerParams.Get_Params().Get_MarkerName();
            const auto& outerForDebugDraw = InMarkerCurrent.Get_AttachedEntityAndActor().Get_Actor().Get();

            UCk_Utils_Marker_UE::PreviewMarker(outerForDebugDraw, FCk_Handle{ InMarkerEntity, _Registry }, markerName);
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------
