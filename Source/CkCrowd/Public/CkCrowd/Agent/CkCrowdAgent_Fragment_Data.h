#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkCore/Time/CkTime.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

#include "CkCrowdAgent_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_CrowdAgent_Setup;
    class FProcessor_CrowdAgent_Steering;
    class FProcessor_CrowdAgent_FaceAngle;
    class FProcessor_CrowdAgent_HandleRequests;
    class FProcessor_CrowdAgent_OnPathResolved;
    class FProcessor_CrowdAgent_OnRouteResolved;
    class FProcessor_CrowdAgent_OnVoxelPathResolved;
    class FProcessor_CrowdAgent_NeighborSync;
    class FProcessor_CrowdAgent_Separation;
    class FProcessor_CrowdAgent_AccelClamp;
    class FProcessor_CrowdAgent_AvoidanceSample;
    class FProcessor_CrowdAgent_BlockDetect;
    class FProcessor_CrowdAgent_BlockedRecheck;
    class FProcessor_CrowdAgent_PathRefresh;
}

class UCk_Utils_CrowdAgent_UE;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKCROWD_API FCk_Handle_CrowdAgent : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_CrowdAgent); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_CrowdAgent);

// --------------------------------------------------------------------------------------------------------------------

// Why an agent could not reach its goal. GoalOccupied: another agent stands on the destination, so
// the closest this agent can physically get is (SelfRadius + BlockerRadius) out — further than its
// arrival radius. NoProgress: the agent has been going nowhere for a while, for any other reason.
UENUM(BlueprintType)
enum class ECk_CrowdAgent_BlockedReason : uint8
{
    GoalOccupied,
    NoProgress
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_CrowdAgent_BlockedReason);

// What an agent does when it discovers its goal is unreachable.
//
// HoldAndRetry (default): stop, report OnGoalBlocked once, then re-check on a cadence and resume
// automatically the moment the goal clears — the caller needs no failure handling at all.
// FailMove: report OnGoalBlocked, then OnGoalFailed, then go Idle — the caller owns recovery.
UENUM(BlueprintType)
enum class ECk_CrowdAgent_BlockedPolicy : uint8
{
    HoldAndRetry,
    FailMove
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_CrowdAgent_BlockedPolicy);

// The agent's steering-state tag, exposed as a queryable value. The tags themselves
// (FTag_CrowdAgent_Idle/PathPending/Walking) are EnTT fragments invisible to BP/AS, and
// state discriminates failure modes no other getter can: Walking with zero desired
// velocity (steering wedge) vs Idle with a live goal (arrival/stop wedge) vs a stale
// PathPending (lost path request).
UENUM(BlueprintType)
enum class ECk_CrowdAgent_MovementState : uint8
{
    None,
    Idle,
    PathPending,
    Walking
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_CrowdAgent_MovementState);

// --------------------------------------------------------------------------------------------------------------------

