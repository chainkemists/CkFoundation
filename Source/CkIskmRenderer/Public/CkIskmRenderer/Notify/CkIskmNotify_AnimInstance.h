#pragma once

#include "Animation/AnimInstance.h"

#include "CkIskmRenderer/Proxy/CkIskmProxy_Fragment_Data.h"

#include "CkIskmNotify_AnimInstance.generated.h"

UCLASS(Blueprintable, BlueprintType)
class CKISKMRENDERER_API UCk_IskmNotify_AnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|IskmNotify")
    void
    Set_OwningProxyHandle(FCk_Handle_IskmProxy InHandle);

protected:
    virtual void
    NativeInitializeAnimation() override;

    virtual bool
    HandleNotify(const FAnimNotifyEvent& AnimNotifyEvent) override;

    virtual void
    NativeOnMontageBlendingOut(class UAnimMontage* Montage, bool bInterrupted) override;

private:
    UPROPERTY()
    FCk_Handle_IskmProxy _OwningHandle;

    UFUNCTION()
    void OnMontageEndedHook(UAnimMontage* InMontage, bool bInterrupted);
};
