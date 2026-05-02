#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

#include "CkCrowdAgent_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Forward declarations for friend declarations below — the actual processor classes live in
// namespace ck (per the codebase convention), so the friend lines need the namespace.
namespace ck
{
    class FProcessor_CrowdAgent_Setup;
    class FProcessor_CrowdAgent_Steering;
    class FProcessor_CrowdAgent_FaceAngle;
    class FProcessor_CrowdAgent_HandleRequests;
    class FProcessor_CrowdAgent_OnPathResolved;
}

class UCk_Utils_CrowdAgent_UE;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKCROWD_API FCk_Handle_CrowdAgent : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_CrowdAgent); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_CrowdAgent);

// --------------------------------------------------------------------------------------------------------------------

// Reflected ECS params for a crowd agent. Gate 0 carries only structural fields
// (Radius, Height, Tags). Gate 2+ adds MaxSpeed/MaxAcceleration/MaxTurnRate/etc.
// Gate 3+ adds SeparationRadius/SeparationWeight/Flags/IgnoreFlags. Gate 4+ adds
// the piercing/sleep/replan tunables. See PLAN.md for the staged-rollout list and
// CkCrowd/Claude.md for the final defaults table.
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Fragment_CrowdAgent_ParamsData
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_CrowdAgent_ParamsData);

    friend class ck::FProcessor_CrowdAgent_Setup;
    friend class ck::FProcessor_CrowdAgent_Steering;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _Radius = 42.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _Height = 192.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FGameplayTagContainer _Tags;

    // Gate 2 — locomotion tunables. Defaults match the Tunables Reference table in CkCrowd/Claude.md.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _MaxSpeed = 240.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _MaxAcceleration = 480.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="0.1"))
    float _MaxTurnRate = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _ArrivalRadius = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _WaypointArrivalRadius = 25.0f;

public:
    CK_PROPERTY(_Radius);
    CK_PROPERTY(_Height);
    CK_PROPERTY(_Tags);
    CK_PROPERTY(_MaxSpeed);
    CK_PROPERTY(_MaxAcceleration);
    CK_PROPERTY(_MaxTurnRate);
    CK_PROPERTY(_ArrivalRadius);
    CK_PROPERTY(_WaypointArrivalRadius);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_CrowdAgent_ParamsData, _Radius, _Height);
};

// --------------------------------------------------------------------------------------------------------------------

// Per-agent path-following state. Written by the steering processor as the agent advances along the
// path produced by CkNavigation; lives on the agent entity. _WaypointIndex is the index of the NEXT
// waypoint the agent is heading toward (Waypoints[_WaypointIndex]); reset to 0 each new MoveTo.
// _ActiveArrivalRadius caches the per-request arrival tolerance — populated from either the params
// default or the MoveTo request's _ArrivalRadiusOverride, whichever applies for the current goal.
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Fragment_CrowdAgent_PathFollowData
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_CrowdAgent_PathFollowData);

    friend class ck::FProcessor_CrowdAgent_Steering;
    friend class ck::FProcessor_CrowdAgent_HandleRequests;
    friend class ck::FProcessor_CrowdAgent_OnPathResolved;
    friend class ::UCk_Utils_CrowdAgent_UE;

private:
    UPROPERTY()
    int32 _WaypointIndex = 0;

    UPROPERTY()
    float _ActiveArrivalRadius = 30.0f;

public:
    CK_PROPERTY_GET(_WaypointIndex);
    CK_PROPERTY_GET(_ActiveArrivalRadius);
};

// --------------------------------------------------------------------------------------------------------------------

// Output of the steering processor: the velocity the agent WANTS this frame (path-follow + future
// separation/piercing combined). Sub-task 2C will copy this into FFragment_Velocity_Current via a
// velocity-bridge processor; until then it's purely advisory and observable via Get_DesiredVelocity.
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Fragment_CrowdAgent_DesiredVelocityData
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_CrowdAgent_DesiredVelocityData);

    friend class ck::FProcessor_CrowdAgent_Steering;
    friend class ck::FProcessor_CrowdAgent_HandleRequests;
    friend class ::UCk_Utils_CrowdAgent_UE;

private:
    UPROPERTY()
    FVector _Velocity = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_Velocity);
};

// --------------------------------------------------------------------------------------------------------------------

// Cached target yaw for the face-angle processor. The processor computes _TargetYaw from the
// steering's desired-velocity heading each frame and lerps the SceneNode's current yaw toward it
// at _MaxTurnRate. Stored as RADIANS (FMath::Atan2 native unit; converted at the BP boundary by
// Get_TargetYawDegrees). Surfaced for the debugger / tests; the orientation itself lives on the
// SceneNode.
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Fragment_CrowdAgent_FaceAngleData
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_CrowdAgent_FaceAngleData);

    friend class ck::FProcessor_CrowdAgent_FaceAngle;
    friend class ::UCk_Utils_CrowdAgent_UE;

private:
    UPROPERTY()
    float _TargetYaw = 0.0f;

public:
    CK_PROPERTY_GET(_TargetYaw);
};

// --------------------------------------------------------------------------------------------------------------------

// Public request — "go to this world location". _ArrivalRadiusOverride carries an optional
// per-request arrival tolerance so callers can request a "stop further out" behavior (e.g. a queue
// position 100cm shy of the actual counter, vs the default 30cm dead-on stop). When the override
// is DoNotOverride, the params' _ArrivalRadius is used instead.
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Request_CrowdAgent_MoveTo : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_CrowdAgent_MoveTo);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_CrowdAgent_MoveTo);

    friend class ck::FProcessor_CrowdAgent_HandleRequests;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FVector _Target = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    ECk_Override _ArrivalRadiusOverrideMode = ECk_Override::DoNotOverride;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(AllowPrivateAccess=true, ClampMin="1.0",
                    EditCondition="_ArrivalRadiusOverrideMode == ECk_Override::Override"))
    float _ArrivalRadiusOverrideValue = 30.0f;

public:
    CK_PROPERTY_GET(_Target);
    CK_PROPERTY(_ArrivalRadiusOverrideMode);
    CK_PROPERTY(_ArrivalRadiusOverrideValue);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_CrowdAgent_MoveTo, _Target);
};

// --------------------------------------------------------------------------------------------------------------------

// Public request — "stop moving immediately". Cancels the active path, drops the agent back into
// Idle, zeroes the steering output. No payload.
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Request_CrowdAgent_Stop : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_CrowdAgent_Stop);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_CrowdAgent_Stop);

    friend class ck::FProcessor_CrowdAgent_HandleRequests;
};

// --------------------------------------------------------------------------------------------------------------------
// Delegate types for the CrowdAgent lifecycle signals. CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE
// (in _Fragment.h, inside namespace ck) references these by name but does not declare them — the
// pattern in CkAudio/AudioTrack puts the delegate at global scope and lets the signal macro pick
// it up. Single-param: the agent handle is the only payload (callers can derive everything else
// from the handle).

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_CrowdAgent_OnGoalReached,
    FCk_Handle_CrowdAgent, InAgent);

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_CrowdAgent_OnGoalFailed,
    FCk_Handle_CrowdAgent, InAgent);

// --------------------------------------------------------------------------------------------------------------------
