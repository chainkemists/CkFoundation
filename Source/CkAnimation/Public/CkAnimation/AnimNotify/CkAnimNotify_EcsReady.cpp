#include "CkAnimNotify_EcsReady.h"

#include "CkCore/Game/CkGame_Utils.h"

auto
    UCk_AnimNotify_EcsReady_UE::
    Do_ReceivedNotify_Implementation(
        AActor* InActorOwner,
        FCk_Handle& InOwnerEntity,
        USkeletalMeshComponent* InMeshComp,
        UAnimSequenceBase* InAnimation,
        const FAnimNotifyEventReference& InEventReference) const
    -> void
{
}

auto
    UCk_AnimNotify_EcsReady_UE::
    Notify(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference)
    -> void
{
    if (NOT UCk_Utils_Game_UE::Get_IsInGame(this))
    { return; }

    auto [OwningActor, OwningHandle] = Get_OwnerActorAndHandle(MeshComp, Animation);

    if (CanReceiveNotify(OwningActor, OwningHandle, MeshComp, Animation))
    {
        Super::Notify(MeshComp, Animation, EventReference);
        Do_ReceivedNotify(OwningActor, OwningHandle, MeshComp, Animation, EventReference);
    }
}

auto
    UCk_AnimNotify_EcsReady_UE::
    BranchingPointNotify(
        FBranchingPointNotifyPayload& BranchingPointPayload)
    -> void
{
    if (NOT UCk_Utils_Game_UE::Get_IsInGame(this))
    { return; }

    auto [OwningActor, OwningHandle] = Get_OwnerActorAndHandle(BranchingPointPayload.SkelMeshComponent, BranchingPointPayload.SequenceAsset);

    if (CanReceiveNotify(OwningActor, OwningHandle, BranchingPointPayload.SkelMeshComponent, BranchingPointPayload.SequenceAsset))
    {
        Super::BranchingPointNotify(BranchingPointPayload);
    }
}

auto
    UCk_AnimNotify_EcsReady_UE::
    CanReceiveNotify(
        AActor* InActorOwner,
        FCk_Handle& InOwnerEntity,
        USkeletalMeshComponent* InMeshComp,
        UAnimSequenceBase* InAnimation)
    -> bool
{
    if (NOT UCk_Utils_Net_UE::Get_IsRoleMatching(InActorOwner, _ReplicationType))
    { return false; }

    return true;
}

auto
    UCk_AnimNotify_EcsReady_UE::
    Get_OwnerActorAndHandle(
        USkeletalMeshComponent* InMeshComp,
        UAnimSequenceBase* InAnimation)
    -> TPair<AActor*, FCk_Handle>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InMeshComp),
        TEXT("Notify [{}] of Animation [{}] has NO valid mesh component!"), this, InAnimation)
    { return {}; }

    auto OwningActor = InMeshComp->GetOwner();

    CK_ENSURE_IF_NOT(ck::IsValid(OwningActor),
        TEXT("Notify [{}] of Animation [{}] has NO valid owning actor!"), this, InAnimation)
    { return {}; }

    CK_ENSURE_IF_NOT(UCk_Utils_OwningActor_UE::Get_IsActorEcsReady(OwningActor),
        TEXT("Notify [{}] of Animation [{}] has an owner [{}] that is NOT Ecs Ready!"), this, InAnimation, OwningActor)
    { return {}; }

    auto OwningHandle = UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(OwningActor);

    return {OwningActor, OwningHandle};
}
