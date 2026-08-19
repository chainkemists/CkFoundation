#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkJolt/CkJolt_Common.h"
#include "CkJolt/Query/CkJoltQuery_Data.h"

#include "CkJoltBody_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UStaticMesh;
class UPhysicalMaterial;

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_JoltBody_ShapeSource : uint8
{
    ExplicitShape,
    StaticMeshAsset
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_JoltBody_ShapeSource);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_JoltBody_MassSource : uint8
{
    FromShape,
    FromStaticMesh,
    Explicit
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_JoltBody_MassSource);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_JoltBody_ComSource : uint8
{
    FromShape,
    ExplicitOffset
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_JoltBody_ComSource);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_JoltBody_SurfaceSource : uint8
{
    PhysicalMaterial,
    Explicit
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_JoltBody_SurfaceSource);

// --------------------------------------------------------------------------------------------------------------------

// Jolt sleeps settled DYNAMIC bodies to save CPU; a kinematic/static body is always considered Awake.
UENUM(BlueprintType)
enum class ECk_Jolt_SleepState : uint8
{
    Awake,
    Asleep
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Jolt_SleepState);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Jolt_TeleportVelocityPolicy : uint8
{
    KeepVelocity,
    ResetVelocity
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Jolt_TeleportVelocityPolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKJOLT_API FCk_Handle_JoltBody : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_JoltBody); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_JoltBody);

// --------------------------------------------------------------------------------------------------------------------

// Config for a Jolt-simulated rigid body; every optional knob mirrors its JPH::BodyCreationSettings default.
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Fragment_JoltBody_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_JoltBody_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_JoltBody_ShapeSource _ShapeSource = ECk_JoltBody_ShapeSource::ExplicitShape;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_ShapeSource == ECk_JoltBody_ShapeSource::ExplicitShape"))
    FCk_Jolt_ShapeDimensions _ShapeDimensions;

    // Soft by design: a hard ref force-loads with the owning package and roots nothing anyway (GC
    // never walks the EnTT registry). The batch on Current roots the mesh through the load window.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_ShapeSource == ECk_JoltBody_ShapeSource::StaticMeshAsset"))
    TSoftObjectPtr<UStaticMesh> _StaticMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_MotionType _MotionType = ECk_MotionType::Dynamic;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_MotionQuality _MotionQuality = ECk_MotionQuality::Discrete;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Jolt_SleepState _InitialSleepState = ECk_Jolt_SleepState::Awake;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_JoltBody_MassSource _MassSource = ECk_JoltBody_MassSource::FromShape;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.001,
                      EditCondition = "_MassSource == ECk_JoltBody_MassSource::Explicit"))
    float _MassKg = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_JoltBody_ComSource _ComSource = ECk_JoltBody_ComSource::FromShape;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_ComSource == ECk_JoltBody_ComSource::ExplicitOffset"))
    FVector _ComOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_JoltBody_SurfaceSource _SurfaceSource = ECk_JoltBody_SurfaceSource::PhysicalMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_SurfaceSource == ECk_JoltBody_SurfaceSource::PhysicalMaterial"))
    TWeakObjectPtr<UPhysicalMaterial> _PhysicalMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.0,
                      EditCondition = "_SurfaceSource == ECk_JoltBody_SurfaceSource::Explicit"))
    float _Friction = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.0, ClampMax = 1.0,
                      EditCondition = "_SurfaceSource == ECk_JoltBody_SurfaceSource::Explicit"))
    float _Restitution = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _GravityFactor = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _LinearDamping = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _AngularDamping = 0.05f;

    /// Speed ceiling for this body, in the project's world units (cm/s). NOT a knob you can leave
    /// alone and forget: Jolt's own default is 500 expressed in ITS units, and ck::jolt::Conv is a
    /// component copy with no rescaling — a Jolt unit IS a centimetre here — so an unset value
    /// silently caps every dynamic body at 5 m/s. Anything thrown, launched, or falling from height
    /// reaches that ceiling and then behaves as though the force were far weaker. Measured on a
    /// ballistic throw solved for 850 cm/s: the body left at exactly 500 cm/s along the correct
    /// direction and landed at a third of the intended range. The default below is Jolt's 500 m/s
    /// INTENT re-expressed in this project's units; lower it per-body for a real terminal velocity.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.0))
    float _MaxLinearVelocity = 50000.0f;

    // Resolved against UCollisionProfile at setup to seed this body's Jolt object layer (v1 is profile-only).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _CollisionProfileName = TEXT("PhysicsActor");

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _PersistContacts = ECk_EnableDisable::Disable;

