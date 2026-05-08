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

    // B2: per-instance transform offsets relative to the entity transform. Reservation
    // only — Plan-1 always pins the SKMC to the entity transform exactly. Plan-2 honors
    // these in the cluster proxy. Declared now so callers don't migrate.
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
    bool _bLoop = true;

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
    bool _bUnique = false;

public:
    CK_PROPERTY_GET(_Sequence);
    CK_PROPERTY(_bLoop);
    CK_PROPERTY(_StartAt);
    CK_PROPERTY(_PlayRate);
    CK_PROPERTY(_TransitionDuration);
    CK_PROPERTY(_BlendOption);
    CK_PROPERTY(_bUnique);
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
