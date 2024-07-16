#include "CkTargetable_Processor.h"

#include "CkActor/ActorModifier/CkActorModifier_Fragment_Data.h"
#include "CkActor/ActorModifier/CkActorModifier_Utils.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "Targetable/CkTargetable_Utils.h"

#include "CkTargeting/CkTargeting_Log.h"

#include "CkNet/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Targetable_Setup::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Targetable_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_Targetable_Params& InParams,
            FFragment_Targetable_Current& InCurrent) const
        -> void
    {
        const auto& LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

        CK_ENSURE_IF_NOT(UCk_Utils_OwningActor_UE::Has(LifetimeOwner),
            TEXT("Targetable [{}] was added to Entity [{}] which does NOT have an Owning Actor"),
            InHandle,
            LifetimeOwner)
        { return; }

        const auto& LifetimeOwnerActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(LifetimeOwner);

        CK_ENSURE_IF_NOT(ck::IsValid(LifetimeOwnerActor),
            TEXT("Targetable [{}] was added to Entity [{}] which has an INVALID Owning Actor"),
            InHandle,
            LifetimeOwner)
        { return; }

        const auto& AttachmentParams = InParams.Get_Params().Get_AttachmentParams();
        const auto& LocalOffset      = AttachmentParams.Get_LocalOffset();
        const auto& BoneName         = AttachmentParams.Get_BoneName();

        const auto& ParentSceneComp = [&]() -> USceneComponent*
        {
            if (ck::Is_NOT_Valid(BoneName))
            { return LifetimeOwnerActor->GetRootComponent(); }

            auto* SkeletalMeshComponent = LifetimeOwnerActor->FindComponentByClass<USkeletalMeshComponent>();

            CK_ENSURE_IF_NOT(ck::IsValid(SkeletalMeshComponent),
                TEXT("Targetable [{}] was added to Entity [{}] that does NOT have a SkeletalMesh Component"),
                InHandle,
                LifetimeOwner)
            { return LifetimeOwnerActor->GetRootComponent(); }

            CK_ENSURE_IF_NOT(UCk_Utils_Actor_UE::Get_DoesBoneExistInSkeletalMesh(LifetimeOwnerActor, BoneName),
                TEXT("Targetable [{}] was added to Entity [{}] that does NOT have the Bone [{}] in its SkeletalMesh"),
                InHandle,
                LifetimeOwner,
                BoneName)
            { return LifetimeOwnerActor->GetRootComponent(); }

            return SkeletalMeshComponent;
        }();

        CK_ENSURE_IF_NOT(ck::IsValid(ParentSceneComp),
            TEXT("Invalid ParentScene Component. Cannot attach Targetable [{}] on Entity [{}]"),
            InHandle,
            LifetimeOwner)
        { return; }

        const auto& Targetable_ActorCompParams = FCk_AddActorComponent_Params{ParentSceneComp}
                                                        .Set_AttachmentSocket(BoneName)
                                                        .Set_AttachmentType(ECk_ActorComponent_AttachmentPolicy::Attach);

        UCk_Utils_ActorModifier_UE::Request_AddActorComponent
        (
            LifetimeOwner,
            FCk_Request_ActorModifier_AddActorComponent{USceneComponent::StaticClass()}
                .Set_ComponentParams(Targetable_ActorCompParams)
                .Set_IsUnique(false)
                .Set_InitializerFunc(
                [=](UActorComponent* InActorComp) mutable
                {
                    auto* SceneComp = Cast<USceneComponent>(InActorComp);
                    SceneComp->AddLocalOffset(LocalOffset);

                    InHandle.Get<FFragment_Targetable_Current>()._AttachmentNode = SceneComp;
                    UCk_Utils_Targetable_UE::Set_TargetableIsReady(InHandle);
                }),
            {},
            {}
        );
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Targetable_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            FFragment_Targetable_Current& InCurrent,
            const FFragment_Targetable_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](const FFragment_Targetable_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequestVariant) -> void
            {
                DoHandleRequest(InHandle, InCurrent, InRequestVariant);
            }), policy::DontResetContainer{});
        });
    }

    auto
        FProcessor_Targetable_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            FFragment_Targetable_Current& InCurrent,
            const FFragment_Targetable_Requests::EnableDisableRequestType& InRequest)
        -> void
    {
        const auto& CurrentEnableDisable = InCurrent.Get_EnableDisable();
        const auto& NewEnableDisable = InRequest.Get_EnableDisable();

        if (CurrentEnableDisable == NewEnableDisable)
        { return; }

        InCurrent._EnableDisable = NewEnableDisable;

        const auto TargetableBasicInfo = FCk_Targetable_BasicInfo{InHandle, UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle)};

        UUtils_Signal_OnTargetableEnableDisable::Broadcast(InHandle, MakePayload(TargetableBasicInfo, NewEnableDisable));
    }
}

// --------------------------------------------------------------------------------------------------------------------
