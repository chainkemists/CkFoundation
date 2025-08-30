#include "CkAnimNotify.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AnimNotify_UE::
    Get_NameForPlayMontageNotify_Implementation() const
    -> FName
{
    return FName(GetNotifyName());
}

auto
    UCk_AnimNotify_UE::
    BranchingPointNotify(
        FBranchingPointNotifyPayload& BranchingPointPayload)
    -> void
{
    Super::BranchingPointNotify(BranchingPointPayload);

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

// --------------------------------------------------------------------------------------------------------------------