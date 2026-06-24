#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkGraphics/CkGraphics_Common.h"

#include "CkIskmRenderer/Renderer/CkIskmRenderer_Fragment_Data.h"

#include "CkIskmProxy_Fragment_Data.generated.h"

class UCk_IskmAnimCollection_Data;
class UMaterialInterface;
class USkeletalMesh;

// ---- enums ----

UENUM(BlueprintType)
enum class ECk_IskmProxy_PoseSource : uint8
{
    Sequence,
    AnimBP,
    Ragdoll,
};

UENUM(BlueprintType)
enum class ECk_IskmProxy_TransformSpace : uint8
{
    World,
    Component,
};

UENUM(BlueprintType)
enum class ECk_IskmProxy_AnimFinishReason : uint8
{
    Completed,
    Stopped,
    Replaced,
};

// ---- typesafe handle ----

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKISKMRENDERER_API FCk_Handle_IskmProxy : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_IskmProxy);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_IskmProxy);

// ---- params ----

USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_Fragment_IskmProxy_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_IskmProxy_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_IskmRenderer _Renderer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FTransform _SpawnTransform = FTransform::Identity;

    // A3: drives the FTag_IskmProxy_Movable tag at Setup. Static proxies skip
    // FProcessor_IskmProxy_UpdateTransform every frame.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _IsMovable = ECk_EnableDisable::Enable;

    // B2: per-instance transform offsets relative to the entity transform. _LocalLocationOffset is
    // HONORED in Plan-1 (composed into the SKMC world transform each frame — see Setup + UpdateTransform);
    // _LocalRotationOffset and _ScaleMultiplier remain reserved for the Plan-2 cluster proxy.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _LocalLocationOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FRotator _LocalRotationOffset = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _ScaleMultiplier = FVector::OneVector;

    // B3: seed values applied to the per-instance custom data at Setup time. Each entry
    // sets one slot; entries past the renderer's _NumCustomDataFloat are ignored.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      TitleProperty = "Index #{_DataIndex}: {_Value}"))
    TArray<FCk_CustomPrimitiveData> _CustomInstanceDataDefaults;

public:
    CK_PROPERTY(_Renderer);
    CK_PROPERTY(_SpawnTransform);
    CK_PROPERTY(_IsMovable);
    CK_PROPERTY(_LocalLocationOffset);
    CK_PROPERTY(_LocalRotationOffset);
    CK_PROPERTY(_ScaleMultiplier);
    CK_PROPERTY(_CustomInstanceDataDefaults);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_IskmProxy_ParamsData, _Renderer, _SpawnTransform);
};

// ---- request structs ----

USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_Request_IskmProxy_PlayAnimation : public FCk_Request_Base
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_Request_IskmProxy_PlayAnimation);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IskmProxy_PlayAnimation);
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UAnimSequenceBase> _Sequence;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _Loop = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _StartAt = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _PlayRate = 1.0f;

    // B1: cross-fade transition fields. Plan-1 IGNORES them — `USkeletalMeshComponent::
    // PlayAnimation` uses `UAnimSingleNodeInstance` with no transition support. Plan-2's
    // GPU pose buffer generates transitions on demand and reads these. Reserved here so
    // callers don't rewrite every Request_PlayAnimation site when Plan-2 lands.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _TransitionDuration = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    EAlphaBlendOption _BlendOption = EAlphaBlendOption::Linear;

    // B1: HONORED in Plan-1. When true, if the same UAnimSequenceBase is already the
    // active _CurrentSequence, the request is a no-op (avoids restarting from frame 0).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _Unique = false;

public:
    CK_PROPERTY_GET(_Sequence);
    CK_PROPERTY(_Loop);
    CK_PROPERTY(_StartAt);
    CK_PROPERTY(_PlayRate);
    CK_PROPERTY(_TransitionDuration);
    CK_PROPERTY(_BlendOption);
    CK_PROPERTY(_Unique);
public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_IskmProxy_PlayAnimation, _Sequence);
};

USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_Request_IskmProxy_StopAnimation : public FCk_Request_Base
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_Request_IskmProxy_StopAnimation);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IskmProxy_StopAnimation);
};

USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_Request_IskmProxy_PlayMontage : public FCk_Request_Base
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_Request_IskmProxy_PlayMontage);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IskmProxy_PlayMontage);
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UAnimMontage> _Montage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _PlayRate = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FName _StartSection = NAME_None;
public:
    CK_PROPERTY_GET(_Montage);
    CK_PROPERTY(_PlayRate);
    CK_PROPERTY(_StartSection);
public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_IskmProxy_PlayMontage, _Montage);
};

USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_Request_IskmProxy_StopMontage : public FCk_Request_Base
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_Request_IskmProxy_StopMontage);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IskmProxy_StopMontage);
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _BlendOutTime = 0.25f;
public:
    CK_PROPERTY(_BlendOutTime);
};

USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_Request_IskmProxy_BeginRagdoll : public FCk_Request_Base
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_Request_IskmProxy_BeginRagdoll);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IskmProxy_BeginRagdoll);
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _Impulse = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FName _ImpulseBoneName = NAME_None;
public:
    CK_PROPERTY(_Impulse);
    CK_PROPERTY(_ImpulseBoneName);
};

// Symmetric counterpart to BeginRagdoll. Restores collision/sim state and flips
// _PoseSource back to AnimBP (if an AnimInstance is live) or Sequence (otherwise).
USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_Request_IskmProxy_EndRagdoll : public FCk_Request_Base
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_Request_IskmProxy_EndRagdoll);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IskmProxy_EndRagdoll);
};

// Deferred via the request queue so it runs in the same handle-requests pass as
// PlayAnimation / StopAnimation. Doing it synchronously in Utils would let a
// same-frame Request_PlayAnimation overwrite the rate when the queue runs.
USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_Request_IskmProxy_SetPlayRate : public FCk_Request_Base
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_Request_IskmProxy_SetPlayRate);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IskmProxy_SetPlayRate);
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _Rate = 1.0f;
public:
    CK_PROPERTY_GET(_Rate);
public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_IskmProxy_SetPlayRate, _Rate);
};

// Deferred via the request queue so visibility flips in the same handle-requests pass as
// animation/pose updates (avoids mid-frame flicker). Hides the base SKMC and all attached
// submesh SKMCs. Pool-hygiene note: a released SKMC keeps its last visibility; the V1 caller
// (named NPCs that never release their proxy) is unaffected — add a Setup reset if a future
// caller hides a proxy then returns its SKMC to the pool.
USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_Request_IskmProxy_SetVisibility : public FCk_Request_Base
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_Request_IskmProxy_SetVisibility);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IskmProxy_SetVisibility);
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _IsVisible = true;
public:
    CK_PROPERTY_GET(_IsVisible);
public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_IskmProxy_SetVisibility, _IsVisible);
};

// Per-instance custom data slot write. Bounded by AnimCollection's _NumCustomDataFloat
// (validated in the handler against InCustomData._Values.IsValidIndex). The handler
// also fans the write out to attached submesh SKMCs so material parameters stay in
// sync across the leader/follower pose.
USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_Request_IskmProxy_SetCustomDataFloat : public FCk_Request_Base
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_Request_IskmProxy_SetCustomDataFloat);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IskmProxy_SetCustomDataFloat);
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _Offset = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _Value = 0.0f;
public:
    CK_PROPERTY(_Offset);
    CK_PROPERTY(_Value);
public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_IskmProxy_SetCustomDataFloat, _Offset, _Value);
};

// Per-proxy material override on the BASE SKMC only (v1 scope — submeshes carry
// their own def-time override materials via FCk_IskmRenderer_MeshDesc). Stored
// sparsely in FFragment_IskmProxy_MaterialOverrides so EndPlay can restore the
// pooled SKMC's mesh-default materials and Setup can re-apply after a
// (re)allocation. SlotIndex is validated in the handler against the mesh's
// material-slot count.
USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_Request_IskmProxy_SetMaterialOverride : public FCk_Request_Base
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_Request_IskmProxy_SetMaterialOverride);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IskmProxy_SetMaterialOverride);
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _SlotIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UMaterialInterface> _Material;
public:
    CK_PROPERTY_GET(_SlotIndex);
    CK_PROPERTY_GET(_Material);
public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_IskmProxy_SetMaterialOverride, _SlotIndex, _Material);
};

