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
        using RequestType             = std::variant<MoveToRequestType, FollowTargetRequestType, StopRequestType, SetMaxSpeedRequestType>;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
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

    // Per-frame displacement staging, consumed solely by FProcessor_CrowdAgent_ConstrainToNavmesh.
    // Nothing else may write a crowd agent's Transform — a second writer bypasses the navmesh
    // constraint, so every new displacement source accumulates here instead.
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

    // Nothing stamps this today; the steering views carry TExclude<> for it so a future sleep pass
    // is wire-compatible without retro-fitting every view.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_Asleep);

    // While present, gameplay code must NOT issue its own MoveTo for this agent, or it fights the
    // goal the debugger took control to issue.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_DebugOverride);

    // The agent has a goal it CANNOT reach. Always co-resident with FTag_CrowdAgent_Idle — the agent
    // HAS stopped, and GoalBlocked only records why and that it still wants the goal.
    // Cleared by any external MoveTo or Stop.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_GoalBlocked);

    // Feet-sample ring mirroring UPathFollowingComponent's block detection
    // (PathFollowingComponent.cpp:1556-1608).
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

        // OnGoalBlocked fires ONCE per blocked episode, not once per re-check.
        bool _BlockedSignalSent = false;

        FCk_Handle _BlockedBy;

    public:
        CK_PROPERTY_GET(_BlockedBy);
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