// Reflected ECS params for a crowd agent (radius, height, tags, locomotion, separation, etc.).
// See the module's Tunables Reference for the defaults table.
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Fragment_CrowdAgent_ParamsData
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_CrowdAgent_ParamsData);

    friend class ck::FProcessor_CrowdAgent_Setup;
    friend class ck::FProcessor_CrowdAgent_Steering;
    friend class ck::FProcessor_CrowdAgent_NeighborSync;
    friend class ck::FProcessor_CrowdAgent_Separation;
    friend class ck::FProcessor_CrowdAgent_HandleRequests;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _Radius = 42.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _Height = 192.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FGameplayTagContainer _Tags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _MaxSpeed = 240.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _MaxAcceleration = 480.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="0.1"))
    float _MaxTurnRate = 4.0f;

    // Resolved via UCk_Nav_ProjectSettings_UE::_QueryFilters for EVERY FindPath this agent issues.
    // Empty tag -> NavData's default filter.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FGameplayTag _NavQueryFilter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _ArrivalRadius = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _WaypointArrivalRadius = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _SeparationRadius = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="0.0"))
    float _SeparationLookahead = 100.0f;

    // Fraction of _MaxSpeed contributed per fully-overlapping neighbor. The solver scales the
    // dimensionless falloff sum by (_SeparationWeight * _MaxSpeed), so the force lands in cm/s —
    // the same units as the path-follow velocity Steering combines it with.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="0.0"))
    float _SeparationWeight = 0.5f;

    // Inertia coefficient on the separation force: 0 = instant changes (vibrate-prone), 1 = fully
    // sticky (force never changes).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true, ClampMin="0.0", ClampMax="1.0"))
    float _SeparationInertia = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1"))
    int32 _MaxNeighborsForSteering = 6;

    // How much of a PushApart de-penetration displacement this agent absorbs while it is IDLE.
    // 1.0 lets a newcomer body-check an already-arrived agent clean off its own goal; an arrived
    // agent has no restoring drive, so that eviction is a one-way walk outward.
    // MUST NOT BE ZERO — idle-vs-idle de-overlap requires BOTH parties to yield, or two idle agents
    // stay interpenetrated forever.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(AllowPrivateAccess=true, ClampMin="0.05", ClampMax="1.0"))
    float _PushApartIdleYield = 0.25f;

    // What this agent does when it finds its goal unreachable. Per-agent on purpose: an employee who
    // must reach a counter may want FailMove, while a browsing customer should just wait.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    ECk_CrowdAgent_BlockedPolicy _BlockedPolicy = ECk_CrowdAgent_BlockedPolicy::HoldAndRetry;

    // Collision-channel bitfield stored as int32 — UPROPERTY does not support uint32 except as
    // bitfields. -1 is every bit set: the agent is in every channel until something narrows it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    int32 _CollisionFlags = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    int32 _IgnoreFlags = 0;

public:
    CK_PROPERTY(_Radius);
    CK_PROPERTY(_Height);
    CK_PROPERTY(_Tags);
    CK_PROPERTY(_MaxSpeed);
    CK_PROPERTY(_MaxAcceleration);
    CK_PROPERTY(_MaxTurnRate);
    CK_PROPERTY(_NavQueryFilter);
    CK_PROPERTY(_ArrivalRadius);
    CK_PROPERTY(_WaypointArrivalRadius);
    CK_PROPERTY(_SeparationRadius);
    CK_PROPERTY(_SeparationLookahead);
    CK_PROPERTY(_SeparationInertia);
    CK_PROPERTY(_SeparationWeight);
    CK_PROPERTY(_MaxNeighborsForSteering);
    CK_PROPERTY(_PushApartIdleYield);
    CK_PROPERTY(_BlockedPolicy);
    CK_PROPERTY(_CollisionFlags);
    CK_PROPERTY(_IgnoreFlags);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_CrowdAgent_ParamsData, _Radius, _Height);
};

// --------------------------------------------------------------------------------------------------------------------

// One entry in the per-agent neighbor cache, rebuilt each frame from the agent's probe overlaps.
// _RelativeOffset is (NbrLoc - SelfLoc) and _RelativeVelocity is (NbrVel - SelfVel), both in world
// space. Sorted by _Distance ascending; trimmed to _MaxNeighborsForSteering.
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_CrowdAgent_Neighbor
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_CrowdAgent_Neighbor);

private:
    UPROPERTY()
    FCk_Handle _Handle;

    UPROPERTY()
    FVector _RelativeOffset = FVector::ZeroVector;

    UPROPERTY()
    FVector _RelativeVelocity = FVector::ZeroVector;

    UPROPERTY()
    float _Distance = 0.0f;

public:
    CK_PROPERTY_GET(_Handle);
    CK_PROPERTY_GET(_RelativeOffset);
    CK_PROPERTY_GET(_RelativeVelocity);
    CK_PROPERTY_GET(_Distance);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_CrowdAgent_Neighbor, _Handle, _RelativeOffset, _RelativeVelocity, _Distance);
};

// --------------------------------------------------------------------------------------------------------------------

