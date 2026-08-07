#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkIskmRenderer/Proxy/CkIskmProxy_Fragment_Data.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkIskmProxy_Utils.generated.h"

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_IskmProxy"))
class CKISKMRENDERER_API UCk_Utils_IskmProxy_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IskmProxy_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_IskmProxy);

public:
    // ---- Add / Has ----

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Add Feature")
    static FCk_Handle_IskmProxy
    Add(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_Fragment_IskmProxy_ParamsData& InParams);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Create")
    static FCk_Handle_IskmProxy
    Create(
        UPARAM(ref) FCk_Handle& InOwner,
        const FTransform& InInitialTransform,
        const FCk_Fragment_IskmProxy_ParamsData& InParams);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Has Feature")
    static bool
    Has(const FCk_Handle& InHandle);

    // ---- Getters ----

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

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Get Custom Data Float")
    static float
    Get_CustomDataFloat(
        const FCk_Handle_IskmProxy& InHandle,
        int32 InOffset);

    // Returns the override recorded for the slot, or nullptr if the slot is
    // not overridden.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Get Material Override")
    static UMaterialInterface*
    Get_MaterialOverride(
        const FCk_Handle_IskmProxy& InHandle,
        int32 InSlotIndex);

    // Effective material on the base SKMC for the slot (the override if one is
    // applied, else the mesh default). Returns nullptr before Setup completes
    // or for an out-of-range slot.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Get Material")
    static UMaterialInterface*
    Get_Material(
        const FCk_Handle_IskmProxy& InHandle,
        int32 InSlotIndex);

    // Returns the value recorded for the morph via Request_SetMorphTarget, or
    // 0 if unset (or cleared).
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Get Morph Target")
    static float
    Get_MorphTarget(
        const FCk_Handle_IskmProxy& InHandle,
        FName InMorphName);

    // Live curve weight on the base SKMC for the morph (0 before Setup
    // completes or if no curve is set). Reads the component, not the recorded
    // request state — use this to observe what the renderer actually applied.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Get Morph Target Weight")
    static float
    Get_MorphTargetWeight(
        const FCk_Handle_IskmProxy& InHandle,
        FName InMorphName);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Get Num Attached Submeshes")
    static int32
    Get_NumAttachedSubmeshes(const FCk_Handle_IskmProxy& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName = "[Ck][IskmProxy] Get Active Montage")
    static UAnimMontage*
    Get_ActiveMontage(const FCk_Handle_IskmProxy& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName = "[Ck][IskmProxy] Get Is Ragdolling")
    static bool
    Get_IsRagdolling(const FCk_Handle_IskmProxy& InHandle);

    // True once the entity outline (ck::FFragment_Usf_OutlineTarget) is applied to this proxy's SKMCs —
    // see CkUsf/Claude.md § Entity outlines. Used by autotests/gyms.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName = "[Ck][IskmProxy] Get Is Outline Applied")
    static bool
    Get_IsOutlineApplied(const FCk_Handle_IskmProxy& InHandle);

    // True once the entity cel pattern (ck::FFragment_Usf_CelPatternTarget) is applied to this proxy's
    // SKMCs — see CkUsf/Claude.md § Cel shade (Stylize). Used by autotests/gyms.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName = "[Ck][IskmProxy] Get Is Cel Pattern Applied")
    static bool
    Get_IsCelPatternApplied(const FCk_Handle_IskmProxy& InHandle);

    // The Custom-Stencil value written on this proxy's SKMCs by the cel pattern, or 0 when none is applied.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName = "[Ck][IskmProxy] Get Cel Pattern Stencil Value")
    static int32
    Get_CelPatternStencilValue(const FCk_Handle_IskmProxy& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Get Pose Source")
    static ECk_IskmProxy_PoseSource
    Get_PoseSource(const FCk_Handle_IskmProxy& InHandle);

    // Returns the live UAnimInstance currently driving the proxy's BaseSKMC,
    // or nullptr if the SKMC has none (Sequence-mode proxies may legitimately
    // have no AnimInstance during the brief window between
    // SetAnimInstanceClass(nullptr) and PlayAnimation completing).
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Get AnimInstance")
    static UAnimInstance*
    Get_AnimInstance(const FCk_Handle_IskmProxy& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Get Socket Transform")
    static FTransform
    Get_SocketTransform(
        const FCk_Handle_IskmProxy& InHandle,
        FName InSocketName,
        ECk_IskmProxy_TransformSpace InSpace);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Get Socket Location")
    static FVector
    Get_SocketLocation(
        const FCk_Handle_IskmProxy& InHandle,
        FName InSocketName,
        ECk_IskmProxy_TransformSpace InSpace);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Get Socket Rotation")
    static FRotator
    Get_SocketRotation(
        const FCk_Handle_IskmProxy& InHandle,
        FName InSocketName,
        ECk_IskmProxy_TransformSpace InSpace);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Get Socket Scale")
    static FVector
    Get_SocketScale(
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

    // ---- Requests ----

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Play Animation",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IskmProxy
    Request_PlayAnimation(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        const FCk_Request_IskmProxy_PlayAnimation& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Stop Animation",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IskmProxy
    Request_StopAnimation(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        const FCk_Request_IskmProxy_StopAnimation& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Set Play Rate",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IskmProxy
    Request_SetPlayRate(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        float InRate,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Set Visibility",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IskmProxy
    Request_SetVisibility(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        bool InIsVisible,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Set Custom Data Float",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IskmProxy
    Request_SetCustomDataFloat(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        int32 InOffset,
        float InValue,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // ---- per-proxy material overrides ----

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Set Material Override",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IskmProxy
    Request_SetMaterialOverride(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        const FCk_Request_IskmProxy_SetMaterialOverride& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Clear Material Overrides",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IskmProxy
    Request_ClearMaterialOverrides(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // ---- per-proxy morph targets ----

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Set Morph Target",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IskmProxy
    Request_SetMorphTarget(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        FName InMorphName,
        float InValue,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Clear Morph Targets",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IskmProxy
    Request_ClearMorphTargets(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Swaps the base body skeletal mesh (shared-skeleton male/female). The handler re-applies
    // recorded material/morph/custom-data so the outfit and body shape survive the swap.
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Set Skeletal Mesh",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IskmProxy
    Request_SetSkeletalMesh(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        USkeletalMesh* InMesh,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Attach Submesh",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IskmProxy
    Request_AttachSubmesh(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        FName InSubmeshName,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Detach Submesh",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IskmProxy
    Request_DetachSubmesh(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        FName InSubmeshName,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Detach All Submeshes",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IskmProxy
    Request_DetachAllSubmeshes(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Set AnimInstance Class",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IskmProxy
    Request_SetAnimInstanceClass(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        TSubclassOf<UAnimInstance> InClass,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Play Montage",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IskmProxy
    Request_PlayMontage(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        const FCk_Request_IskmProxy_PlayMontage& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Stop Montage",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IskmProxy
    Request_StopMontage(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        const FCk_Request_IskmProxy_StopMontage& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request Begin Ragdoll",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IskmProxy
    Request_BeginRagdoll(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        const FCk_Request_IskmProxy_BeginRagdoll& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Request End Ragdoll",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IskmProxy
    Request_EndRagdoll(
        UPARAM(ref) FCk_Handle_IskmProxy& InHandle,
        const FCk_Request_IskmProxy_EndRagdoll& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Follower transform is recomputed every frame (after the Transform request
    // pass) as Offset × Socket(Component-space) × LeaderEntityTransform —
    // lag-free with respect to the leader's root motion, unlike sampling the
    // leader SKMC's world-space socket (one frame stale by construction).
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Add Socket Follower")
    static FCk_Handle_Transform
    Add_SocketFollower(
        UPARAM(ref) FCk_Handle_IskmProxy& InLeader,
        UPARAM(ref) FCk_Handle_Transform& InFollower,
        FName InSocketName,
        const FTransform& InOffset);

    // Detach a follower previously attached with Add_SocketFollower (idempotent — no-op when no
    // follower fragment is present). The follower entity keeps its last transform; whoever
    // detaches owns driving it from then on (the LOD flip driver's far socket loop).
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Remove Socket Follower")
    static FCk_Handle_Transform
    Remove_SocketFollower(
        UPARAM(ref) FCk_Handle_Transform& InFollower);

    // ---- Binds ----

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
};
