#include "CkAnimNotifyState_EcsReady.h"

#include "CkCore/Game/CkGame_Utils.h"

auto
    UCk_AnimNotifyState_EcsReady_UE::
    Do_ReceivedNotifyBegin_Implementation(
        AActor* InActorOwner,
        FCk_Handle& InOwnerEntity,
        USkeletalMeshComponent* InMeshComp,
        UAnimSequenceBase* InAnimation,
        const FAnimNotifyEventReference& InEventReference,
        FCk_Time InTotalDuration) const
    -> bool
{
    return false;
}

auto
    UCk_AnimNotifyState_EcsReady_UE::
    Do_ReceivedNotifyTick_Implementation(
        AActor* InActorOwner,
        FCk_Handle& InOwnerEntity,
        USkeletalMeshComponent* InMeshComp,
        UAnimSequenceBase* InAnimation,
        const FAnimNotifyEventReference& InEventReference,
        FCk_Time InFrameDeltaT) const
    -> bool

{
    return false;
}

auto
    UCk_AnimNotifyState_EcsReady_UE::
    Do_ReceivedNotifyEnd_Implementation(
        AActor* InActorOwner,
        FCk_Handle& InOwnerEntity,
        USkeletalMeshComponent* InMeshComp,
        UAnimSequenceBase* InAnimation,
        const FAnimNotifyEventReference& InEventReference) const
    -> bool

{
    return false;
}

auto
    UCk_AnimNotifyState_EcsReady_UE::
    NotifyBegin(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        float TotalDuration,
        const FAnimNotifyEventReference& EventReference)
    -> void
{
    if (NOT UCk_Utils_Game_UE::Get_IsInGame(MeshComp))
    { return; }

    auto [OwningActor, OwningHandle] = Get_OwnerActorAndHandle(MeshComp, Animation);

    if (CanReceiveNotify(OwningActor, OwningHandle, MeshComp, Animation))
    {
        Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
        Do_ReceivedNotifyBegin(OwningActor, OwningHandle, MeshComp, Animation, EventReference, UCk_Utils_Time_UE::Make_FromSeconds(TotalDuration));
    }
}

auto
    UCk_AnimNotifyState_EcsReady_UE::
    NotifyTick(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        float FrameDeltaTime,
        const FAnimNotifyEventReference& EventReference)
    -> void
{
    if (NOT UCk_Utils_Game_UE::Get_IsInGame(MeshComp))
    { return; }

    auto [OwningActor, OwningHandle] = Get_OwnerActorAndHandle(MeshComp, Animation);

    if (CanReceiveNotify(OwningActor, OwningHandle, MeshComp, Animation))
    {
        Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
        Do_ReceivedNotifyTick(OwningActor, OwningHandle, MeshComp, Animation, EventReference, UCk_Utils_Time_UE::Make_FromSeconds(FrameDeltaTime));
    }
}

auto
    UCk_AnimNotifyState_EcsReady_UE::
    NotifyEnd(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference)
    -> void
{
    if (NOT UCk_Utils_Game_UE::Get_IsInGame(MeshComp))
    { return; }

    auto [OwningActor, OwningHandle] = Get_OwnerActorAndHandle(MeshComp, Animation);

    if (CanReceiveNotify(OwningActor, OwningHandle, MeshComp, Animation))
    {
        Super::NotifyEnd(MeshComp, Animation, EventReference);
        Do_ReceivedNotifyEnd(OwningActor, OwningHandle, MeshComp, Animation, EventReference);
    }
}

auto
    UCk_AnimNotifyState_EcsReady_UE::
    BranchingPointNotifyBegin(
        FBranchingPointNotifyPayload& BranchingPointPayload)
    -> void
{
    if (NOT UCk_Utils_Game_UE::Get_IsInGame(BranchingPointPayload.SkelMeshComponent))
    { return; }

    auto [OwningActor, OwningHandle] = Get_OwnerActorAndHandle(BranchingPointPayload.SkelMeshComponent, BranchingPointPayload.SequenceAsset);

    if (CanReceiveNotify(OwningActor, OwningHandle, BranchingPointPayload.SkelMeshComponent, BranchingPointPayload.SequenceAsset))
    {
        Super::BranchingPointNotifyBegin(BranchingPointPayload);
    }
}

auto
    UCk_AnimNotifyState_EcsReady_UE::
    BranchingPointNotifyEnd(
        FBranchingPointNotifyPayload& BranchingPointPayload)
    -> void
{
    if (NOT UCk_Utils_Game_UE::Get_IsInGame(BranchingPointPayload.SkelMeshComponent))
    { return; }

    auto [OwningActor, OwningHandle] = Get_OwnerActorAndHandle(BranchingPointPayload.SkelMeshComponent, BranchingPointPayload.SequenceAsset);

    if (CanReceiveNotify(OwningActor, OwningHandle, BranchingPointPayload.SkelMeshComponent, BranchingPointPayload.SequenceAsset))
    {
        Super::BranchingPointNotifyEnd(BranchingPointPayload);
    }
}

auto
    UCk_AnimNotifyState_EcsReady_UE::
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
    UCk_AnimNotifyState_EcsReady_UE::
    Get_OwnerActorAndHandle(
        USkeletalMeshComponent* InMeshComp,
        UAnimSequenceBase* InAnimation)
    -> TPair<AActor*, FCk_Handle>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InMeshComp),
        TEXT("Notify State [{}] of Animation [{}] has NO valid mesh component!"), this, InAnimation)
    { return {}; }

    auto OwningActor = InMeshComp->GetOwner();

    CK_ENSURE_IF_NOT(ck::IsValid(OwningActor),
        TEXT("Notify State [{}] of Animation [{}] has NO valid owning actor!"), this, InAnimation)
    { return {}; }

    CK_ENSURE_IF_NOT(UCk_Utils_OwningActor_UE::Get_IsActorEcsReady(OwningActor),
        TEXT("Notify State [{}] of Animation [{}] has an owner [{}] that is NOT Ecs Ready!"), this, InAnimation, OwningActor)
    { return {}; }

    auto OwningHandle = UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(OwningActor);

    return {OwningActor, OwningHandle};
}
