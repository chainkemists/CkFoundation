#include "CkAnimNotifyState.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AnimNotifyState_UE::
    Get_NameForPlayMontageNotify_Implementation() const
    -> FName
{
    return FName(GetNotifyName());
}

auto
    UCk_AnimNotifyState_UE::
    BranchingPointNotifyBegin(
        FBranchingPointNotifyPayload& BranchingPointPayload)
    -> void
{
    Super::BranchingPointNotifyBegin(BranchingPointPayload);

    if (_PlayMontagePolicy == ECk_AnimNotify_PlayMontagePolicy::Ignored)
    { return; }

    USkeletalMeshComponent* MeshComp = BranchingPointPayload.SkelMeshComponent;
    if (ck::Is_NOT_Valid(MeshComp))
    { return; }

    UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
    if (ck::Is_NOT_Valid(AnimInstance))
    { return; }

    AnimInstance->OnPlayMontageNotifyBegin.Broadcast(Get_NameForPlayMontageNotify(), BranchingPointPayload);
}

auto
    UCk_AnimNotifyState_UE::
    BranchingPointNotifyEnd(
        FBranchingPointNotifyPayload& BranchingPointPayload)
    -> void
{
    Super::BranchingPointNotifyEnd(BranchingPointPayload);

    if (_PlayMontagePolicy == ECk_AnimNotify_PlayMontagePolicy::Ignored)
    { return; }

    USkeletalMeshComponent* MeshComp = BranchingPointPayload.SkelMeshComponent;
    if (ck::Is_NOT_Valid(MeshComp))
    { return; }

    UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
    if (ck::Is_NOT_Valid(AnimInstance))
    { return; }

    AnimInstance->OnPlayMontageNotifyEnd.Broadcast(Get_NameForPlayMontageNotify(), BranchingPointPayload);
}

// --------------------------------------------------------------------------------------------------------------------