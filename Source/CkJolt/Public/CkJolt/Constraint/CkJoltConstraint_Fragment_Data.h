#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkJoltConstraint_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// The Jolt constraint flavor this entity hosts. Distance keeps two anchor points within a range (a
// spring when soft limits are enabled); Point pins two anchor points together (rotation stays free —
// the chain/rope link); Hinge allows rotation about a single axis (doors), with optional limits,
// friction, and a motor.
UENUM(BlueprintType)
enum class ECk_JoltConstraint_Type : uint8
{
    Distance,
    Point,
    Hinge
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_JoltConstraint_Type);

// --------------------------------------------------------------------------------------------------------------------

// How the spring knobs are interpreted (mirrors JPH::ESpringMode): FrequencyAndDamping = oscillation
// frequency in Hz + damping ratio (0 = none, 1 = critical); StiffnessAndDamping = raw k and c in
// F = -k * x - c * v.
UENUM(BlueprintType)
enum class ECk_JoltConstraint_SpringMode : uint8
{
    FrequencyAndDamping,
    StiffnessAndDamping
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_JoltConstraint_SpringMode);

// --------------------------------------------------------------------------------------------------------------------

// Hinge motor state (mirrors JPH::EMotorState): Velocity drives to a target angular velocity (spinners,
// conveyor flaps); Position drives to a target angle (self-closing doors).
UENUM(BlueprintType)
enum class ECk_JoltConstraint_MotorState : uint8
{
    Off,
    Velocity,
    Position
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_JoltConstraint_MotorState);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKJOLT_API FCk_Handle_JoltConstraint : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_JoltConstraint); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_JoltConstraint);

// --------------------------------------------------------------------------------------------------------------------

// Config for a Jolt constraint between body A (the entity Create is called on) and body B
// (_OtherBody; an INVALID handle anchors to the world). All anchors/axes are WORLD-space at creation
// time — place the bodies where you want them constrained, then Create. Essentials = the constraint
// type; everything else is an optional fluent knob mirroring the JPH constraint-settings defaults.
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_JoltConstraint_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_JoltConstraint_Spec);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_JoltConstraint_Type _ConstraintType = ECk_JoltConstraint_Type::Distance;

    // Body B. Invalid = constrain body A to the WORLD.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _OtherBody;

    // World-space anchor on body A (for Hinge this is THE pivot; _WorldAnchorB is ignored).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _WorldAnchorA = FVector::ZeroVector;

    // World-space anchor on body B (or the world attachment point when _OtherBody is invalid).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _WorldAnchorB = FVector::ZeroVector;

    // Distance only. Negative = derive from the distance between the two anchors at creation.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_ConstraintType == ECk_JoltConstraint_Type::Distance"))
    float _MinDistance = -1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_ConstraintType == ECk_JoltConstraint_Type::Distance"))
    float _MaxDistance = -1.0f;

    // Distance only: Enable turns the range limits into a SPRING (soft limits).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_ConstraintType == ECk_JoltConstraint_Type::Distance"))
    ECk_EnableDisable _UseSpring = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_ConstraintType == ECk_JoltConstraint_Type::Distance"))
    ECk_JoltConstraint_SpringMode _SpringMode = ECk_JoltConstraint_SpringMode::FrequencyAndDamping;

    // Frequency in Hz (FrequencyAndDamping) or stiffness k (StiffnessAndDamping).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.0,
                      EditCondition = "_ConstraintType == ECk_JoltConstraint_Type::Distance"))
    float _SpringFrequencyOrStiffness = 2.0f;

    // Damping ratio (FrequencyAndDamping; 1 = critical) or damping c (StiffnessAndDamping).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.0,
                      EditCondition = "_ConstraintType == ECk_JoltConstraint_Type::Distance"))
    float _SpringDamping = 0.5f;

    // Hinge only. World-space rotation axis; (0,0,1) = a normal door swinging about the vertical.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_ConstraintType == ECk_JoltConstraint_Type::Hinge"))
    FVector _HingeAxis = FVector::UpVector;

    // Hinge only. [-180, 0] and [0, 180]; the full (-180, 180) range means NO limits.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = -180.0, ClampMax = 0.0,
                      EditCondition = "_ConstraintType == ECk_JoltConstraint_Type::Hinge"))
    float _LimitsMinDegrees = -180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.0, ClampMax = 180.0,
                      EditCondition = "_ConstraintType == ECk_JoltConstraint_Type::Hinge"))
    float _LimitsMaxDegrees = 180.0f;

    // Hinge only. Constant friction torque (N m) resisting rotation — a door that does not swing freely.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.0,
                      EditCondition = "_ConstraintType == ECk_JoltConstraint_Type::Hinge"))
    float _MaxFrictionTorque = 0.0f;

public:
    CK_PROPERTY_GET(_ConstraintType);
    CK_PROPERTY(_OtherBody);
    CK_PROPERTY(_WorldAnchorA);
    CK_PROPERTY(_WorldAnchorB);
    CK_PROPERTY(_MinDistance);
    CK_PROPERTY(_MaxDistance);
    CK_PROPERTY(_UseSpring);
    CK_PROPERTY(_SpringMode);
    CK_PROPERTY(_SpringFrequencyOrStiffness);
    CK_PROPERTY(_SpringDamping);
    CK_PROPERTY(_HingeAxis);
    CK_PROPERTY(_LimitsMinDegrees);
    CK_PROPERTY(_LimitsMaxDegrees);
    CK_PROPERTY(_MaxFrictionTorque);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_JoltConstraint_Spec, _ConstraintType);
};

// --------------------------------------------------------------------------------------------------------------------

// Enable/disable the constraint without destroying it (a disabled rope link is a cut that can heal).
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Request_JoltConstraint_SetEnabled : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_JoltConstraint_SetEnabled);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_JoltConstraint_SetEnabled);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _Enabled = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY_GET(_Enabled);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_JoltConstraint_SetEnabled, _Enabled);
};

// --------------------------------------------------------------------------------------------------------------------

// Distance constraints only: override the kept-apart range (UE units). Ignored with an ensure on other
// constraint types.
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Request_JoltConstraint_Distance_SetRange : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_JoltConstraint_Distance_SetRange);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_JoltConstraint_Distance_SetRange);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.0))
    float _MinDistance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.0))
    float _MaxDistance = 0.0f;

public:
    CK_PROPERTY_GET(_MinDistance);
    CK_PROPERTY_GET(_MaxDistance);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_JoltConstraint_Distance_SetRange, _MinDistance, _MaxDistance);
};

// --------------------------------------------------------------------------------------------------------------------

// Hinge constraints only: drive the motor. Velocity mode spins at _TargetAngularVelocityDegS; Position
// mode drives toward _TargetAngleDegrees (clamped to the hinge limits). Ignored with an ensure on other
// constraint types.
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Request_JoltConstraint_Hinge_SetMotor : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_JoltConstraint_Hinge_SetMotor);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_JoltConstraint_Hinge_SetMotor);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_JoltConstraint_MotorState _MotorState = ECk_JoltConstraint_MotorState::Off;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _TargetAngularVelocityDegS = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _TargetAngleDegrees = 0.0f;

public:
    CK_PROPERTY_GET(_MotorState);
    CK_PROPERTY(_TargetAngularVelocityDegS);
    CK_PROPERTY(_TargetAngleDegrees);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_JoltConstraint_Hinge_SetMotor, _MotorState);
};
