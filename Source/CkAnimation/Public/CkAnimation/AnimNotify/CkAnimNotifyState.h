#pragma once

#include "CkAnimation/CkAnimation_Common.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Ensure/CkEnsure.h"

#include <Animation/AnimNotifies/AnimNotifyState.h>

#include "CkAnimNotifyState.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable)
class CKANIMATION_API UCk_AnimNotifyState_UE : public UAnimNotifyState
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(UCk_AnimNotifyState_UE);

public:
    UCk_AnimNotifyState_UE(
        const FObjectInitializer& ObjectInitializer)
	    : Super(ObjectInitializer)
    {
	    bIsNativeBranchingPoint = true;
    }

public:
    UFUNCTION(BlueprintNativeEvent,
        Category = "Ck|Animation",
        DisplayName="[Ck] Get Name For Play Montage Notify (Override Optional)")
	FName Get_NameForPlayMontageNotify() const;

public:
    virtual auto
    BranchingPointNotifyBegin(
        FBranchingPointNotifyPayload& BranchingPointPayload) -> void override;

    virtual auto
    BranchingPointNotifyEnd(
        FBranchingPointNotifyPayload& BranchingPointPayload) -> void override;

private:
    // Controls whether this triggers on the play montage task and play montage proxy
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_AnimNotify_PlayMontagePolicy _PlayMontagePolicy = ECk_AnimNotify_PlayMontagePolicy::Ignored;
};

// --------------------------------------------------------------------------------------------------------------------
