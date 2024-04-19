#include "CkMarker_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Utils.h"

#include "CkOverlapBody/CkOverlapBody_Log.h"
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
        const auto& MarkerLifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InMarkerEntity);
        const auto& MarkerAttachedEntityAndActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails(MarkerLifetimeOwner);
        const auto& MarkerAttachedEntity = MarkerAttachedEntityAndActor.Get_Handle();

        constexpr auto IncludePendingKill = true;
        CK_ENSURE_IF_NOT(ck::IsValid(MarkerAttachedEntityAndActor.Get_Actor().Get(IncludePendingKill), ck::IsValid_Policy_IncludePendingKill{}),
            TEXT("Marker Entity [{}] is Attached to Entity [{}] that does NOT have a valid Actor"),
            InMarkerEntity,
            MarkerAttachedEntity)
        { return; }

        if (ck::Is_NOT_Valid(MarkerAttachedEntityAndActor.Get_Actor()))
        { return; }

        InCurrentComp._AttachedEntityAndActor = MarkerAttachedEntityAndActor;

        const auto& Params        = InParamsComp.Get_Params();
        const auto& ShapeParams   = Params.Get_ShapeParams();
        const auto& PhysicsParams = Params.Get_PhysicsParams();

        CK_ENSURE_IF_NOT
        (
            PhysicsParams.Get_CollisionProfileName().Name != TEXT("NoCollision"),
            TEXT("Marker [{}] added to Entity [{}] has Collision Profile set to [{}]. Marker will NOT work properly!"),
            Params.Get_MarkerName(),
            MarkerLifetimeOwner,
            PhysicsParams.Get_CollisionProfileName()
        ) {};

        switch (const auto& ShapeType = ShapeParams.Get_ShapeDimensions().Get_ShapeType())
        {
            case ECk_ShapeType::Box:
            {
                UCk_Utils_MarkerAndSensor_UE::Add_MarkerOrSensor_ActorComponent<UCk_Marker_ActorComponent_Box_UE, ECk_ShapeType::Box>(InMarkerEntity, MarkerAttachedEntityAndActor, Params);
                break;
            }
            case ECk_ShapeType::Sphere:
            {
                UCk_Utils_MarkerAndSensor_UE::Add_MarkerOrSensor_ActorComponent<UCk_Marker_ActorComponent_Sphere_UE, ECk_ShapeType::Sphere>(InMarkerEntity, MarkerAttachedEntityAndActor, Params);
                break;
            }
            case ECk_ShapeType::Capsule:
            {
                UCk_Utils_MarkerAndSensor_UE::Add_MarkerOrSensor_ActorComponent<UCk_Marker_ActorComponent_Capsule_UE, ECk_ShapeType::Capsule>(InMarkerEntity, MarkerAttachedEntityAndActor, Params);
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

        const auto& AttachedActorSkeletalMeshComponent = MarkerAttachedEntityAndActor.Get_Actor()->FindComponentByClass<USkeletalMeshComponent>();

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

                const auto& Params     = InParamsComp.Get_Params();
                const auto& MarkerName = Params.Get_MarkerName();
                const auto& Marker     = InCurrentComp.Get_Marker().Get();

                UCk_Utils_Physics_UE::Request_SetGenerateOverlapEvents(Marker, NewEnableDisable);
                UCk_Utils_Physics_UE::Request_SetCollisionEnabled(Marker, CollisionEnabled);

                UUtils_Signal_OnMarkerEnableDisable::Broadcast(InMarkerEntity, MakePayload(InCurrentComp.Get_AttachedEntityAndActor().Get_Handle(), MarkerName, NewEnableDisable));
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

        if (ck::Is_NOT_Valid(Marker, ck::IsValid_Policy_IncludePendingKill{}))
        {
            ck::overlap_body::Verbose(TEXT("Expected Marker Actor Component of Entity [{}] to still exist during the Teardown process. "
                "However, it's possible that that the Actor and it's components were pulled from under us on Client machines due to the way "
                "destruction is handled in Unreal."), InCurrentComp.Get_AttachedEntityAndActor().Get_Handle());
            return;
        }

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
        const auto& MarkerAttachedEntityAndActor = InCurrentComp.Get_AttachedEntityAndActor();
        const auto& MarkerAttachedActor          = MarkerAttachedEntityAndActor.Get_Actor();

        CK_ENSURE_IF_NOT(ck::IsValid(MarkerAttachedActor),
            TEXT("Cannot update Marker [{}] Transform because its Attached Actor is INVALID"),
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

            CK_ENSURE_IF_NOT(UCk_Utils_Actor_UE::Get_DoesBoneExistInSkeletalMesh(MarkerAttachedActor.Get(), BoneName),
                TEXT("Marker Entity [{}] cannot update its Transform according to Bone [{}] because its Attached Actor [{}] does NOT have it in its Skeletal Mesh"),
                InMarkerEntity,
                BoneName,
                MarkerAttachedEntityAndActor)
            { return {}; }

            const auto& AttachedActorSkeletalMeshComponent = MarkerAttachedActor->FindComponentByClass<USkeletalMeshComponent>();
            const auto& SocketBoneName                     = AttachedActorSkeletalMeshComponent->GetSocketBoneName(BoneName);
            const auto& BoneIndex                          = AttachedActorSkeletalMeshComponent->GetBoneIndex(SocketBoneName);
            const auto& AttachedActorTransform             = MarkerAttachedActor->GetTransform();
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

            const auto& OuterForDebugDraw = InMarkerCurrent.Get_AttachedEntityAndActor().Get_Actor().Get();

            const auto MarkerHandle = UCk_Utils_Marker_UE::CastChecked(FCk_Handle{InMarkerEntity, _Registry});
            UCk_Utils_Marker_UE::Preview(OuterForDebugDraw, MarkerHandle);
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------
