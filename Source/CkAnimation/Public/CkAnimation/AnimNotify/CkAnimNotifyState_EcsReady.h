#pragma once
#include "CkAnimation/AnimNotify/CkAnimNotifyState.h"

#include "CkAnimNotifyState_EcsReady.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable)
class CKANIMATION_API UCk_AnimNotifyState_EcsReady_UE : public UCk_AnimNotifyState_UE
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(UCk_AnimNotifyState_EcsReady_UE);

public:
    UFUNCTION(BlueprintNativeEvent,
        Category = "Ck|Animation",
        DisplayName="[Ck] Received Notify Begin (Override)")
	bool
    Do_ReceivedNotifyBegin(
        AActor* InActorOwner,
        UPARAM(ref) FCk_Handle& InOwnerEntity,
        USkeletalMeshComponent* InMeshComp,
        UAnimSequenceBase* InAnimation,
        const FAnimNotifyEventReference& InEventReference,
        FCk_Time InTotalDuration) const;

    UFUNCTION(BlueprintNativeEvent,
        Category = "Ck|Animation",
        DisplayName="[Ck] Received Notify Tick (Override)")
	bool
    Do_ReceivedNotifyTick(
        AActor* InActorOwner,
        UPARAM(ref) FCk_Handle& InOwnerEntity,
        USkeletalMeshComponent* InMeshComp,
        UAnimSequenceBase* InAnimation,
        const FAnimNotifyEventReference& InEventReference,
        FCk_Time InFrameDeltaT) const;

    UFUNCTION(BlueprintNativeEvent,
        Category = "Ck|Animation",
        DisplayName="[Ck] Received Notify End (Override)")
	bool
    Do_ReceivedNotifyEnd(
        AActor* InActorOwner,
        UPARAM(ref) FCk_Handle& InOwnerEntity,
        USkeletalMeshComponent* InMeshComp,
        UAnimSequenceBase* InAnimation,
        const FAnimNotifyEventReference& InEventReference) const;


public:
    virtual auto
    NotifyBegin(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        float TotalDuration,
        const FAnimNotifyEventReference& EventReference) -> void override;

    virtual auto
    NotifyTick(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        float FrameDeltaTime,
        const FAnimNotifyEventReference& EventReference) -> void override;

    virtual auto
    NotifyEnd(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) -> void override;

    virtual auto
    BranchingPointNotifyBegin(
        FBranchingPointNotifyPayload& BranchingPointPayload) -> void override;

    virtual auto
    BranchingPointNotifyEnd(
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
