#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkCore/Time/CkTime.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkNavigation/Nav/CkNav_Fragment_Data.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

#include "CkCrowdAgent_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_CrowdAgent_Setup;
    class FProcessor_CrowdAgent_Steering;
    class FProcessor_CrowdAgent_FaceAngle;
    class FProcessor_CrowdAgent_FaceAngle3D;
    class FProcessor_CrowdAgent_HandleRequests;
    class FProcessor_CrowdAgent_OnPathResolved;
    class FProcessor_CrowdAgent_OnRouteResolved;
    class FProcessor_CrowdAgent_OnVoxelPathResolved;
    class FProcessor_CrowdAgent_OnGroundNavPathResolved;
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
// arrival radius; _BlockedBy names it. NoProgress: the agent stopped making progress along its path
// and re-planning did not help, so the obstruction is static and there is no blocker handle.
// GoalCrowded: the destination is not reachable through the pack already settled on it — _BlockedBy
// names the settled neighbour this agent came to rest behind, which may be several bodies out from
// the goal itself. It retries like GoalOccupied, because the pack is made of agents and will move.
// The causes retry differently — see ECk_CrowdAgent_BlockedPolicy.
UENUM(BlueprintType)
enum class ECk_CrowdAgent_BlockedReason : uint8
{
    GoalOccupied,
    NoProgress,
    GoalCrowded
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_CrowdAgent_BlockedReason);

// What an agent does when it discovers its goal is unreachable.
//
// HoldAndRetry (default): stop, report OnGoalBlocked once, then re-check on a cadence and resume
// automatically the moment the goal clears. For a GoalOccupied block that hold is UNBOUNDED — the
// blocker is another agent and waiting it out is the whole point (a queue), so the caller needs no
// failure handling. For a NoProgress block the obstruction is static and never clears on its own,
// so the re-checks are bounded by _BlockedMaxRetries and OnGoalFailed fires when they run out.
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

// Which locomotion the agent's steering pipeline is built around.
//
// Grounded (default): the agent is a walker. Every stage of the pipeline applies — the navmesh
// constraint that lands its Z on the walkable surface, the planar separation/push-apart forces, the
// 2D velocity-obstacle sampler, the Recast-bound stationary markup and path refresh, and the
// yaw-only facing.
// Flying: the agent moves through free space, so all of those are excluded by tag and its
// displacement is applied in three dimensions. Its path must come from a volumetric provider —
// a Recast polyline is a floor-hugging route no flyer wants — and it gets no avoidance at all:
// the sampler and both separation forces are planar, and a 3D replacement is not built.
UENUM(BlueprintType)
enum class ECk_CrowdAgent_Mode : uint8
{
    Grounded,
    Flying
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_CrowdAgent_Mode);

// --------------------------------------------------------------------------------------------------------------------

// Which provider owns the query the active movement episode is waiting on. Recorded when the
// episode dispatches, because the provider choice in RequestPathForActiveGoal reads MUTABLE state
// (the installed-path fragments and the PathPending tag) and so cannot be re-derived once the
// episode ends. Ending an episode has to abandon the right provider's in-flight query, and the
// path-trouble overlay has to name the provider that is actually stalled rather than assuming
// CkNavigation.
UENUM(BlueprintType)
enum class ECk_CrowdAgent_PathProvider : uint8
{
    None,           // No episode in flight
    Navigation,     // CkNavigation (Recast)
    PathNetwork,    // CkPathNetwork sidewalk follower
    VoxelNav,       // CkVoxelNav volumetric
    GroundNav       // CkGroundNav grounded surface
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_CrowdAgent_PathProvider);

// --------------------------------------------------------------------------------------------------------------------

