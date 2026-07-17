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
    TObjectPtr<UPhysicalMaterial> _PhysicalMaterial;

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

// Toggle a JoltBody between Awake and Asleep. Phase 4 adds the remaining request types (impulses,
// velocity, motion-type change, ...).
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
