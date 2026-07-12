#pragma once

#include "CkCrowd/Agent/CkCrowdAgent_Fragment_Data.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkNavigation/NavAreaMarkup/CkNavAreaMarkup_Utils.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Reflected params alias — entity-side fragment is the same struct as the BP-visible data.
    using FFragment_CrowdAgent_Params         = FCk_Fragment_CrowdAgent_ParamsData;
    using FFragment_CrowdAgent_PathFollow     = FCk_Fragment_CrowdAgent_PathFollowData;
    using FFragment_CrowdAgent_DesiredVelocity = FCk_Fragment_CrowdAgent_DesiredVelocityData;
    using FFragment_CrowdAgent_FaceAngle      = FCk_Fragment_CrowdAgent_FaceAngleData;

    // --------------------------------------------------------------------------------------------------------------------

    // Drained by FProcessor_CrowdAgent_HandleRequests. Variant request type so MoveTo + Stop share
    // a single per-frame queue (no separate fragment per request kind).
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
        using RequestType             = std::variant<MoveToRequestType, FollowTargetRequestType, StopRequestType, SetMaxSpeedRequestType>;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // Follow-mode state: present while the agent's active goal is a LIVE target point
    // (FCk_Request_CrowdAgent_FollowTarget). Carries the originating request (target handle +
    // tuners) and the repath-cadence accumulator. Added by HandleRequests when a FollowTarget
    // request lands; removed by a plain MoveTo, by Stop, or when the target handle dies.
    // Fragment presence IS the FollowTarget processor's view filter.
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

    // Identity of the last path-network corridor installed into this agent's nav-path slot, so the
    // OnRouteResolved bridge can distinguish "fresh plan to consume" from "corridor I already
    // installed" — the corridor fragment persists across MoveTos and network rebuilds. Goal pins
    // which MoveTo the corridor answered; epoch pins which build of the network it was planned
    // against (a rebuild replans the same goal at a new epoch → re-install). Cleared by
    // HandleRequests on every network-routed MoveTo.
    struct CKCROWD_API FFragment_CrowdAgent_InstalledRoute
    {
    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_InstalledRoute);

        friend class FProcessor_CrowdAgent_OnRouteResolved;

    private:
        FVector _GoalLocation = FVector::ZeroVector;
        int32 _NetworkEpoch = 0;

    public:
        CK_PROPERTY_GET(_GoalLocation);
        CK_PROPERTY_GET(_NetworkEpoch);
    };

    // Per-frame displacement staging. The integrator delta (ApplyOffset) and the de-overlap shove
    // (PushApart) both accumulate here instead of writing the Transform directly; the single
    // consumer is FProcessor_CrowdAgent_ConstrainToNavmesh, which walks the accumulated
    // displacement along the navmesh surface (dtCrowd's corridor movePosition — the stage the
    // original port dropped) and enqueues ONE Transform offset. Nothing else may write the
    // Transform of a crowd agent; a second writer would bypass the navmesh constraint.
    struct CKCROWD_API FFragment_CrowdAgent_PendingDisplacement
    {
    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_PendingDisplacement);

        friend class FProcessor_CrowdAgent_ApplyOffset;
        friend class FProcessor_CrowdAgent_PushApart;
        friend class FProcessor_CrowdAgent_ConstrainToNavmesh;

    private:
        FVector _Displacement = FVector::ZeroVector;

    public:
        CK_PROPERTY_GET(_Displacement);
    };

    // Stationary-agent nav markup: after an agent has held still for the configured delay, a
    // cost-area disc (UCk_NavArea_CrowdAgent) is painted under it so pathfinding routes AROUND
    // standing crowds instead of through them. The strong ptr owns the painter object's lifetime
    // (UE GC does not trace fragment members); FProcessor_CrowdAgent_NavMarkup_EndPlay
    // unregisters it on entity teardown.
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

        // Windowed-displacement stillness sampling. "Stationary" is PHYSICAL, not the Idle tag:
        // a blocked/pressing WALKER plugs a corridor exactly like an idle agent, and mutual
        // pressers never go Idle — they must become obstacles too or a congealed clump never
        // dissolves. Windowed (not instantaneous) so a push-apart shove spike doesn't unpaint a
        // standing queue.
        FVector _StillnessSampleLoc = FVector::ZeroVector;
        float _StillnessSampleAccumSec = 0.0f;

        // PathRefresh trigger data: painted XY half-extent, a monotonic paint serial (compared
        // against each walking agent's path serial — only a disc NEWER than the path triggers a
        // refresh), and seconds since paint (dynamic navmesh tiles rebuild asynchronously; the
        // disc must settle before a re-path would actually see it).
        float _MarkupRadiusUu = 0.0f;
        // Painted vertical half-extent — the confirm query below must reach the floor polys the
        // box painted, and the disc centre rides at capsule height (same Z trap as the paint fix).
        float _MarkupVerticalHalfExtentUu = 0.0f;
        float _SecondsSincePaint = 0.0f;
        uint64 _PaintSerial = 0;

        // Set once the rebuilt navmesh actually REPORTS the cost area at the disc's location.
        // Tile rebuild latency is unbounded under churn — a fixed settle timer alone let the
        // one-shot re-path fire against the pre-rebuild mesh, return the same straight path,
        // and burn the serial. Reset on every (re)paint.
        bool _ConfirmedOnMesh = false;

    public:
        CK_PROPERTY_GET(_Markup);
        CK_PROPERTY_GET(_MarkupLocation);
        CK_PROPERTY_GET(_MarkupRadiusUu);
        CK_PROPERTY_GET(_MarkupVerticalHalfExtentUu);
        CK_PROPERTY_GET(_SecondsSincePaint);
        CK_PROPERTY_GET(_PaintSerial);
        CK_PROPERTY_GET(_ConfirmedOnMesh);
    };

    // Marks an agent as needing one-time setup: stamped by Add(), consumed by FProcessor_CrowdAgent_Setup
    // on the next tick, which spawns the agent's probe child entity.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_NeedsSetup);

    // Agent state tags. Mutually exclusive: an agent is either Idle (no goal), PathPending
    // (FindPath enqueued, awaiting result), or Walking (path-ready, steering active). Set by the
    // move-request handler and the path-ready signal handler; consumed as an exclude on steering
    // view membership.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_Idle);
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_PathPending);
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_Walking);

    // Forward-compatible exclude tag — not currently stamped anywhere. Steering / separation /
    // apply-offset processors carry TExclude<FTag_CrowdAgent_Asleep> so a future sleep optimization
    // that stamps it on idle agents is wire-compatible without retro-fitting every view.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_Asleep);

    // Debugger "take control" marker. While present, gameplay code (e.g. the NPC SM) must NOT issue
    // its own MoveTo for this agent, so the debugger's manually-issued goal isn't immediately fought.
    // Set/cleared via UCk_Utils_CrowdAgent_UE::Request_SetDebugOverride; checked via Get_HasDebugOverride.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_DebugOverride);

    // The agent has a goal it CANNOT reach — something is standing on it, or the agent has made no
    // progress for a while. Always co-resident with FTag_CrowdAgent_Idle: the agent has stopped
    // (so the Idle/PathPending/Walking exclusivity above still holds, and AccelClamp's Idle branch
    // ramps it to a halt), and GoalBlocked merely records WHY it stopped and that it still wants the
    // goal. FProcessor_CrowdAgent_BlockedRecheck watches these agents and resumes them when the goal
    // clears. Cleared by any external MoveTo or Stop.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_GoalBlocked);

    // Per-agent state for block detection. The feet-sample ring mirrors UPathFollowingComponent's
    // block detection (PathFollowingComponent.cpp:1556-1608): sample the agent's position on a fixed
    // cadence into a small ring buffer, and if every sample sits within a small radius of their
    // centroid, the agent is going nowhere.
    struct CKCROWD_API FFragment_CrowdAgent_BlockDetect
    {
        friend class FProcessor_CrowdAgent_BlockDetect;
        friend class FProcessor_CrowdAgent_BlockedRecheck;
        friend class FProcessor_CrowdAgent_HandleRequests;
        friend class FProcessor_CrowdAgent_PathRefresh;
        friend class ::UCk_Utils_CrowdAgent_UE;

    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_BlockDetect);

    private:
        TArray<FVector, TInlineAllocator<10>> _FeetSamples;
        int32 _NextSampleIdx = 0;
        float _SampleAccumulatorSec = 0.0f;
        float _RecheckAccumulatorSec = 0.0f;

        // OnGoalBlocked fires ONCE per blocked episode, not once per re-check. Without this an agent
        // that holds, retries, and re-blocks would spam the signal every cadence.
        bool _BlockedSignalSent = false;

        FCk_Handle _BlockedBy;

    public:
        CK_PROPERTY_GET(_BlockedBy);
    };

    // --------------------------------------------------------------------------------------------------------------------
    // Lifecycle signals fired by the steering chain. OnGoalReached fires once when the agent's
    // path-follow cursor crosses the final waypoint within _ActiveArrivalRadius (Walking → Idle).
    // OnGoalFailed fires once when CkNavigation reports Failed status on the path query
    // (PathPending → Idle). Both are bind-rebindable via UCk_Utils_CrowdAgent_UE::BindTo_OnGoal*.
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

    // OnGoalBlocked fires ONCE when the agent discovers its goal is unreachable — something is standing
    // on it, or the agent has stopped making progress. It is NOT a failure: under the default
    // HoldAndRetry policy the agent stops, waits, and resumes on its own when the goal clears. Under
    // FailMove, OnGoalFailed follows immediately after and the caller owns recovery.
    //
    // The payload names the blocker, which is what a gameplay-side queue manager needs to reassign the
    // NPC somewhere else rather than have it stand and wait.
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKCROWD_API,
        CrowdAgent_OnGoalBlocked,
        FCk_Delegate_CrowdAgent_OnGoalBlocked,
        FCk_Handle_CrowdAgent,
        FCk_CrowdAgent_GoalBlockedInfo);
}

// --------------------------------------------------------------------------------------------------------------------