// Whether the in-flight or installed route used its strict policy pass. Strict can exclude authored
// AvoidIfPossible volumes, stationary-crowd markup, or both. A genuine strict no-route verdict
// re-dispatches once with Permissive, which admits finite-cost soft areas while preserving hard ones.
UENUM(BlueprintType)
enum class ECk_CrowdAgent_PlanPhase : uint8
{
    Permissive,
    Strict
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_CrowdAgent_PlanPhase);

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

    // Filter for the STRICT phase of two-phase planning, where stationary-crowd markup is
    // impassable rather than a toll. A host whose agents plan with a custom _NavQueryFilter maps a
    // strict VARIANT of that filter here (same areas plus the UCk_NavArea_CrowdAgent exclusion) —
    // filter classes cannot compose at query time, so the permissive filter's own exclusions would
    // otherwise be lost for the strict attempt. Empty tag -> the framework's
    // UCk_NavQueryFilter_AvoidStandingCrowds, which excludes only the crowd area.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FGameplayTag _NavQueryFilterStrict;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _ArrivalRadius = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    ECk_EnableDisable _CloseGoalStrafe = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="0.0"))
    float _CloseGoalStrafeDistanceUu = 60.0f;

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

    // Read once, by Add, to stamp FTag_CrowdAgent_Flying. Every processor that opts a flyer out does
    // so on that tag, so changing this field afterwards changes nothing.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    ECk_CrowdAgent_Mode _AgentMode = ECk_CrowdAgent_Mode::Grounded;

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
    CK_PROPERTY(_NavQueryFilterStrict);
    CK_PROPERTY(_ArrivalRadius);
    CK_PROPERTY(_CloseGoalStrafe);
    CK_PROPERTY(_CloseGoalStrafeDistanceUu);
    CK_PROPERTY(_WaypointArrivalRadius);
    CK_PROPERTY(_SeparationRadius);
    CK_PROPERTY(_SeparationLookahead);
    CK_PROPERTY(_SeparationInertia);
    CK_PROPERTY(_SeparationWeight);
    CK_PROPERTY(_MaxNeighborsForSteering);
    CK_PROPERTY(_PushApartIdleYield);
    CK_PROPERTY(_BlockedPolicy);
    CK_PROPERTY(_AgentMode);
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
    friend class ck::FProcessor_CrowdAgent_OnGroundNavPathResolved;
    friend class ck::FProcessor_CrowdAgent_BlockDetect;
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

    // Monotonic identity of the accepted MoveTo episode that owns _ActiveGoal.
    // Same-goal requests ignored while Walking do not advance it, allowing a
    // callback consumer to reject an arrival owned by an earlier intent.
    UPROPERTY()
    int32 _ActiveMoveEpisode = 0;

    // Opaque caller identity for the accepted MoveTo that owns _ActiveGoal. Zero
    // preserves the ordinary uncorrelated request contract. A different nonzero
    // identity may claim a fresh episode even when its goal matches the walk in flight.
    UPROPERTY()
    int32 _ActiveMoveCorrelationId = 0;

    // World-space start of the CURRENT path segment. CkNavigation's ExtractWaypoints strips the
    // path's start point, so the first segment's incoming direction — which Steering's
    // plane-crossing retirement needs — cannot be recovered from the waypoint array alone.
    UPROPERTY()
    FVector _CurrentSegmentStart = FVector::ZeroVector;

    // Leading waypoints that path-install normalization must not retire before Steering has
    // physically followed them. PathNetwork uses one protected point when an agent starts inside
    // stationary markup: that point is the outward escape leg, not a stale async-path corner.
    UPROPERTY()
    int32 _ProtectedLeadingWaypointCount = 0;

    // Physical egress computed from the real location before a nav query starts outside painted
    // markup. OnPathResolved prepends and protects it, then clears this transient value.
    UPROPERTY()
    TArray<FVector> _PendingEscapePrefix;

    // A Partial path ends at the closest REACHABLE point, not the goal. Set at install when
    // that end falls outside the arrival radius of the (projected) goal: the final-stop latch
    // must then report OnGoalFailed, not OnGoalReached — an agent marooned off its goal's nav
    // region otherwise "arrives" at the stub's end and no caller can tell the difference.
    UPROPERTY()
    bool _ActivePathEndsShortOfGoal = false;

    // Incremented for every CkNavigation request issued for this movement episode. The result
    // carries the same opaque value so OnPathResolved can ignore an older deferred query.
    UPROPERTY()
    int32 _ActiveNavigationRequestRevision = 0;

    // Provider that owns the in-flight query for this episode. Set when the episode dispatches;
    // cleared when it ends. The end-of-episode seam routes its abandon on this, so it must be
    // recorded rather than re-derived — RequestPathForActiveGoal's provider choice reads state
    // the teardown has already changed.
    UPROPERTY()
    ECk_CrowdAgent_PathProvider _ActiveProvider = ECk_CrowdAgent_PathProvider::None;

    // Stationary-markup confirmation serial current when this path was installed. PathRefresh
    // compares it against each confirmed disc's serial: only a disc that became visible to Recast
    // AFTER the path can trigger a re-path, so a path that already chose to pay a disc's cost is
    // never re-planned for it.
    UPROPERTY()
    uint64 _PathSerial = 0;

    // Filter phase of the in-flight query — OnPathResolved must know which phase just answered to
    // decide between the permissive fallback and the terminal failure flow.
    UPROPERTY()
    ECk_CrowdAgent_PlanPhase _PlanPhase = ECk_CrowdAgent_PlanPhase::Permissive;

    // _PlanPhase can be Strict solely because AvoidIfPossible volumes are active. Persist whether
    // this installed query also used the stationary-crowd exclusion filter so chord gates do not
    // re-derive a different base filter after settings change.
    UPROPERTY()
    bool _PlanUsesStrictStandingCrowdFilter = false;

    // A strict plan failed during this movement episode. This suppresses repeated strict retries
    // for both standing-crowd and AvoidIfPossible-volume policy.
    UPROPERTY()
    bool _StrictPlanFailed = false;

    // The public goal-failed payload specifically promises that standing bodies blocked the strict
    // route. Keep that meaning separate now that a volume-only strict phase can also fall back.
    UPROPERTY()
    bool _StrictStandingCrowdPlanFailed = false;