public:
    CK_PROPERTY_GET(_ShapeSource);
    CK_PROPERTY(_ShapeDimensions);
    CK_PROPERTY(_StaticMesh);
    CK_PROPERTY(_MotionType);
    CK_PROPERTY(_MotionQuality);
    CK_PROPERTY(_InitialSleepState);
    CK_PROPERTY(_MassSource);
    CK_PROPERTY(_MassKg);
    CK_PROPERTY(_ComSource);
    CK_PROPERTY(_ComOffset);
    CK_PROPERTY(_SurfaceSource);
    CK_PROPERTY(_PhysicalMaterial);
    CK_PROPERTY(_Friction);
    CK_PROPERTY(_Restitution);
    CK_PROPERTY(_GravityFactor);
    CK_PROPERTY(_LinearDamping);
    CK_PROPERTY(_AngularDamping);
    CK_PROPERTY(_MaxLinearVelocity);
    CK_PROPERTY(_CollisionProfileName);
    CK_PROPERTY(_PersistContacts);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_JoltBody_ParamsData, _ShapeSource);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Request_JoltBody_SetSleepState : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_JoltBody_SetSleepState);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_JoltBody_SetSleepState);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Jolt_SleepState _SleepState = ECk_Jolt_SleepState::Awake;

public:
    CK_PROPERTY_GET(_SleepState);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_JoltBody_SetSleepState, _SleepState);
};

// --------------------------------------------------------------------------------------------------------------------

// Continuous force (UE units, Newton-equivalent) at the center of mass for the next sub-step. Activates the body.
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Request_JoltBody_AddForce : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_JoltBody_AddForce);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_JoltBody_AddForce);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _Force = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_Force);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_JoltBody_AddForce, _Force);
};

// --------------------------------------------------------------------------------------------------------------------

// Continuous force at a world-space point (produces torque about the center of mass) for the next sub-step.
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Request_JoltBody_AddForceAtLocation : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_JoltBody_AddForceAtLocation);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_JoltBody_AddForceAtLocation);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _Force = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _WorldLocation = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_Force);
    CK_PROPERTY_GET(_WorldLocation);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_JoltBody_AddForceAtLocation, _Force, _WorldLocation);
};

// --------------------------------------------------------------------------------------------------------------------

// Continuous torque (UE units) for the next sub-step. Activates the body.
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Request_JoltBody_AddTorque : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_JoltBody_AddTorque);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_JoltBody_AddTorque);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _Torque = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_Torque);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_JoltBody_AddTorque, _Torque);
};

// --------------------------------------------------------------------------------------------------------------------

// Instantaneous impulse (UE units) at the center of mass. Activates the body.
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Request_JoltBody_AddImpulse : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_JoltBody_AddImpulse);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_JoltBody_AddImpulse);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _Impulse = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_Impulse);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_JoltBody_AddImpulse, _Impulse);
};

// --------------------------------------------------------------------------------------------------------------------

// Instantaneous impulse at a world-space point (produces angular impulse about the center of mass).
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Request_JoltBody_AddImpulseAtLocation : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_JoltBody_AddImpulseAtLocation);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_JoltBody_AddImpulseAtLocation);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _Impulse = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _WorldLocation = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_Impulse);
    CK_PROPERTY_GET(_WorldLocation);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_JoltBody_AddImpulseAtLocation, _Impulse, _WorldLocation);
};

// --------------------------------------------------------------------------------------------------------------------

// Instantaneous angular impulse (UE units). Activates the body.
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Request_JoltBody_AddAngularImpulse : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_JoltBody_AddAngularImpulse);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_JoltBody_AddAngularImpulse);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _AngularImpulse = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_AngularImpulse);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_JoltBody_AddAngularImpulse, _AngularImpulse);
};

// --------------------------------------------------------------------------------------------------------------------

// Linear velocity (UE units/s) of the center of mass, set directly. Activates the body.
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Request_JoltBody_SetLinearVelocity : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_JoltBody_SetLinearVelocity);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_JoltBody_SetLinearVelocity);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _LinearVelocity = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_LinearVelocity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_JoltBody_SetLinearVelocity, _LinearVelocity);
};

// --------------------------------------------------------------------------------------------------------------------

