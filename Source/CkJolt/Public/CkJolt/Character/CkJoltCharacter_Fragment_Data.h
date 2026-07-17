#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkJoltCharacter_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// How a JoltCharacter interacts with rigid bodies it touches: whether a body may push the character, and
// whether the character may impulse (push) a body. Maps to Jolt's CharacterContactSettings per contact.
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

// The character's relationship to the ground beneath it, mirrored from Jolt's EGroundState onto the entity
// (BP/AS-readable). OnSteepSlope == Jolt's OnSteepGround (touching too-steep ground, will slide);
// NotSupported == touching an object that cannot support it (will fall).
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

// Config for a Jolt CharacterVirtual added onto an entity. Essentials = the capsule dimensions. Everything
// else is an optional fluent knob mirroring JPH::CharacterVirtualSettings / CharacterBaseSettings defaults.
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Fragment_JoltCharacter_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_JoltCharacter_ParamsData);

private:
    // Capsule radius (UE units). Matches FCk_Jolt_ShapeDimensions capsule semantics.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.1))
    float _CapsuleRadius = 34.0f;

    // Capsule cylinder-section half-height (UE units) — the straight section only, matching UE's FKSphylElem
    // and FCk_Jolt_ShapeDimensions._HalfHeight. Total capsule half-height is _CapsuleHalfHeight + _CapsuleRadius.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.1))
    float _CapsuleHalfHeight = 56.0f;

    // Character mass (kg) — used to push down objects the character stands on, and to scale push impulses.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.001))
    float _MassKg = 70.0f;

    // Maximum angle of slope the character can still walk on (degrees).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.0, ClampMax = 90.0))
    float _MaxSlopeAngleDegrees = 50.0f;

    // Maximum force (Newtons) with which the character can push other bodies.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.0))
    float _MaxStrengthNewtons = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_JoltCharacter_PushPolicy _PushPolicy = ECk_JoltCharacter_PushPolicy::PushAndBePushed;

    // The collision profile that seeds this character's Jolt object layer (v1 is profile-only), resolved
    // against UCollisionProfile at setup — the signature mirrors CkJolt's layer-table seeding.
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
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_JoltCharacter_ParamsData, _CapsuleRadius, _CapsuleHalfHeight);
};

// --------------------------------------------------------------------------------------------------------------------

// Set the character's desired world-space velocity (UE units/s). Applied each stepping frame: on the ground
// it drives horizontal movement (combined with ground velocity); in the air the character retains momentum.
// This is a CONTINUOUS intent — the last set value persists until changed (set to zero to stop).
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

// Add an upward jump velocity (UE units/s, +Z) the next time the character is supported. One-shot: consumed
// by the first stepping frame in which the character is on/against supporting ground; ignored while airborne.
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

// Instantly move the character to a world-space location, snapping the entity's Transform and step pose too
// (no interpolation across the jump). _AlsoSetRotation gates whether _Rotation is applied (enum-mode + value
// optionality, house rule). Ignored while the character has not finished setup.
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

// Fired when the character's ground state changes (e.g. OnGround -> InAir when walking off a ledge).
DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_JoltCharacter_OnGroundStateChanged,
    FCk_Handle_JoltCharacter, InHandle,
    ECk_JoltCharacter_GroundState, InGroundState);

// --------------------------------------------------------------------------------------------------------------------
