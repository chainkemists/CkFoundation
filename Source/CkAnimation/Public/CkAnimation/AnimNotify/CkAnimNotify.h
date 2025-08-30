#pragma once

#include "CkAnimation/CkAnimation_Common.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Ensure/CkEnsure.h"

#include <Animation/AnimNotifies/AnimNotify.h>

#include "CkAnimNotify.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable)
class CKANIMATION_API UCk_AnimNotify_UE : public UAnimNotify
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(UCk_AnimNotify_UE);

public:
    UCk_AnimNotify_UE(
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
    BranchingPointNotify(
        FBranchingPointNotifyPayload& BranchingPointPayload) -> void override;

private:
    // Controls whether this triggers on the play montage task and play montage proxy
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_AnimNotify_PlayMontagePolicy _PlayMontagePolicy = ECk_AnimNotify_PlayMontagePolicy::Ignored;
};

// --------------------------------------------------------------------------------------------------------------------
