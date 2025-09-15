#pragma once
#include "CkAnimation/AnimNotify/CkAnimNotify.h"

#include "CkAnimNotify_EcsReady.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable)
class CKANIMATION_API UCk_AnimNotify_EcsReady_UE : public UCk_AnimNotify_UE
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(UCk_AnimNotify_EcsReady_UE);

public:
    UFUNCTION(BlueprintNativeEvent,
        Category = "Ck|Animation",
        DisplayName="[Ck] Received Notify (Override)")
	void
    Do_ReceivedNotify(
        AActor* InActorOwner,
        UPARAM(ref) FCk_Handle& InOwnerEntity,
        USkeletalMeshComponent* InMeshComp,
        UAnimSequenceBase* InAnimation,
        const FAnimNotifyEventReference& InEventReference) const;

public:
    virtual auto
    Notify(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) -> void override;

    virtual auto
    BranchingPointNotify(
        FBranchingPointNotifyPayload& BranchingPointPayload) -> void override;

    virtual auto
    CanReceiveNotify(
        AActor* InActorOwner,
        UPARAM(ref) FCk_Handle& InOwnerEntity,
        USkeletalMeshComponent* InMeshComp,
        UAnimSequenceBase* InAnimation) -> bool;

private:
    virtual auto
    Get_OwnerActorAndHandle(
        USkeletalMeshComponent* InMeshComp,
        UAnimSequenceBase* InAnimation) -> TPair<AActor*, FCk_Handle>;

private:
    // Controls whether this triggers on the play montage task and play montage proxy
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_Net_ReplicationType _ReplicationType = ECk_Net_ReplicationType::LocalOnly;
};

// --------------------------------------------------------------------------------------------------------------------
