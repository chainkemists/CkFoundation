#include "CkIskmRenderer/Notify/CkIskmNotify_AnimInstance.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifies/AnimNotify.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkIskmRenderer/Proxy/CkIskmProxy_Fragment.h"

void UCk_IskmNotify_AnimInstance::Set_OwningProxyHandle(FCk_Handle_IskmProxy InHandle)
{
    _OwningHandle = InHandle;
}

void UCk_IskmNotify_AnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    // AddUnique, not AddDynamic: a SetSkeletalMesh(..., bReinitPose=true) re-runs
    // NativeInitializeAnimation on the *reused* instance, so a plain AddDynamic would
    // bind (this, OnMontageEndedHook) a second time and trip the AddInternal ensure.
    OnMontageEnded.AddUniqueDynamic(this, &UCk_IskmNotify_AnimInstance::OnMontageEndedHook);
}

bool UCk_IskmNotify_AnimInstance::HandleNotify(const FAnimNotifyEvent& AnimNotifyEvent)
{
    const auto Result = Super::HandleNotify(AnimNotifyEvent);
    if (ck::IsValid(_OwningHandle))
    {
        ck::UUtils_Signal_IskmProxy_OnAnimationNotify::Broadcast(
            _OwningHandle, ck::MakePayload(_OwningHandle, AnimNotifyEvent.NotifyName));
    }
    return Result;
}

void UCk_IskmNotify_AnimInstance::NativeOnMontageBlendingOut(UAnimMontage* InMontage, bool InInterrupted)
{
    if (ck::IsValid(_OwningHandle))
    {
        // Clear ECS-side state for the entity. The OnMontageFinished signal is the source
        // of truth for listeners; clearing _CurrentMontage and the active-montage tag here
        // keeps fragment state consistent with the SKMC-side state. Friend access to
        // FFragment_IskmProxy_AnimState was added in pre-M setup commit `411ac4e29`.
        if (auto& AnimState = _OwningHandle.Get<ck::FFragment_IskmProxy_AnimState>();
            AnimState.Get_CurrentMontage().Get() == InMontage)
        {
            AnimState._CurrentMontage.Reset();
        }
        _OwningHandle.Try_Remove<ck::FTag_IskmProxy_HasActiveMontage>();

        ck::UUtils_Signal_IskmProxy_OnMontageFinished::Broadcast(
            _OwningHandle,
            ck::MakePayload(_OwningHandle, FCk_IskmProxy_MontageRef{InMontage}, InInterrupted));
    }
}

void UCk_IskmNotify_AnimInstance::OnMontageEndedHook(UAnimMontage* InMontage, bool InInterrupted)
{
    NativeOnMontageBlendingOut(InMontage, InInterrupted);
}