public:
    CK_PROPERTY_GET(_WaypointIndex);
    CK_PROPERTY_GET(_ActiveArrivalRadius);
    CK_PROPERTY_GET(_ActiveGoal);
    CK_PROPERTY_GET(_ActiveMoveEpisode);
    CK_PROPERTY_GET(_ActiveMoveCorrelationId);
    CK_PROPERTY_GET(_CurrentSegmentStart);
    CK_PROPERTY_GET(_ProtectedLeadingWaypointCount);
    CK_PROPERTY_GET(_PendingEscapePrefix);
    CK_PROPERTY_GET(_ActivePathEndsShortOfGoal);
    CK_PROPERTY_GET(_ActiveNavigationRequestRevision);
    CK_PROPERTY_GET(_ActiveProvider);
    CK_PROPERTY_GET(_PathSerial);
    CK_PROPERTY_GET(_PlanPhase);
    CK_PROPERTY_GET(_PlanUsesStrictStandingCrowdFilter);
    CK_PROPERTY_GET(_StrictPlanFailed);
    CK_PROPERTY_GET(_StrictStandingCrowdPlanFailed);
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
    friend class ck::FProcessor_CrowdAgent_BlockDetect;
    friend class ck::FProcessor_CrowdAgent_BlockedRecheck;
    friend class ck::FProcessor_CrowdAgent_OnPathResolved;
    friend class ::UCk_Utils_CrowdAgent_UE;

private:
    UPROPERTY()
    FVector _Velocity = FVector::ZeroVector;

    // Last frame's _Velocity AFTER AccelClamp ramped it, and the baseline for next tick's clamp.
    // Independent of CkPhysics' FFragment_Velocity_Current, which has been min/max trimmed.
    UPROPERTY()
    FVector _LastVelocity = FVector::ZeroVector;

    UPROPERTY()
    bool _CloseGoalStrafeActive = false;

public:
    CK_PROPERTY_GET(_Velocity);
    CK_PROPERTY_GET(_LastVelocity);
    CK_PROPERTY_GET(_CloseGoalStrafeActive);
};

// --------------------------------------------------------------------------------------------------------------------

// Committed facing target for the face-angle processor, in RADIANS (converted at the BP boundary by
// Get_TargetYawDegrees). Surfaced for the debugger and tests; the orientation itself lives on the
// SceneNode. _TargetPitch stays at its default for a grounded agent — only the flying facing
// processor writes it.
//
// The remaining fields are the facing filter's own state: the desired-velocity heading is only
// tracked while the agent is genuinely moving (_FacingEngaged), and a large heading change is held
// as a candidate until it repeats rather than committed on the frame it appears.
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Fragment_CrowdAgent_FaceAngleData
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_CrowdAgent_FaceAngleData);

    friend class ck::FProcessor_CrowdAgent_FaceAngle;
    friend class ck::FProcessor_CrowdAgent_FaceAngle3D;
    friend class ::UCk_Utils_CrowdAgent_UE;

private:
    UPROPERTY()
    float _TargetYaw = 0.0f;

    UPROPERTY()
    float _TargetPitch = 0.0f;

    UPROPERTY()
    bool _FacingEngaged = false;

    UPROPERTY()
    float _PendingTargetYaw = 0.0f;

    UPROPERTY()
    int32 _PendingTargetFrames = 0;

public:
    CK_PROPERTY_GET(_TargetYaw);
    CK_PROPERTY_GET(_TargetPitch);
    CK_PROPERTY_GET(_FacingEngaged);
};

// --------------------------------------------------------------------------------------------------------------------

// Public request — "go to this world location". _ArrivalRadiusOverride lets a caller choose a
// tighter or wider final-stop radius than the agent default for this movement episode.
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

    // Optional caller identity for correlating a retained arrival with the exact
    // accepted request that authored it. Zero keeps legacy same-goal no-op behavior.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    int32 _CorrelationId = 0;

    // Skips the same-goal no-op so a caller can demand a fresh path to the goal it is already
    // walking to. The guard exists to stop a noisy re-issuer resetting the waypoint cursor forever;
    // this is the explicit lever for the caller who knows the world changed under the frozen path.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    bool _ForceRepath = false;

