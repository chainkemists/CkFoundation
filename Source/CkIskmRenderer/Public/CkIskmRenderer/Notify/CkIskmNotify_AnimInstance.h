#pragma once

#include "Animation/AnimInstance.h"

#include "CkIskmRenderer/Proxy/CkIskmProxy_Fragment_Data.h"

#include "CkIskmNotify_AnimInstance.generated.h"

UCLASS(Blueprintable, BlueprintType)
class CKISKMRENDERER_API UCk_IskmNotify_AnimInstance : public UAnimInstance
{
    GENERATED_BODY()

    // Reads _OwningHandle to expose the owning proxy/entity to anim code.
    friend class UCk_Utils_IskmNotify_UE;

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|IskmNotify")
    void
    Set_OwningProxyHandle(FCk_Handle_IskmProxy InHandle);

protected:
    virtual void
    NativeInitializeAnimation() override;

    virtual bool
    HandleNotify(const FAnimNotifyEvent& AnimNotifyEvent) override;

    // NOT a virtual override — UAnimInstance exposes montage-end via the OnMontageBlendingOut delegate.
    // This is a plain helper that the bound OnMontageEndedHook forwards to.
    void
    NativeOnMontageBlendingOut(class UAnimMontage* InMontage, bool InInterrupted);

private:
    UPROPERTY()
    FCk_Handle_IskmProxy _OwningHandle;

    UFUNCTION()
    void OnMontageEndedHook(UAnimMontage* InMontage, bool InInterrupted);
};