// Angular velocity (UE units/s), set directly. Activates the body.
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Request_JoltBody_SetAngularVelocity : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_JoltBody_SetAngularVelocity);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_JoltBody_SetAngularVelocity);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _AngularVelocity = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_AngularVelocity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_JoltBody_SetAngularVelocity, _AngularVelocity);
};

// --------------------------------------------------------------------------------------------------------------------

// Instantly move to a world-space pose, snapping the entity's Transform and step pose too (no interpolation
// across the jump). KeepVelocity preserves the pre-teleport momentum; ResetVelocity zeroes linear + angular.
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Request_JoltBody_Teleport : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_JoltBody_Teleport);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_JoltBody_Teleport);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FRotator _Rotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Jolt_TeleportVelocityPolicy _VelocityPolicy = ECk_Jolt_TeleportVelocityPolicy::ResetVelocity;

public:
    CK_PROPERTY_GET(_Location);
    CK_PROPERTY_GET(_Rotation);
    CK_PROPERTY(_VelocityPolicy);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_JoltBody_Teleport, _Location, _Rotation);
};

// --------------------------------------------------------------------------------------------------------------------

// Switch a live body's motion type. Dynamic/Kinematic activate the body; Static does not (a Static body is
// inert by definition). The drain also re-stamps the entity's motion-type tags, because
// FTag_JoltBody_KinematicFromECS is what selects a body INTO the kinematic push and OUT of the interpolated
// writeback — leaving it stale would have the body pushed from the ECS transform AND written back into it.
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Request_JoltBody_SetMotionType : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_JoltBody_SetMotionType);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_JoltBody_SetMotionType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_MotionType _MotionType = ECk_MotionType::Dynamic;

public:
    CK_PROPERTY_GET(_MotionType);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_JoltBody_SetMotionType, _MotionType);
};

// --------------------------------------------------------------------------------------------------------------------

// Move a live body onto the Jolt object layer derived from a UCollisionProfile name, using the same
// profile -> signature -> layer resolution FProcessor_JoltBody_Setup performs at creation. An unknown or
// collision-disabled profile ENSURES and is skipped — the body keeps its previous layer rather than silently
// landing on one that collides with nothing.
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Request_JoltBody_SetCollisionProfile : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_JoltBody_SetCollisionProfile);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_JoltBody_SetCollisionProfile);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _CollisionProfileName = NAME_None;

public:
    CK_PROPERTY_GET(_CollisionProfileName);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_JoltBody_SetCollisionProfile, _CollisionProfileName);
};

// --------------------------------------------------------------------------------------------------------------------

// Payload for a JoltBody contact begin/persist signal. _OtherEntity may be INVALID (the other body has no
// live entity). _ContactPoints/_ContactNormal are on THIS body's surface; _RelativeNormalSpeed is the closing
// speed along that normal in UE units/s, POSITIVE when approaching (Jolt's raw sign is the opposite).
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_JoltBody_Payload_OnContact
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_JoltBody_Payload_OnContact);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _OtherEntity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FVector> _ContactPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _ContactNormal = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _RelativeNormalSpeed = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _OtherIsSensor = ECk_EnableDisable::Disable;

public:
    CK_PROPERTY_GET(_OtherEntity);
    CK_PROPERTY_GET(_ContactPoints);
    CK_PROPERTY_GET(_ContactNormal);
    CK_PROPERTY_GET(_RelativeNormalSpeed);
    CK_PROPERTY_GET(_OtherIsSensor);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_JoltBody_Payload_OnContact, _OtherEntity, _ContactPoints, _ContactNormal, _RelativeNormalSpeed, _OtherIsSensor);
};

// --------------------------------------------------------------------------------------------------------------------

// Payload for a JoltBody contact-removed signal. _OtherEntity may be INVALID (the other body has no live entity).
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_JoltBody_Payload_OnContactRemoved
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_JoltBody_Payload_OnContactRemoved);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _OtherEntity;

public:
    CK_PROPERTY_GET(_OtherEntity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_JoltBody_Payload_OnContactRemoved, _OtherEntity);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_JoltBody_OnContact,
    FCk_Handle_JoltBody, InHandle,
    FCk_JoltBody_Payload_OnContact, InPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_JoltBody_OnContactRemoved,
    FCk_Handle_JoltBody, InHandle,
    FCk_JoltBody_Payload_OnContactRemoved, InPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_JoltBody_OnSleepStateChanged,
    FCk_Handle_JoltBody, InHandle,
    ECk_Jolt_SleepState, InSleepState);
