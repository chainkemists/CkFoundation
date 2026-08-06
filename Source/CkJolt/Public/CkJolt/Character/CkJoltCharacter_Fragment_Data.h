#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkJoltCharacter_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Two independent axes: whether a body may push the character, and whether the character may push a body.
UENUM(BlueprintType)
enum class ECk_JoltCharacter_PushPolicy : uint8
{
    PushAndBePushed,
    PushOnly,
    BePushedOnly,
    Neither
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_JoltCharacter_PushPolicy);

// --------------------------------------------------------------------------------------------------------------------

// Mirrors Jolt's EGroundState. OnSteepSlope == OnSteepGround (touching too-steep ground, will slide);
// NotSupported == touching an object that cannot support the character (will fall).
UENUM(BlueprintType)
enum class ECk_JoltCharacter_GroundState : uint8
{
    OnGround,
    OnSteepSlope,
    InAir,
    NotSupported
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_JoltCharacter_GroundState);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKJOLT_API FCk_Handle_JoltCharacter : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_JoltCharacter); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_JoltCharacter);

// --------------------------------------------------------------------------------------------------------------------

// Config for a Jolt CharacterVirtual; optional knobs mirror JPH::CharacterVirtualSettings defaults.
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_JoltCharacter_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_JoltCharacter_Spec);

private:
    // Capsule radius (UE units). Matches FCk_Jolt_ShapeDimensions capsule semantics.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.1))
    float _CapsuleRadius = 34.0f;

    // Cylinder-section half-height (UE units) — the straight section only, matching UE's FKSphylElem. Total
    // capsule half-height is _CapsuleHalfHeight + _CapsuleRadius.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.1))
    float _CapsuleHalfHeight = 56.0f;

    // Character mass (kg) — used to push down objects the character stands on, and to scale push impulses.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.001))
    float _MassKg = 70.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.0, ClampMax = 90.0))
    float _MaxSlopeAngleDegrees = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.0))
    float _MaxStrengthNewtons = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_JoltCharacter_PushPolicy _PushPolicy = ECk_JoltCharacter_PushPolicy::PushAndBePushed;

    // Resolved against UCollisionProfile at setup to seed this character's Jolt object layer.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _CollisionProfileName = TEXT("Pawn");

public:
    CK_PROPERTY_GET(_CapsuleRadius);
    CK_PROPERTY_GET(_CapsuleHalfHeight);
    CK_PROPERTY(_MassKg);
    CK_PROPERTY(_MaxSlopeAngleDegrees);
    CK_PROPERTY(_MaxStrengthNewtons);
    CK_PROPERTY(_PushPolicy);
    CK_PROPERTY(_CollisionProfileName);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_JoltCharacter_Spec, _CapsuleRadius, _CapsuleHalfHeight);
};

// --------------------------------------------------------------------------------------------------------------------

// Desired world-space velocity (UE units/s), applied each stepping frame. A CONTINUOUS intent — the last set
// value persists until changed (set to zero to stop).
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Request_JoltCharacter_Move : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_JoltCharacter_Move);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_JoltCharacter_Move);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _DesiredVelocity = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_DesiredVelocity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_JoltCharacter_Move, _DesiredVelocity);
};

// --------------------------------------------------------------------------------------------------------------------

// Upward jump velocity (UE units/s, +Z). One-shot, consumed by the first stepping frame in which the
// character is supported; it stays armed while airborne.
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Request_JoltCharacter_Jump : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_JoltCharacter_Jump);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_JoltCharacter_Jump);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _JumpVelocity = 0.0f;

public:
    CK_PROPERTY_GET(_JumpVelocity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_JoltCharacter_Jump, _JumpVelocity);
};

// --------------------------------------------------------------------------------------------------------------------

// Instantly move to a world-space location, snapping the entity's Transform and step pose too (no
// interpolation across the jump).
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Request_JoltCharacter_Teleport : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_JoltCharacter_Teleport);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_JoltCharacter_Teleport);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _AlsoSetRotation = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_AlsoSetRotation == ECk_EnableDisable::Enable"))
    FRotator _Rotation = FRotator::ZeroRotator;

public:
    CK_PROPERTY_GET(_Location);
    CK_PROPERTY(_AlsoSetRotation);
    CK_PROPERTY(_Rotation);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_JoltCharacter_Teleport, _Location);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_JoltCharacter_OnGroundStateChanged,
    FCk_Handle_JoltCharacter, InHandle,
    ECk_JoltCharacter_GroundState, InGroundState);

// --------------------------------------------------------------------------------------------------------------------