// Per-agent path-following state, advanced by the steering processor. _WaypointIndex is the index of
// the NEXT waypoint the agent heads toward, reset to 0 on each new MoveTo; _ActiveArrivalRadius
// caches the tolerance that applies to the CURRENT goal (params default or per-request override).
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Fragment_CrowdAgent_PathFollowData
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_CrowdAgent_PathFollowData);

    friend class ck::FProcessor_CrowdAgent_Steering;
    friend class ck::FProcessor_CrowdAgent_HandleRequests;
    friend class ck::FProcessor_CrowdAgent_OnPathResolved;
    friend class ck::FProcessor_CrowdAgent_OnRouteResolved;
    friend class ck::FProcessor_CrowdAgent_OnVoxelPathResolved;
    friend class ck::FProcessor_CrowdAgent_BlockedRecheck;
    friend class ck::FProcessor_CrowdAgent_PathRefresh;
    friend class ::UCk_Utils_CrowdAgent_UE;

private:
    UPROPERTY()
    int32 _WaypointIndex = 0;

    UPROPERTY()
    float _ActiveArrivalRadius = 30.0f;

    // World-space goal of the active MoveTo, so HandleRequests can no-op a re-issued MoveTo on
    // (nearly) the same goal — re-issuing resets the waypoint cursor and the final-stop never
    // latches (the "orbit" failure mode).
    UPROPERTY()
    FVector _ActiveGoal = FVector::ZeroVector;

    // World-space start of the CURRENT path segment. CkNavigation's ExtractWaypoints strips the
    // path's start point, so the first segment's incoming direction — which Steering's
    // plane-crossing retirement needs — cannot be recovered from the waypoint array alone.
    UPROPERTY()
    FVector _CurrentSegmentStart = FVector::ZeroVector;

    // A Partial path ends at the closest REACHABLE point, not the goal. Set at install when
    // that end falls outside the arrival radius of the (projected) goal: the final-stop latch
    // must then report OnGoalFailed, not OnGoalReached — an agent marooned off its goal's nav
    // region otherwise "arrives" at the stub's end and no caller can tell the difference.
    UPROPERTY()
    bool _ActivePathEndsShortOfGoal = false;

    // Stationary-markup confirmation serial current when this path was installed. PathRefresh
    // compares it against each confirmed disc's serial: only a disc that became visible to Recast
    // AFTER the path can trigger a re-path, so a path that already chose to pay a disc's cost is
    // never re-planned for it.
    UPROPERTY()
    uint64 _PathSerial = 0;

public:
    CK_PROPERTY_GET(_WaypointIndex);
    CK_PROPERTY_GET(_ActiveArrivalRadius);
    CK_PROPERTY_GET(_ActiveGoal);
    CK_PROPERTY_GET(_CurrentSegmentStart);
    CK_PROPERTY_GET(_ActivePathEndsShortOfGoal);
    CK_PROPERTY_GET(_PathSerial);
};

// --------------------------------------------------------------------------------------------------------------------

// Output of the steering processor: the velocity the agent WANTS this frame. The velocity-bridge
// processor copies it into FFragment_Velocity_Current.
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Fragment_CrowdAgent_DesiredVelocityData
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_CrowdAgent_DesiredVelocityData);

    friend class ck::FProcessor_CrowdAgent_Steering;
    friend class ck::FProcessor_CrowdAgent_HandleRequests;
    friend class ck::FProcessor_CrowdAgent_AccelClamp;
    friend class ck::FProcessor_CrowdAgent_AvoidanceSample;
    friend class ::UCk_Utils_CrowdAgent_UE;

private:
    UPROPERTY()
    FVector _Velocity = FVector::ZeroVector;

    // Last frame's _Velocity AFTER AccelClamp ramped it, and the baseline for next tick's clamp.
    // Independent of CkPhysics' FFragment_Velocity_Current, which has been min/max trimmed.
    UPROPERTY()
    FVector _LastVelocity = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_Velocity);
    CK_PROPERTY_GET(_LastVelocity);
};

// --------------------------------------------------------------------------------------------------------------------