public:
    CK_PROPERTY_GET(_Target);
    CK_PROPERTY(_ArrivalRadiusOverrideMode);
    CK_PROPERTY(_ArrivalRadiusOverrideValue);
    CK_PROPERTY(_CorrelationId);
    CK_PROPERTY(_ForceRepath);

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

// Transient, per-agent comfort-space override. This intentionally scales only the
// reactive separation volume; it never changes physical body radius, collision, nav
// projection, predictive avoidance TOI, or PushApart contact radius.
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Request_CrowdAgent_SetTransientPersonalSpaceScale : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_CrowdAgent_SetTransientPersonalSpaceScale);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_CrowdAgent_SetTransientPersonalSpaceScale);

    friend class ck::FProcessor_CrowdAgent_HandleRequests;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    float _Scale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    float _DurationSeconds = 0.0f;

public:
    CK_PROPERTY_GET(_Scale);
    CK_PROPERTY_GET(_DurationSeconds);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_CrowdAgent_SetTransientPersonalSpaceScale, _Scale, _DurationSeconds);
};

// Public request — replace this agent's navigation-query policy. Policy requests are resolved as
// batch setup before movement requests, so the last policy in a frame applies to every MoveTo or
// FollowTarget accepted in that frame. When enabled, _ForceReplan rebuilds an active Recast or
// PathNetwork route without changing its movement episode, goal, arrival radius, correlation, or
// follow ownership. Voxel paths do not consume Recast query filters and remain undisturbed.
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Request_CrowdAgent_SetNavQueryFilter : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_CrowdAgent_SetNavQueryFilter);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_CrowdAgent_SetNavQueryFilter);

    friend class ck::FProcessor_CrowdAgent_HandleRequests;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FGameplayTag _NavQueryFilter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    ECk_EnableDisable _ForceReplan = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY_GET(_NavQueryFilter);
    CK_PROPERTY(_ForceReplan);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_CrowdAgent_SetNavQueryFilter, _NavQueryFilter);
};

// --------------------------------------------------------------------------------------------------------------------
// Delegate types for the CrowdAgent lifecycle signals: CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE
// (in _Fragment.h) references these by name but does not declare them, so they live at global scope.

// Why a movement episode terminally failed. The high-level reason a caller branches on; the nav
// layer's detail rides alongside in FCk_CrowdAgent_GoalFailedInfo.
UENUM(BlueprintType)
enum class ECk_CrowdAgent_GoalFailReason : uint8
{
    PathFailed,                    // CkNavigation answered Failed — no route to the goal at all
    PathEndsShortOfGoal,           // a Partial path was walked to its end; the goal is unreachable from there
    NoProgressRetriesExhausted,    // the agent stopped making progress and every bounded retry re-wedged
    BlockedFailMovePolicy          // the goal is blocked and this agent's _BlockedPolicy is FailMove; the OnGoalBlocked fired just before this names the blocker
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_CrowdAgent_GoalFailReason);

// --------------------------------------------------------------------------------------------------------------------

// Payload of OnGoalFailed — the clear message a queue manager or planner acts on. The load-bearing
// bit is _NoCrowdFreeRouteExisted: true means the strict planning phase found no route that avoids
// STANDING BODIES, so the goal may open up when they move (retry later, or wait); false means the
// failure is structural (walls, fixtures, no navmesh) and retrying against unchanged nav is futile.
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_CrowdAgent_GoalFailedInfo
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_CrowdAgent_GoalFailedInfo);

private:
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_CrowdAgent_GoalFailReason _Reason = ECk_CrowdAgent_GoalFailReason::PathFailed;

    // Detail for PathFailed; None for the other reasons.
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_Nav_PathFailReason _NavFailReason = ECk_Nav_PathFailReason::None;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _NoCrowdFreeRouteExisted = false;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _GoalLocation = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_Reason);
    CK_PROPERTY_GET(_NavFailReason);
    CK_PROPERTY_GET(_NoCrowdFreeRouteExisted);
    CK_PROPERTY_GET(_GoalLocation);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_CrowdAgent_GoalFailedInfo,
        _Reason, _NavFailReason, _NoCrowdFreeRouteExisted, _GoalLocation);
};

// --------------------------------------------------------------------------------------------------------------------

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

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_CrowdAgent_OnGoalFailed,
    FCk_Handle_CrowdAgent, InAgent,
    FCk_CrowdAgent_GoalFailedInfo, InInfo);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_CrowdAgent_OnGoalBlocked,
    FCk_Handle_CrowdAgent, InAgent,
    FCk_CrowdAgent_GoalBlockedInfo, InInfo);

// --------------------------------------------------------------------------------------------------------------------
