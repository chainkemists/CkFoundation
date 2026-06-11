#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkProjectile/Ballistic/CkBallistic_Fragment_Data.h"

#include "CkBallisticMotion_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKPROJECTILE_API FCk_Handle_BallisticMotion : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_BallisticMotion); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_BallisticMotion);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_BallisticMotion_ImpactResponse : uint8
{
    // Clamp to the impact point and stop simulating
    Stop,

    // Reflect off the impact normal (scaled by Restitution) and continue on a new trajectory segment
    Bounce
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_BallisticMotion_ImpactResponse);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_BallisticMotion_ImpactOutcome : uint8
{
    Bounced,
    Stopped
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_BallisticMotion_ImpactOutcome);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_BallisticMotion_LaunchTime : uint8
{
    // Anchor the trajectory at the current world time
    CurrentWorldTime,

    // Anchor the trajectory at a caller-provided world time. A time in the past launches the
    // projectile 'retroactively' — the basis of lag-compensated fast-forwarding
    OverrideTime
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_BallisticMotion_LaunchTime);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROJECTILE_API FCk_Fragment_BallisticMotion_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_BallisticMotion_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Ballistic_TrajectoryParams _TrajectoryParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_BallisticMotion_ImpactResponse _ImpactResponse = ECk_BallisticMotion_ImpactResponse::Stop;

    // Fraction of velocity along the impact normal retained after a bounce (coefficient of restitution)
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = "0", UIMax = "1", ClampMin = "0",
                  EditCondition = "_ImpactResponse == ECk_BallisticMotion_ImpactResponse::Bounce"))
    float _Restitution = 0.3f;

    // A bounce that would leave the projectile slower than this (cm/s) stops it instead
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = "0", ClampMin = "0",
                  EditCondition = "_ImpactResponse == ECk_BallisticMotion_ImpactResponse::Bounce"))
    float _MinBounceSpeed = 80.0f;

    // Distance (cm) to nudge the projectile away from the surface after a bounce so the same
    // contact is not immediately re-detected
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = "0", ClampMin = "0",
                  EditCondition = "_ImpactResponse == ECk_BallisticMotion_ImpactResponse::Bounce"))
    float _ImpactNudgeDistance = 0.2f;

public:
    CK_PROPERTY_GET(_TrajectoryParams);
    CK_PROPERTY(_ImpactResponse);
    CK_PROPERTY(_Restitution);
    CK_PROPERTY(_MinBounceSpeed);
    CK_PROPERTY(_ImpactNudgeDistance);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_BallisticMotion_ParamsData, _TrajectoryParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROJECTILE_API FCk_Request_BallisticMotion_Launch : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_BallisticMotion_Launch);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_BallisticMotion_Launch);

private:
    // Velocity at launch, in cm/s. The trajectory starts at the entity's current location
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _StartVelocity = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_BallisticMotion_LaunchTime _LaunchTimePolicy = ECk_BallisticMotion_LaunchTime::CurrentWorldTime;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                  EditCondition = "_LaunchTimePolicy == ECk_BallisticMotion_LaunchTime::OverrideTime"))
    FCk_Time _OverrideStartTime;

public:
    CK_PROPERTY_GET(_StartVelocity);
    CK_PROPERTY(_LaunchTimePolicy);
    CK_PROPERTY(_OverrideStartTime);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_BallisticMotion_Launch, _StartVelocity);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROJECTILE_API FCk_Request_BallisticMotion_Stop : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_BallisticMotion_Stop);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_BallisticMotion_Stop);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROJECTILE_API FCk_BallisticMotion_Payload_OnImpact
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_BallisticMotion_Payload_OnImpact);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _OtherEntity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FVector _ImpactLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FVector _ImpactNormal = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FVector _ImpactVelocity = FVector::ZeroVector;

    // World time at which the trajectory actually crossed the impact point — computed analytically,
    // so it is unaffected by how late the contact was observed
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Time _ImpactWorldTime;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_BallisticMotion_ImpactOutcome _Outcome = ECk_BallisticMotion_ImpactOutcome::Stopped;

public:
    CK_PROPERTY_GET(_OtherEntity);
    CK_PROPERTY_GET(_ImpactLocation);
    CK_PROPERTY_GET(_ImpactNormal);
    CK_PROPERTY_GET(_ImpactVelocity);
    CK_PROPERTY_GET(_ImpactWorldTime);
    CK_PROPERTY_GET(_Outcome);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_BallisticMotion_Payload_OnImpact,
        _OtherEntity, _ImpactLocation, _ImpactNormal, _ImpactVelocity, _ImpactWorldTime, _Outcome);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_BallisticMotion_OnImpact,
    FCk_Handle_BallisticMotion, InHandle,
    FCk_BallisticMotion_Payload_OnImpact, InPayload);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_BallisticMotion_OnTrajectoryChanged,
    FCk_Handle_BallisticMotion, InHandle,
    FCk_Ballistic_InitialConditions, InNewInitialConditions,
    int32, InTrajectorySegmentIndex);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_BallisticMotion_OnStopped,
    FCk_Handle_BallisticMotion, InHandle,
    FVector, InFinalLocation);

// --------------------------------------------------------------------------------------------------------------------