// Cached target yaw for the face-angle processor, in RADIANS (converted at the BP boundary by
// Get_TargetYawDegrees). Surfaced for the debugger and tests; the orientation itself lives on the
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

// Public request — "go to this world location". _ArrivalRadiusOverride lets a caller stop further
// out than the params' _ArrivalRadius (e.g. a queue position shy of the counter).
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

// Public request — "FOLLOW this live target point". The goal is a transform HANDLE, not a position
// snapshot. The follow persists until a plain MoveTo or Stop lands, or the target handle dies (the
// agent then keeps its last resolved goal).
// - _RepathThresholdCm must exceed the MoveTo same-goal epsilon of 20cm, or every check re-paths.
// - _ResumeSlackCm is hysteresis against arrive/leave flicker at the arrival boundary.
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Request_CrowdAgent_FollowTarget : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_CrowdAgent_FollowTarget);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_CrowdAgent_FollowTarget);

    friend class ck::FProcessor_CrowdAgent_HandleRequests;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FCk_Handle_Transform _TargetPoint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FCk_Time _RepathPeriod = FCk_Time{0.25f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="25.0"))
    float _RepathThresholdCm = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="0.0"))
    float _ResumeSlackCm = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    ECk_Override _ArrivalRadiusOverrideMode = ECk_Override::DoNotOverride;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(AllowPrivateAccess=true, ClampMin="1.0",
                    EditCondition="_ArrivalRadiusOverrideMode == ECk_Override::Override"))
    float _ArrivalRadiusOverrideValue = 30.0f;

public:
    CK_PROPERTY_GET(_TargetPoint);
    CK_PROPERTY(_RepathPeriod);
    CK_PROPERTY(_RepathThresholdCm);
    CK_PROPERTY(_ResumeSlackCm);
    CK_PROPERTY(_ArrivalRadiusOverrideMode);
    CK_PROPERTY(_ArrivalRadiusOverrideValue);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_CrowdAgent_FollowTarget, _TargetPoint);
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

// Public request — override the agent's max speed at runtime (sprint/flee gaits). Applies from the
// next tick and persists until the next SetMaxSpeed; does not disturb the active path or goal.
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Request_CrowdAgent_SetMaxSpeed : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_CrowdAgent_SetMaxSpeed);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_CrowdAgent_SetMaxSpeed);

    friend class ck::FProcessor_CrowdAgent_HandleRequests;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _MaxSpeed = 240.0f;

public:
    CK_PROPERTY_GET(_MaxSpeed);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_CrowdAgent_SetMaxSpeed, _MaxSpeed);
};

// --------------------------------------------------------------------------------------------------------------------
// Delegate types for the CrowdAgent lifecycle signals: CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE
// (in _Fragment.h) references these by name but does not declare them, so they live at global scope.

// Payload of OnGoalBlocked. _BlockedBy is the agent standing on the goal — invalid when the reason
// is NoProgress, where there is no single culprit.
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_CrowdAgent_GoalBlockedInfo
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_CrowdAgent_GoalBlockedInfo);

private:
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_CrowdAgent_BlockedReason _Reason = ECk_CrowdAgent_BlockedReason::GoalOccupied;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Handle _BlockedBy;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _DistanceToGoal = 0.0f;

public:
    CK_PROPERTY_GET(_Reason);
    CK_PROPERTY_GET(_BlockedBy);
    CK_PROPERTY_GET(_DistanceToGoal);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_CrowdAgent_GoalBlockedInfo, _Reason, _BlockedBy, _DistanceToGoal);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_CrowdAgent_OnGoalReached,
    FCk_Handle_CrowdAgent, InAgent);

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_CrowdAgent_OnGoalFailed,
    FCk_Handle_CrowdAgent, InAgent);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_CrowdAgent_OnGoalBlocked,
    FCk_Handle_CrowdAgent, InAgent,
    FCk_CrowdAgent_GoalBlockedInfo, InInfo);

// --------------------------------------------------------------------------------------------------------------------
