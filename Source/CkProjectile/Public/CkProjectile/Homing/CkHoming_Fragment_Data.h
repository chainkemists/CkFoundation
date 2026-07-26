#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkProjectile/Homing/CkHoming_ProNav.h"

#include "CkHoming_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKPROJECTILE_API FCk_Handle_Homing : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Homing); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Homing);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Homing_TargetMode : uint8
{
    // No target — homing is dormant until a SetTarget request arrives
    None,

    // Chasing an entity (with an optional point-on-target offset that moves with it)
    Entity,

    WorldPoint
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Homing_TargetMode);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Homing_TargetPoint : uint8
{
    // Home in on the target entity's transform location
    TargetLocation,

    // Home in on a specific world-space point captured relative to the target — it moves and
    // rotates with the target (e.g. a weak spot on a boss)
    WorldSpacePointOnTarget
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Homing_TargetPoint);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROJECTILE_API FCk_Fragment_Homing_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Homing_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Homing_GuidanceSettings _GuidanceSettings;

    // OnTargetMissed only fires when the projectile is within this distance of the target at the
    // moment it stops closing — prevents far-away course corrections from reading as misses.
    // 0 = no distance gate
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _MissNotifyDistanceThreshold = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _StartingState = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY_GET(_GuidanceSettings);
    CK_PROPERTY(_MissNotifyDistanceThreshold);
    CK_PROPERTY(_StartingState);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Homing_ParamsData, _GuidanceSettings);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROJECTILE_API FCk_Request_Homing_SetTargetEntity : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Homing_SetTargetEntity);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Homing_SetTargetEntity);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _TargetEntity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Homing_TargetPoint _TargetPoint = ECk_Homing_TargetPoint::TargetLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_TargetPoint == ECk_Homing_TargetPoint::WorldSpacePointOnTarget"))
    FVector _WorldSpacePoint = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_TargetEntity);
    CK_PROPERTY(_TargetPoint);
    CK_PROPERTY(_WorldSpacePoint);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Homing_SetTargetEntity, _TargetEntity);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROJECTILE_API FCk_Request_Homing_SetTargetLocation : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Homing_SetTargetLocation);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Homing_SetTargetLocation);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _TargetLocation = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_TargetLocation);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Homing_SetTargetLocation, _TargetLocation);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROJECTILE_API FCk_Request_Homing_ClearTarget : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Homing_ClearTarget);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Homing_ClearTarget);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROJECTILE_API FCk_Request_Homing_SetDesiredTimeToImpact : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Homing_SetDesiredTimeToImpact);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Homing_SetDesiredTimeToImpact);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Homing_ImpactTiming _ImpactTiming = ECk_Homing_ImpactTiming::ArriveAtDesiredTime;

    // Counts down from now — the guidance modulates its closure rate to land the intercept when
    // this elapses
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_ImpactTiming == ECk_Homing_ImpactTiming::ArriveAtDesiredTime"))
    FCk_Time _DesiredTimeToImpact;

public:
    CK_PROPERTY_GET(_ImpactTiming);
    CK_PROPERTY(_DesiredTimeToImpact);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Homing_SetDesiredTimeToImpact, _ImpactTiming);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROJECTILE_API FCk_Request_Homing_EnableDisable : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Homing_EnableDisable);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Homing_EnableDisable);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY_GET(_EnableDisable);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Homing_EnableDisable, _EnableDisable);
};

// --------------------------------------------------------------------------------------------------------------------

// The closest approach has passed: the projectile was closing on the target and is now receding
// (inside the miss-notify threshold, when one is set). Use to detonate proximity warheads or to
// let the target know it dodged
USTRUCT(BlueprintType)
struct CKPROJECTILE_API FCk_Homing_Payload_OnTargetMissed
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Homing_Payload_OnTargetMissed);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Target;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _MissDistance = 0.0f;

public:
    CK_PROPERTY(_Target);
    CK_PROPERTY(_MissDistance);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Homing_Payload_OnTargetMissed, _Target, _MissDistance);
};

// --------------------------------------------------------------------------------------------------------------------

// The chased entity ceased to exist (destroyed mid-flight). Homing deactivates; gameplay decides
// what the projectile does next (self-destruct, retarget, fly straight)
USTRUCT(BlueprintType)
struct CKPROJECTILE_API FCk_Homing_Payload_OnTargetLost
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Homing_Payload_OnTargetLost);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _LostTarget;

public:
    CK_PROPERTY(_LostTarget);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Homing_Payload_OnTargetLost, _LostTarget);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Homing_OnTargetMissed,
    FCk_Handle_Homing, InHandle,
    FCk_Homing_Payload_OnTargetMissed, InPayload);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Homing_OnTargetLost,
    FCk_Handle_Homing, InHandle,
    FCk_Homing_Payload_OnTargetLost, InPayload);

// --------------------------------------------------------------------------------------------------------------------
