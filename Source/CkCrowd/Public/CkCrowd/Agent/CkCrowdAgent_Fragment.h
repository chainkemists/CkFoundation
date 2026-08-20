#pragma once

#include "CkCrowd/Agent/CkCrowdAgent_Fragment_Data.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkNavigation/NavAreaMarkup/CkNavAreaMarkup_Utils.h"
#include "CkNavigation/Nav/CkNav_Fragment_Data.h"

#include "CkPathNetwork/Network/CkPathNetwork_Fragment_Data.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_CrowdAgent_Params         = FCk_Fragment_CrowdAgent_ParamsData;
    using FFragment_CrowdAgent_PathFollow     = FCk_Fragment_CrowdAgent_PathFollowData;
    using FFragment_CrowdAgent_DesiredVelocity = FCk_Fragment_CrowdAgent_DesiredVelocityData;
    using FFragment_CrowdAgent_FaceAngle      = FCk_Fragment_CrowdAgent_FaceAngleData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKCROWD_API FFragment_CrowdAgent_MoveRequests
    {
    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_MoveRequests);
        friend class FProcessor_CrowdAgent_HandleRequests;
        friend class ::UCk_Utils_CrowdAgent_UE;

    public:
        using MoveToRequestType       = FCk_Request_CrowdAgent_MoveTo;
        using FollowTargetRequestType = FCk_Request_CrowdAgent_FollowTarget;
        using StopRequestType         = FCk_Request_CrowdAgent_Stop;
        using SetMaxSpeedRequestType  = FCk_Request_CrowdAgent_SetMaxSpeed;
        using SetNavQueryFilterRequestType = FCk_Request_CrowdAgent_SetNavQueryFilter;
        using SetTransientPersonalSpaceScaleRequestType = FCk_Request_CrowdAgent_SetTransientPersonalSpaceScale;
        using RequestType             = std::variant<MoveToRequestType, FollowTargetRequestType, StopRequestType, SetMaxSpeedRequestType, SetNavQueryFilterRequestType, SetTransientPersonalSpaceScaleRequestType>;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // Runtime-only state; omitted from snapshot payloads because a comfort squeeze must never
    // survive a save/load boundary.
    struct CKCROWD_API FFragment_CrowdAgent_TransientPersonalSpace
    {
    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_TransientPersonalSpace);
        friend class FProcessor_CrowdAgent_HandleRequests;
        friend class FProcessor_CrowdAgent_TransientPersonalSpace;

    private:
        float _Scale = 1.0f;
        float _RemainingSeconds = 0.0f;

    public:
        CK_PROPERTY_GET(_Scale);
        CK_PROPERTY_GET(_RemainingSeconds);
    };

    // Added by HandleRequests when a FollowTarget request lands; removed by a plain MoveTo, by
    // Stop, or when the target handle dies. Presence IS the FollowTarget processor's view filter.
    struct CKCROWD_API FFragment_CrowdAgent_FollowTarget
    {
    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_FollowTarget);
        friend class FProcessor_CrowdAgent_HandleRequests;
        friend class FProcessor_CrowdAgent_FollowTarget;

    private:
        FCk_Request_CrowdAgent_FollowTarget _Request;
        float _RepathAccumulatorSec = 0.0f;

    public:
        CK_PROPERTY_GET(_Request);
    };

    // Identity of the last path-network corridor installed into this agent's nav-path slot: goal
    // pins which MoveTo it answered, epoch pins which build of the network it was planned against.
    struct CKCROWD_API FFragment_CrowdAgent_InstalledRoute
    {
    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_InstalledRoute);

        friend class FProcessor_CrowdAgent_OnRouteResolved;

    private:
        FVector _GoalLocation = FVector::ZeroVector;
        int32 _NetworkEpoch = 0;
        int32 _TuningRevision = 0;

    public:
        CK_PROPERTY_GET(_GoalLocation);
        CK_PROPERTY_GET(_NetworkEpoch);
        CK_PROPERTY_GET(_TuningRevision);
    };

    // Identity of the last volumetric path installed into this agent's nav-path slot: goal pins which
    // MoveTo it answered, epoch pins which bake of the volume it was planned against.
    struct CKCROWD_API FFragment_CrowdAgent_InstalledVoxelPath
    {
    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_InstalledVoxelPath);

        friend class FProcessor_CrowdAgent_OnVoxelPathResolved;

    private:
        FVector _GoalLocation = FVector::ZeroVector;
        int32 _VolumeEpoch = 0;

        // The volume epoch the stale-epoch replan last asked against. It is stamped BEFORE the answer
        // arrives — an answer that fails never updates _VolumeEpoch, and without a separate record the
        // drift would be re-requested every frame forever.
        int32 _RequestedAgainstEpoch = 0;

    public:
        CK_PROPERTY_GET(_GoalLocation);
        CK_PROPERTY_GET(_VolumeEpoch);
        CK_PROPERTY_GET(_RequestedAgainstEpoch);
    };

    // Value-only record of the last pathfinding problem in the current movement episode. A
    // PathNetwork failure survives the CkNavigation fallback overwriting FFragment_Nav_PathResult,
    // so both the in-world overlay and the Crowd Debugger can explain which layer had trouble.
    // Reset only when a genuinely new MoveTo is accepted or Stop abandons the goal.
    struct CKCROWD_API FFragment_CrowdAgent_PathTrouble
    {
    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_PathTrouble);

        friend class FProcessor_CrowdAgent_HandleRequests;
        friend class FProcessor_CrowdAgent_OnRouteResolved;
        friend class FProcessor_CrowdAgent_OnVoxelPathResolved;
        friend class FProcessor_CrowdAgent_OnPathResolved;

        static constexpr auto FadeDurationSeconds = 5.0;

    private:
        FVector _AgentLocation = FVector::ZeroVector;
        FVector _GoalLocation = FVector::ZeroVector;
        double _EventTimeSeconds = -1.0;
        ECk_PathNetwork_RouteFailReason _PathNetworkFailReason = ECk_PathNetwork_RouteFailReason::None;
        ECk_Nav_PathStatus _NavigationStatus = ECk_Nav_PathStatus::None;
        ECk_Nav_PathFailReason _NavigationFailReason = ECk_Nav_PathFailReason::None;
        bool _HasEvent = false;
        bool _HadPathNetworkFailure = false;
        bool _UsedNavigationFallback = false;

    public:
        CK_PROPERTY_GET(_AgentLocation);
        CK_PROPERTY_GET(_GoalLocation);
        CK_PROPERTY_GET(_EventTimeSeconds);
        CK_PROPERTY_GET(_PathNetworkFailReason);
        CK_PROPERTY_GET(_NavigationStatus);
        CK_PROPERTY_GET(_NavigationFailReason);
        CK_PROPERTY_GET(_HasEvent);
        CK_PROPERTY_GET(_HadPathNetworkFailure);
        CK_PROPERTY_GET(_UsedNavigationFallback);
    };

    // Per-frame displacement staging. Exactly ONE processor consumes it and writes the agent's
    // Transform: FProcessor_CrowdAgent_ConstrainToNavmesh for a grounded agent, and
    // FProcessor_CrowdAgent_ApplyDisplacement3D for a flying one — the two views are disjoint on
    // FTag_CrowdAgent_Flying. A second writer bypasses the navmesh constraint, so every new
    // displacement source accumulates here instead.
    struct CKCROWD_API FFragment_CrowdAgent_PendingDisplacement
    {
    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_PendingDisplacement);

        friend class FProcessor_CrowdAgent_ApplyOffset;
        friend class FProcessor_CrowdAgent_PushApart;
        friend class FProcessor_CrowdAgent_ConstrainToNavmesh;
        friend class FProcessor_CrowdAgent_ApplyDisplacement3D;

    private:
        FVector _Displacement = FVector::ZeroVector;

    public:
        CK_PROPERTY_GET(_Displacement);
    };

    // Stationary-agent nav markup (see CkCrowd/CLAUDE.md). The strong ptr owns the painter
    // object's lifetime — UE GC does not trace fragment members.
    struct CKCROWD_API FFragment_CrowdAgent_NavMarkup
    {
    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_NavMarkup);

        friend class FProcessor_CrowdAgent_StationaryMarkup;
        friend class FProcessor_CrowdAgent_NavMarkup_EndPlay;
        friend class FProcessor_CrowdAgent_PathRefresh;

    private:
        float _StationarySeconds = 0.0f;
        FVector _MarkupLocation = FVector::ZeroVector;
        TStrongObjectPtr<UCk_NavAreaMarkup_UE> _Markup;

        // Stillness is sampled over a window, not instantaneously, so a push-apart shove spike
        // cannot unpaint a standing queue.
        FVector _StillnessSampleLoc = FVector::ZeroVector;
        float _StillnessSampleAccumSec = 0.0f;

        // PathRefresh trigger data: painted XY half-extent, a monotonic CONFIRMATION serial
        // (assigned only after the rebuilt navmesh reports this disc, then compared against each
        // walking agent's path serial), and seconds since paint. Paint alone is not eligibility:
        // dynamic navmesh tiles rebuild asynchronously, so a re-path cannot see the disc yet.
        float _MarkupRadiusUu = 0.0f;
        // The confirm query must reach the floor polys the paint box covered, and the disc centre
        // rides at capsule height.
        float _MarkupVerticalHalfExtentUu = 0.0f;
        float _SecondsSincePaint = 0.0f;
        uint64 _ConfirmationSerial = 0;

        // Set once the rebuilt navmesh actually REPORTS the cost area at the disc's location; tile
        // rebuild latency is unbounded, so the settle timer alone is not proof. Reset on every (re)paint.
        bool _ConfirmedOnMesh = false;

    public:
        CK_PROPERTY_GET(_Markup);
        CK_PROPERTY_GET(_MarkupLocation);
        CK_PROPERTY_GET(_MarkupRadiusUu);
        CK_PROPERTY_GET(_MarkupVerticalHalfExtentUu);
        CK_PROPERTY_GET(_SecondsSincePaint);
        CK_PROPERTY_GET(_ConfirmationSerial);
        CK_PROPERTY_GET(_ConfirmedOnMesh);
    };

    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_NeedsSetup);

    // Mutually exclusive: an agent is either Idle (no goal), PathPending (FindPath enqueued), or
    // Walking (path-ready, steering active).
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_Idle);
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_PathPending);
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_Walking);

    // One-shot guard while an initial PathNetwork failure is being retried through CkNavigation.
    // Cleared when that nav request resolves, or when a new movement episode replaces it.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_PathNetworkFallbackPending);

    // The same one-shot guard for a volumetric path failure being retried through CkNavigation. It is a
    // distinct tag rather than a shared one so the two providers can never consume each other's retry.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_VoxelPathFallbackPending);

    // Nothing stamps this today; the steering views carry TExclude<> for it so a future sleep pass
    // is wire-compatible without retro-fitting every view.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_Asleep);

    // Stamped by Add from the params' _AgentMode and never changed after. Every surface-bound or
    // planar stage of the pipeline excludes it — the navmesh constraint (replaced for these agents by
    // FProcessor_CrowdAgent_ApplyDisplacement3D), StationaryMarkup and PathRefresh (Recast disc math),
    // AvoidanceSample/Separation/PushApart (2D by construction), and the yaw-only FaceAngle (replaced
    // by FProcessor_CrowdAgent_FaceAngle3D). Path-follow, acceleration clamping, the velocity bridge
    // and the integrator are already dimension-agnostic and run unchanged.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_Flying);

    // While present, gameplay code must NOT issue its own MoveTo for this agent, or it fights the
    // goal the debugger took control to issue.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_DebugOverride);

    // The agent has a goal it CANNOT reach. Always co-resident with FTag_CrowdAgent_Idle — the agent
    // HAS stopped, and GoalBlocked only records why and that it still wants the goal.
    // Cleared by any external MoveTo or Stop.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_GoalBlocked);

    // A movement episode ended because no usable path exists or a bounded NoProgress retry
    // budget was exhausted. The active goal remains in PathFollow for diagnostics and an
    // explicit ForceRepath/new-goal wake, but identical ordinary MoveTo requests are inert.
    // Always co-resident with Idle; never co-resident with Walking or PathPending.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_GoalFailedHold);

    // Progress-along-path stall detection plus the escalation ladder that answers it.
    // UPathFollowingComponent's feet-sample centroid ring (PathFollowingComponent.cpp:1556-1608)
    // used to live here; it was blind to an agent sliding laterally along a wall, which is exactly
    // the motion ConstrainToNavmesh's surface walk produces.
    struct CKCROWD_API FFragment_CrowdAgent_BlockDetect
    {
        friend class FProcessor_CrowdAgent_BlockDetect;
        friend class FProcessor_CrowdAgent_BlockedRecheck;
        friend class FProcessor_CrowdAgent_HandleRequests;
        friend class FProcessor_CrowdAgent_OnPathResolved;
        friend class FProcessor_CrowdAgent_PathRefresh;
        friend class ::UCk_Utils_CrowdAgent_UE;

    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_BlockDetect);

    private:
        // Best remaining path distance (agent -> current waypoint, plus the polyline tail) seen
        // since the last improvement worth the epsilon. Only ever lowered while a path is walked,
        // so it IS the windowed minimum; the resets below are what re-seed it.
        float _BestRemainingPathDistanceCm = TNumericLimits<float>::Max();
        float _SecondsWithoutProgress = 0.0f;
        float _SampleAccumulatorSec = 0.0f;
        float _RecheckAccumulatorSec = 0.0f;

        // Internal re-paths already spent on this stall episode. A re-path that then makes real
        // progress zeroes it; an exhausted count is what promotes a stall to a block.
        int32 _StallRepathCount = 0;

        // BlockedRecheck cycles spent re-pathing a NoProgress block. Bounded, because static
        // geometry never clears — unlike an agent-occupied goal, which is a queue wait.
        int32 _BlockedRetryCount = 0;

        // How many bodies deep in a settled pack this agent came to rest. 0 for anything that is
        // not a GoalCrowded block, 1 for an agent stopped behind a body standing on the goal
        // itself, +1 per ring outward. It widens the goal region the NEXT agent back may find its
        // anchor in, so a pack keeps propagating outward at the rate it physically grows.
        int32 _CrowdedGoalDepth = 0;

        ECk_CrowdAgent_BlockedReason _BlockedCause = ECk_CrowdAgent_BlockedReason::GoalOccupied;

        // OnGoalBlocked fires ONCE per blocked episode, not once per re-check.
        bool _BlockedSignalSent = false;

        FCk_Handle _BlockedBy;

        auto
        DoResetProgressWindow() -> void
        {
            _BestRemainingPathDistanceCm = TNumericLimits<float>::Max();
            _SecondsWithoutProgress = 0.0f;
            _SampleAccumulatorSec = 0.0f;
        }

    public:
        CK_PROPERTY_GET(_BlockedBy);
        CK_PROPERTY_GET(_BlockedCause);
        CK_PROPERTY_GET(_CrowdedGoalDepth);
    };

    // --------------------------------------------------------------------------------------------------------------------
    // OnGoalReached fires once when the path-follow cursor crosses the final waypoint within
    // _ActiveArrivalRadius (Walking → Idle) — and only when that waypoint actually is the goal.
    // OnGoalFailed fires once when CkNavigation reports Failed on the path query
    // (PathPending → Idle), or when a PARTIAL path is walked to its end but the goal is
    // unreachable from there (Walking → Idle; see _ActivePathEndsShortOfGoal).
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKCROWD_API,
        CrowdAgent_OnGoalReached,
        FCk_Delegate_CrowdAgent_OnGoalReached,
        FCk_Handle_CrowdAgent);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKCROWD_API,
        CrowdAgent_OnGoalFailed,
        FCk_Delegate_CrowdAgent_OnGoalFailed,
        FCk_Handle_CrowdAgent);

    // OnGoalBlocked fires ONCE when the agent discovers its goal is unreachable. It is NOT a failure:
    // under the default HoldAndRetry policy the agent resumes on its own when the goal clears. The
    // payload names the blocker so gameplay can reassign the NPC instead.
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKCROWD_API,
        CrowdAgent_OnGoalBlocked,
        FCk_Delegate_CrowdAgent_OnGoalBlocked,
        FCk_Handle_CrowdAgent,
        FCk_CrowdAgent_GoalBlockedInfo);
}

// --------------------------------------------------------------------------------------------------------------------
