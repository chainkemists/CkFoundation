#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkIskmRenderer/Proxy/CkIskmProxy_Fragment_Data.h"

#include "CkIskmProxy_Utils.generated.h"

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_IskmProxy"))
class CKISKMRENDERER_API UCk_Utils_IskmProxy_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IskmProxy_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_IskmProxy);

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Add")
    static FCk_Handle_IskmProxy
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_IskmProxy_ParamsData& InParams);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Has")
    static bool
    Has(const FCk_Handle& InHandle);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Bind To OnAnimationNotify")
    static FCk_Handle_IskmProxy
    BindTo_OnAnimationNotify(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        const FCk_Delegate_IskmProxy_OnAnimationNotify& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Unbind From OnAnimationNotify")
    static FCk_Handle_IskmProxy
    UnbindFrom_OnAnimationNotify(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        const FCk_Delegate_IskmProxy_OnAnimationNotify& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Bind To OnAnimationFinished")
    static FCk_Handle_IskmProxy
    BindTo_OnAnimationFinished(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        const FCk_Delegate_IskmProxy_OnAnimationFinished& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Unbind From OnAnimationFinished")
    static FCk_Handle_IskmProxy
    UnbindFrom_OnAnimationFinished(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        const FCk_Delegate_IskmProxy_OnAnimationFinished& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Play Animation")
    static FCk_Handle_IskmProxy
    Request_PlayAnimation(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        const FCk_Request_IskmProxy_PlayAnimation& InRequest);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Stop Animation")
    static FCk_Handle_IskmProxy
    Request_StopAnimation(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        const FCk_Request_IskmProxy_StopAnimation& InRequest);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Set Play Rate")
    static FCk_Handle_IskmProxy
    Request_SetPlayRate(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        float InRate);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Get Playing Animation")
    static UAnimSequenceBase*
    Get_PlayingAnimation(const FCk_Handle_IskmProxy& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Get Play Time")
    static float
    Get_PlayTime(const FCk_Handle_IskmProxy& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Get Play Length")
    static float
    Get_PlayLength(const FCk_Handle_IskmProxy& InHandle);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Set Custom Data Float")
    static FCk_Handle_IskmProxy
    Request_SetCustomDataFloat(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        int32 InOffset,
        float InValue);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Get Custom Data Float")
    static float
    Get_CustomDataFloat(
        const FCk_Handle_IskmProxy& InHandle,
        int32 InOffset);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Attach Submesh")
    static FCk_Handle_IskmProxy
    Request_AttachSubmesh(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        FName InSubmeshName);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Detach Submesh")
    static FCk_Handle_IskmProxy
    Request_DetachSubmesh(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        FName InSubmeshName);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Detach All Submeshes")
    static FCk_Handle_IskmProxy
    Request_DetachAllSubmeshes(UPARAM(ref) FCk_Handle_IskmProxy& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Get Num Attached Submeshes")
    static int32
    Get_NumAttachedSubmeshes(const FCk_Handle_IskmProxy& InHandle);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Set AnimInstance Class")
    static FCk_Handle_IskmProxy
    Request_SetAnimInstanceClass(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        TSubclassOf<UAnimInstance> InClass);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Get Pose Source")
    static ECk_IskmProxy_PoseSource
    Get_PoseSource(const FCk_Handle_IskmProxy& InHandle);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Play Montage")
    static FCk_Handle_IskmProxy
    Request_PlayMontage(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        const FCk_Request_IskmProxy_PlayMontage& InRequest);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Stop Montage")
    static FCk_Handle_IskmProxy
    Request_StopMontage(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        const FCk_Request_IskmProxy_StopMontage& InRequest);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Bind To OnMontageFinished")
    static FCk_Handle_IskmProxy
    BindTo_OnMontageFinished(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        const FCk_Delegate_IskmProxy_OnMontageFinished& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Unbind From OnMontageFinished")
    static FCk_Handle_IskmProxy
    UnbindFrom_OnMontageFinished(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        const FCk_Delegate_IskmProxy_OnMontageFinished& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Begin Ragdoll")
    static FCk_Handle_IskmProxy
    Request_BeginRagdoll(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        const FCk_Request_IskmProxy_BeginRagdoll& InRequest);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request End Ragdoll")
    static FCk_Handle_IskmProxy
    Request_EndRagdoll(UPARAM(ref) FCk_Handle_IskmProxy& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Get Socket Transform")
    static FTransform
    Get_SocketTransform(
        const FCk_Handle_IskmProxy& InHandle,
        FName InSocketName,
        ECk_IskmProxy_TransformSpace InSpace);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Line Trace Instance")
    static bool
    LineTrace_Instance(
        const FCk_Handle_IskmProxy& InHandle,
        const FCk_IskmProxy_LineTraceParams& InParams,
        FCk_IskmProxy_LineTraceResult& OutResult);
};
