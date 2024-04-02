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
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Marker_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMarkerEntity,
            FFragment_Marker_Current& InCurrentComp,
            const FFragment_Marker_Params& InParamsComp) const
        -> void
    {
        const auto& MarkerOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InMarkerEntity);
        const auto& MarkerOwningActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(MarkerOwner);

        constexpr auto IncludePendingKill = true;
        CK_ENSURE_IF_NOT(ck::IsValid(MarkerOwningActor, ck::IsValid_Policy_IncludePendingKill{}),
            TEXT("Marker Entity [{}] has a MarkerOwner [{}] that does NOT have a valid Actor"),
            InMarkerEntity,
            MarkerOwner)
        { return; }

        if (ck::Is_NOT_Valid(MarkerOwningActor))
        { return; }

        const auto& Params        = InParamsComp.Get_Params();
        const auto& ShapeParams   = Params.Get_ShapeParams();
        const auto& PhysicsParams = Params.Get_PhysicsParams();

        CK_ENSURE_IF_NOT
        (
            PhysicsParams.Get_CollisionProfileName().Name != TEXT("NoCollision"),
            TEXT("Marker [{}] added to Entity [{}] has Collision Profile set to [{}]. Marker will NOT work properly!"),
            Params.Get_MarkerName(),
            MarkerOwner,
            PhysicsParams.Get_CollisionProfileName()
        ) {};

        switch (const auto& ShapeType = ShapeParams.Get_ShapeDimensions().Get_ShapeType())
        {
            case ECk_ShapeType::Box:
            {
                UCk_Utils_MarkerAndSensor_UE::Add_MarkerOrSensor_ActorComponent<UCk_Marker_ActorComponent_Box_UE, ECk_ShapeType::Box>(InMarkerEntity, MarkerOwningActor, Params);
                break;
            }
            case ECk_ShapeType::Sphere:
            {
                UCk_Utils_MarkerAndSensor_UE::Add_MarkerOrSensor_ActorComponent<UCk_Marker_ActorComponent_Sphere_UE, ECk_ShapeType::Sphere>(InMarkerEntity, MarkerOwningActor, Params);
                break;
            }
            case ECk_ShapeType::Capsule:
            {
                UCk_Utils_MarkerAndSensor_UE::Add_MarkerOrSensor_ActorComponent<UCk_Marker_ActorComponent_Capsule_UE, ECk_ShapeType::Capsule>(InMarkerEntity, MarkerOwningActor, Params);
                break;
            }
            default:
            {
                CK_INVALID_ENUM(ShapeType);
                break;
            }
        }

        const auto& AttachmentParams = Params.Get_AttachmentParams();
        const auto& UseBoneTransform = EnumHasAnyFlags(AttachmentParams.Get_AttachmentPolicy(), ECk_Marker_AttachmentPolicy::UseBonePosition | ECk_Marker_AttachmentPolicy::UseBoneRotation);

        if (NOT UseBoneTransform)
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(AttachmentParams.Get_BoneName()),
            TEXT("Marker Entity [{}] uses Attachment Policy [UseBonePosition: {}, UseBoneRotation: {}] but has an INVALID BoneName specified"),
            InMarkerEntity,
            EnumHasAnyFlags(AttachmentParams.Get_AttachmentPolicy(), ECk_Marker_AttachmentPolicy::UseBonePosition),
            EnumHasAnyFlags(AttachmentParams.Get_AttachmentPolicy(), ECk_Marker_AttachmentPolicy::UseBoneRotation))
        { return; }

        const auto& AttachedActorSkeletalMeshComponent = MarkerOwningActor.Get_Actor()->FindComponentByClass<USkeletalMeshComponent>();

        CK_ENSURE_IF_NOT(ck::IsValid(AttachedActorSkeletalMeshComponent),
            TEXT("Marker Entity [{}] cannot use Attachment Policy [UseBonePosition: {}, UseBoneRotation: {}] because it is attached to an Actor that has NO SkeletalMesh"),
            InMarkerEntity,
            EnumHasAnyFlags(AttachmentParams.Get_AttachmentPolicy(), ECk_Marker_AttachmentPolicy::UseBonePosition),
            EnumHasAnyFlags(AttachmentParams.Get_AttachmentPolicy(), ECk_Marker_AttachmentPolicy::UseBoneRotation))
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
            const FFragment_Marker_Requests& InRequestsComp) const
        -> void
    {
        InMarkerEntity.CopyAndRemove(InRequestsComp, [&](const FFragment_Marker_Requests& InRequests)
        {
            ck::algo::ForEachRequest(InRequests._Requests,
            [&](const FFragment_Marker_Requests::RequestType& InRequest) -> void
            {
                const auto& CurrentEnableDisable = InCurrentComp.Get_EnableDisable();
                const auto& NewEnableDisable = InRequest.Get_EnableDisable();

                if (CurrentEnableDisable == NewEnableDisable)
                { return; }

                InCurrentComp._EnableDisable = NewEnableDisable;
                const auto& CollisionEnabled = NewEnableDisable == ECk_EnableDisable::Enable
                                                 ? ECollisionEnabled::QueryOnly
                                                 : ECollisionEnabled::NoCollision;

                const auto& Marker = InCurrentComp.Get_Marker().Get();

                UCk_Utils_Physics_UE::Request_SetGenerateOverlapEvents(Marker, NewEnableDisable);
                UCk_Utils_Physics_UE::Request_SetCollisionEnabled(Marker, CollisionEnabled);

                const auto MarkerBasicInfo = FCk_Marker_BasicInfo{InMarkerEntity, UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InMarkerEntity)};

                UUtils_Signal_OnMarkerEnableDisable::Broadcast(InMarkerEntity, MakePayload(MarkerBasicInfo, NewEnableDisable));
            }, policy::DontResetContainer{});
        });
    }

    auto
        FProcessor_Marker_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMarkerEntity,
            FFragment_Marker_Current& InCurrentComp,
            const FFragment_Marker_Params& InParamsComp) const
        -> void
    {
        if (InCurrentComp.Get_EnableDisable() == ECk_EnableDisable::Disable)
        { return; }

        InCurrentComp._EnableDisable = ECk_EnableDisable::Disable;

        // Since we are in the teardown, we are ok if the marker object is pending kill
        constexpr auto IncludePendingKill = true;
        const auto& Marker = InCurrentComp.Get_Marker().Get(IncludePendingKill);

        CK_ENSURE_IF_NOT(ck::IsValid(Marker, ck::IsValid_Policy_IncludePendingKill{}),
            TEXT("Expected Marker Actor Component of MarkerOwner [{}] to still exist during the Teardown process.\n"
                 "Because the entity destruction is done in multiple phases and the Teardown process is operating on a valid entity, it is expected for the Marker to still exist.\n"
                 "If we are the client, did the object get unexpectedly destroyed before we reached this point ?"),
            InCurrentComp.Get_MarkerOwner().Get_Handle())
        { return; }

        UCk_Utils_Physics_UE::Request_SetGenerateOverlapEvents(Marker, ECk_EnableDisable::Disable);
        UCk_Utils_Physics_UE::Request_SetCollisionEnabled(Marker, ECollisionEnabled::NoCollision);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Marker_UpdateTransform::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMarkerEntity,
            const FFragment_Marker_Current& InCurrentComp,
            const FFragment_Marker_Params&  InParamsComp) const
        -> void
    {
        // TODO: Extract this in a common function to avoid code duplication between similar system for Sensor
        const auto& MarkerOwner = InCurrentComp.Get_MarkerOwner();
        const auto& MarkerOwningActor = MarkerOwner.Get_Actor();

        CK_ENSURE_IF_NOT(ck::IsValid(MarkerOwningActor),
            TEXT("Cannot update Marker [{}] Transform because its Owning Actor is INVALID"),
            InMarkerEntity)
        { return; }

        const auto& Marker = InCurrentComp.Get_Marker().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(Marker), TEXT("Invalid Marker Actor Component stored for Marker Entity [{}]"), InMarkerEntity)
        { return; }

        const auto& BoneTransform = [&]() -> TOptional<FTransform>
        {
            const auto& Params           = InParamsComp.Get_Params();
            const auto& AttachmentParams = Params.Get_AttachmentParams();
            const auto& BoneName         = AttachmentParams.Get_BoneName();

            CK_ENSURE_IF_NOT(UCk_Utils_Actor_UE::Get_DoesBoneExistInSkeletalMesh(MarkerOwningActor.Get(), BoneName),
                TEXT("Marker Entity [{}] cannot update its Transform according to Bone [{}] because its Owning Actor [{}] does NOT have it in its Skeletal Mesh"),
                InMarkerEntity,
                BoneName,
                MarkerOwner)
            { return {}; }

            const auto& AttachedActorSkeletalMeshComponent = MarkerOwningActor->FindComponentByClass<USkeletalMeshComponent>();
            const auto& SocketBoneName                     = AttachedActorSkeletalMeshComponent->GetSocketBoneName(BoneName);
            const auto& BoneIndex                          = AttachedActorSkeletalMeshComponent->GetBoneIndex(SocketBoneName);
            const auto& AttachedActorTransform             = MarkerOwningActor->GetTransform();
            const auto& AttachmentPolicy                   = AttachmentParams.Get_AttachmentPolicy();
            auto SkeletonTransform                         = AttachedActorSkeletalMeshComponent->GetBoneTransform(BoneIndex);

            if (NOT EnumHasAnyFlags(AttachmentPolicy, ECk_Marker_AttachmentPolicy::UseBoneRotation))
            {
                SkeletonTransform.CopyRotation(AttachedActorTransform);
            }

            if (NOT EnumHasAnyFlags(AttachmentPolicy, ECk_Marker_AttachmentPolicy::UseBonePosition))
            {
                SkeletonTransform.CopyTranslation(AttachedActorTransform);
            }

            return SkeletonTransform;
        }();

        if (ck::Is_NOT_Valid(BoneTransform))
        { return; }

        Marker->SetWorldTransform(*BoneTransform);
        Marker->AddLocalTransform(InParamsComp.Get_Params().Get_RelativeTransform());
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_Marker_DebugPreviewAll::
        FProcessor_Marker_DebugPreviewAll(
        const FCk_Registry& InRegistry)
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

        _Registry.View<FFragment_Marker_Current>().ForEach(
        [&](FCk_Entity InMarkerEntity, const FFragment_Marker_Current& InMarkerCurrent)
        {
            if (ck::Is_NOT_Valid(InMarkerCurrent.Get_Marker()))
            { return; }

            const auto& OuterForDebugDraw = InMarkerCurrent.Get_MarkerOwner().Get_Actor().Get();

            const auto MarkerHandle = UCk_Utils_Marker_UE::CastChecked(FCk_Handle{InMarkerEntity, _Registry});
            UCk_Utils_Marker_UE::Preview(OuterForDebugDraw, MarkerHandle);
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------