// Drops every recorded override and restores the mesh's default material on
// all base-SKMC slots.
USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_Request_IskmProxy_ClearMaterialOverrides : public FCk_Request_Base
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_Request_IskmProxy_ClearMaterialOverrides);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IskmProxy_ClearMaterialOverrides);
};

// Per-proxy morph-target (blend-shape) write on the BASE SKMC only (v1 scope —
// LeaderPoseComponent copies bone transforms, not morph curves, so submeshes do
// NOT inherit these). Stored in FFragment_IskmProxy_MorphTargets so EndPlay can
// ClearMorphTargets() on the pooled SKMC and Setup can re-apply after a
// (re)allocation. Names that don't exist on the mesh are stored as curves with
// no visual effect (engine behavior) — no validation against the mesh's morph
// list is performed.
USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_Request_IskmProxy_SetMorphTarget : public FCk_Request_Base
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_Request_IskmProxy_SetMorphTarget);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IskmProxy_SetMorphTarget);
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FName _MorphName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _Value = 0.0f;
public:
    CK_PROPERTY_GET(_MorphName);
    CK_PROPERTY_GET(_Value);
public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_IskmProxy_SetMorphTarget, _MorphName, _Value);
};

// Drops every recorded morph target and clears all morph curves on the base SKMC.
USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_Request_IskmProxy_ClearMorphTargets : public FCk_Request_Base
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_Request_IskmProxy_ClearMorphTargets);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IskmProxy_ClearMorphTargets);
};

// Swaps the BASE SKMC's skeletal mesh (e.g. a male<->female body sharing one skeleton).
// SetSkeletalMesh re-runs InitAnim (fresh AnimInstance of the preserved AnimClass); the
// handler re-establishes the notify bridge and re-applies recorded material overrides,
// morphs and per-instance custom data so the swap keeps the current outfit / body shape.
USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_Request_IskmProxy_SetSkeletalMesh : public FCk_Request_Base
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_Request_IskmProxy_SetSkeletalMesh);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IskmProxy_SetSkeletalMesh);
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<USkeletalMesh> _Mesh;
public:
    CK_PROPERTY_GET(_Mesh);
public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_IskmProxy_SetSkeletalMesh, _Mesh);
};

// Outfit submesh attach. The submesh is identified by name (resolved against the
// Renderer PDA's _Submeshes array). Capped at MaxSubmeshPerInstance (default 15)
// because Plan-2 packs mesh presence into a 4-bit GPU custom-data field.
USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_Request_IskmProxy_AttachSubmesh : public FCk_Request_Base
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_Request_IskmProxy_AttachSubmesh);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IskmProxy_AttachSubmesh);
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FName _SubmeshName;
public:
    CK_PROPERTY_GET(_SubmeshName);
public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_IskmProxy_AttachSubmesh, _SubmeshName);
};

USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_Request_IskmProxy_DetachSubmesh : public FCk_Request_Base
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_Request_IskmProxy_DetachSubmesh);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IskmProxy_DetachSubmesh);
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FName _SubmeshName;
public:
    CK_PROPERTY_GET(_SubmeshName);
public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_IskmProxy_DetachSubmesh, _SubmeshName);
};

USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_Request_IskmProxy_DetachAllSubmeshes : public FCk_Request_Base
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_Request_IskmProxy_DetachAllSubmeshes);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IskmProxy_DetachAllSubmeshes);
};

// Switches the SKMC's AnimInstance class — flipping pose source between AnimBP
// (when InClass is valid) and Sequence (when nullptr). nullptr falls back to
// UCk_IskmNotify_AnimInstance so OnAnimationNotify / OnMontageFinished signals
// keep firing in sequence mode.
USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_Request_IskmProxy_SetAnimInstanceClass : public FCk_Request_Base
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_Request_IskmProxy_SetAnimInstanceClass);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IskmProxy_SetAnimInstanceClass);
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TSubclassOf<UAnimInstance> _AnimInstanceClass;
public:
    CK_PROPERTY(_AnimInstanceClass);
public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_IskmProxy_SetAnimInstanceClass, _AnimInstanceClass);
};

// ---- LineTrace types ----

USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_IskmProxy_LineTraceParams
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_IskmProxy_LineTraceParams);
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _Start = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _End = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _Thickness = 0.0f;
public:
    CK_PROPERTY(_Start);
    CK_PROPERTY(_End);
    CK_PROPERTY(_Thickness);
};

USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_IskmProxy_LineTraceResult
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_IskmProxy_LineTraceResult);
private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _bHit = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _Position = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _Normal = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FName _BoneName = NAME_None;
public:
    CK_PROPERTY(_bHit);
    CK_PROPERTY(_Position);
    CK_PROPERTY(_Normal);
    CK_PROPERTY(_BoneName);
};

// ---- signal payload wrappers ----
//
// Framework convention: CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE static_asserts that
// no T_Args are raw pointers, AND UE's DECLARE_DYNAMIC_DELEGATE macros can't reflect
// TObjectPtr<T> template params cleanly. The codebase pattern (AnimPlan, AudioTrack,
// Aggro, etc.) is to wrap UObject refs in a small BlueprintType struct that carries
// a TObjectPtr as a UPROPERTY field. Subscribers read the wrapped pointer via the
// generated CK_PROPERTY_GET accessor.

USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_IskmProxy_AnimSequenceRef
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_IskmProxy_AnimSequenceRef);
private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TObjectPtr<UAnimSequenceBase> _Sequence;
public:
    CK_PROPERTY_GET(_Sequence);
public:
    FCk_IskmProxy_AnimSequenceRef() = default;
    explicit FCk_IskmProxy_AnimSequenceRef(UAnimSequenceBase* InSequence) : _Sequence(InSequence) {}
};

USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_IskmProxy_MontageRef
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_IskmProxy_MontageRef);
private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TObjectPtr<UAnimMontage> _Montage;
public:
    CK_PROPERTY_GET(_Montage);
public:
    FCk_IskmProxy_MontageRef() = default;
    explicit FCk_IskmProxy_MontageRef(UAnimMontage* InMontage) : _Montage(InMontage) {}
};

// ---- signals + delegates ----
//
// Per CkEcs/CLAUDE.md "Signals" section: CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE
// expects the dynamic delegate to ALREADY be declared via DECLARE_DYNAMIC_DELEGATE_*
// — the macro doesn't auto-generate it from the name argument. Declare each delegate
// inline before the corresponding signal macro.
//
// Sibling pattern (CkAudioTrack_Fragment_Data.h:222 + CkAudioTrack_Fragment.h:97):
// DECLARE_DYNAMIC_DELEGATE_* lives at FILE scope, but the
// CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE invocations live INSIDE `namespace ck`
// — the generated `UUtils_Signal_*` classes end up in `ck::`. Call sites use the
// `ck::UUtils_Signal_*::Broadcast/Bind/Unbind` form. Mirror the split exactly.

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FCk_Delegate_IskmProxy_OnAnimationFinished,
    FCk_Handle_IskmProxy, InHandle,
    FCk_IskmProxy_AnimSequenceRef, InSequence,
    ECk_IskmProxy_AnimFinishReason, InReason);

DECLARE_DYNAMIC_DELEGATE_TwoParams(FCk_Delegate_IskmProxy_OnAnimationNotify,
    FCk_Handle_IskmProxy, InHandle,
    FName, InNotifyName);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FCk_Delegate_IskmProxy_OnMontageFinished,
    FCk_Handle_IskmProxy, InHandle,
    FCk_IskmProxy_MontageRef, InMontage,
    bool, bWasInterrupted);

namespace ck
{
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKISKMRENDERER_API,
        IskmProxy_OnAnimationFinished,
        FCk_Delegate_IskmProxy_OnAnimationFinished,
        FCk_Handle_IskmProxy,
        FCk_IskmProxy_AnimSequenceRef,
        ECk_IskmProxy_AnimFinishReason);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKISKMRENDERER_API,
        IskmProxy_OnAnimationNotify,
        FCk_Delegate_IskmProxy_OnAnimationNotify,
        FCk_Handle_IskmProxy,
        FName /* notify name */);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKISKMRENDERER_API,
        IskmProxy_OnMontageFinished,
        FCk_Delegate_IskmProxy_OnMontageFinished,
        FCk_Handle_IskmProxy,
        FCk_IskmProxy_MontageRef,
        bool /* was interrupted */);
}
