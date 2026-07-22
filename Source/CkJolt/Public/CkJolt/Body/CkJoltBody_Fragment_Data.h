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

// Where a JoltBody's collision shape comes from.
UENUM(BlueprintType)
enum class ECk_JoltBody_ShapeSource : uint8
{
    ExplicitShape,
    StaticMeshAsset
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_JoltBody_ShapeSource);

// --------------------------------------------------------------------------------------------------------------------

// How a JoltBody's mass is determined.
UENUM(BlueprintType)
enum class ECk_JoltBody_MassSource : uint8
{
    FromShape,
    FromStaticMesh,
    Explicit
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_JoltBody_MassSource);

// --------------------------------------------------------------------------------------------------------------------

// How a JoltBody's center of mass is determined.
UENUM(BlueprintType)
enum class ECk_JoltBody_ComSource : uint8
{
    FromShape,
    ExplicitOffset
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_JoltBody_ComSource);

// --------------------------------------------------------------------------------------------------------------------

// How a JoltBody's surface (friction/restitution) is determined.
UENUM(BlueprintType)
enum class ECk_JoltBody_SurfaceSource : uint8
{
    PhysicalMaterial,
    Explicit
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_JoltBody_SurfaceSource);

// --------------------------------------------------------------------------------------------------------------------

// Awake / asleep state of a simulated Jolt body. Jolt puts settled dynamic bodies to sleep to save
// CPU; a kinematic/static body is always considered Awake.
UENUM(BlueprintType)
enum class ECk_Jolt_SleepState : uint8
{
    Awake,
    Asleep
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Jolt_SleepState);

// --------------------------------------------------------------------------------------------------------------------

// How a Teleport request treats the body's existing velocity: KeepVelocity leaves linear/angular velocity
// intact (the body keeps its momentum through the jump); ResetVelocity zeroes both (a clean respawn).
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

// Config for a Jolt-simulated rigid body added onto an entity. Essentials = the shape source (+ its
// payload). Everything else is an optional fluent knob mirroring JPH::BodyCreationSettings defaults.
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_ShapeSource == ECk_JoltBody_ShapeSource::StaticMeshAsset"))
    TObjectPtr<UStaticMesh> _StaticMesh;

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

    // Defaults mirror JPH::BodyCreationSettings (GravityFactor 1.0, Linear/AngularDamping 0.05).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _GravityFactor = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _LinearDamping = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _AngularDamping = 0.05f;

    // The collision profile that seeds this body's Jolt object layer (v1 is profile-only). Resolved
    // against UCollisionProfile at setup — the signature mirrors CkJolt's layer-table seeding.
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
    CK_PROPERTY(_CollisionProfileName);
    CK_PROPERTY(_PersistContacts);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_JoltBody_ParamsData, _ShapeSource);
};

// --------------------------------------------------------------------------------------------------------------------

// Toggle a JoltBody between Awake and Asleep.
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

// Apply a continuous force (UE units, Newton-equivalent) to a JoltBody's center of mass for the next
// sub-step. Activates the body; ignored while the body has not finished setup.
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

// Apply a continuous force at a world-space point (produces torque about the center of mass) for the
// next sub-step. Activates the body; ignored while the body has not finished setup.
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

// Apply a continuous torque (UE units) to a JoltBody for the next sub-step. Activates the body; ignored
// while the body has not finished setup.
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

// Apply an instantaneous impulse (UE units) at a JoltBody's center of mass. Activates the body; ignored
// while the body has not finished setup.
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

// Apply an instantaneous impulse at a world-space point (produces angular impulse about the center of
// mass). Activates the body; ignored while the body has not finished setup.
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

// Apply an instantaneous angular impulse (UE units) to a JoltBody. Activates the body; ignored while
// the body has not finished setup.
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

// Directly set a JoltBody's linear velocity (UE units/s) of the center of mass. Activates the body;
// ignored while the body has not finished setup.
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

// Directly set a JoltBody's angular velocity (UE units/s). Activates the body; ignored while the body
// has not finished setup.
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

// Instantly move a JoltBody to a world-space pose (position + rotation), snapping the entity's Transform
// and step pose too (no interpolation across the jump). _VelocityPolicy decides whether the pre-teleport
// velocity survives. Ignored while the body has not finished setup.
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

// Payload for a JoltBody contact begin/persist signal. _OtherEntity is the other body's entity (may be an
// INVALID handle when the other body has no live entity, e.g. a baked static-world body). _ContactPoints
// and _ContactNormal are on THIS body's surface; _RelativeNormalSpeed is the closing speed along the
// contact normal in UE units/s, POSITIVE when the bodies are approaching (impact hardness thresholds work
// as "> X"; the raw event keeps Jolt's negative-when-closing convention — the router negates it here);
// _OtherIsSensor reports whether the other body is a Jolt sensor.
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

// Payload for a JoltBody contact-removed signal. _OtherEntity is the other body's entity (may be an
// INVALID handle when the other body has no live entity).
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

// OnContactAdded and OnContactPersisted share the contact payload; the removed signal carries its own
// minimal payload. SleepStateChanged carries the body's new sleep state directly.
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
