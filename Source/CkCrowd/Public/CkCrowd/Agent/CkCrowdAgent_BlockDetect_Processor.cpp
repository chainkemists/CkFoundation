#include "CkCrowdAgent_BlockDetect_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkNavigation/Nav/CkNav_Algorithm.h"
#include "CkNavigation/Utils/CkNav_Utils.h"

#include "CkPhysics/Velocity/CkVelocity_Utils.h"

#include "CkPathNetwork/Network/CkPathNetwork_Utils.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_PathRefresh_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_Settled_Algorithm.h"
#include "CkCrowd/Agent/CkCrowdAgent_Utils.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_BlockDetect);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_BlockedRecheck);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::BlockDetect"), STAT_CkCrowd_BlockDetectProc, STATGROUP_CkCrowd);
DECLARE_CYCLE_STAT(TEXT("Crowd::BlockedRecheck"), STAT_CkCrowd_BlockedRecheckProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace ck_crowdagent_blockdetect
    {
        // Returns the blocking neighbour, or an invalid handle when the goal is clear.
        auto Get_GoalBlocker(
            const FVector& InSelfLoc,
            const FVector& InSelfVel,
            const FVector& InFinalWaypoint,
            float InSelfRadius,
            float InArrivalRadius,
            float InStationarySpeedThreshold,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache) -> FCk_Handle
        {
            const auto SelfDistToGoal2D = FVector::Dist2D(InSelfLoc, InFinalWaypoint);

            for (const auto& Nbr : InNeighborCache.Get_Neighbors())
            {
                const auto NbrAbsVel = Nbr.Get_RelativeVelocity() + InSelfVel;
                const auto NeighbourHasSettled = NbrAbsVel.Size2D() < InStationarySpeedThreshold;
                if (NOT NeighbourHasSettled)
                { continue; }

                const auto NbrCentre = InSelfLoc + Nbr.Get_RelativeOffset();

                // Neighbours share our radius (the same approximation the rest of the module makes).
                const auto CombinedRadius = InSelfRadius + InSelfRadius;
                const auto ClosestApproach = CombinedRadius - InArrivalRadius;

                if (ClosestApproach <= 0.0f)
                { continue; }  // arrival tolerance is wide enough to swallow the blocker — not blocked

                const auto NbrDistToGoal2D = FVector::Dist2D(NbrCentre, InFinalWaypoint);

                // Strictly nearer, exactly as the cluster rule requires of an anchor: a body standing
                // FARTHER from the goal than we do cannot be why we stop short of it. Without this the
                // markup-bootstrapped innermost and its own settled dependent hold each other — each
                // inside the other's foreclosure ring — and the goal is never taken.
                if (NbrDistToGoal2D >= SelfDistToGoal2D)
                { continue; }

                if (NbrDistToGoal2D < ClosestApproach)
                { return Nbr.Get_Handle(); }
            }

            return FCk_Handle{};
        }

        // A settled neighbour only blocks when it lies within this cone around the agent's own
        // direction of travel — 0.5 is cos(60 degrees), so the wedge is +/-60. Without it, brushing
        // laterally past someone parked near the goal in a wide corridor reads as a block: the
        // strictly-nearer test alone admits a neighbour up to ~75 degrees off the line to the goal.
        constexpr auto FrontalConeCosine = 0.5f;

        // Below this separation a direction is numerical noise, so the cone test cannot be posed and
        // is treated as passed: standing on the goal, or interpenetrating a neighbour, are both
        // states the other tests already answer for.
        constexpr auto DegenerateDirectionCm = 1.0f;

        // The depth an anchor contributes to whoever settles behind it. An agent that reached the
        // goal, or stopped for any reason other than a crowd, is the FOOT of a chain and
        // contributes 0 — which is what guarantees every chain bottoms out at a body genuinely
        // standing on the destination instead of drifting outward on its own.
        //
        // A GoalCrowded block always stamps depth >= 1, so 0 unambiguously means "not chained" and
        // the two branches of the region test collapse into a single expression at the call site.
        auto Get_AnchorCrowdDepth(
            const FCk_Handle& InAnchor) -> int32
        {
            if (NOT InAnchor.Has<FTag_CrowdAgent_GoalBlocked>())
            { return 0; }

            if (NOT InAnchor.Has<FFragment_CrowdAgent_BlockDetect>())
            { return 0; }

            const auto& AnchorBlockDetect = InAnchor.Get<FFragment_CrowdAgent_BlockDetect>();
            if (AnchorBlockDetect.Get_BlockedCause() != ECk_CrowdAgent_BlockedReason::GoalCrowded)
            { return 0; }

            return AnchorBlockDetect.Get_CrowdedGoalDepth();
        }

        // The strictly-nearer and frontal-cone tests exist to keep hold CONSTRUCTION sound: nearer
        // makes the anchor relation acyclic, the cone proves the anchor is in the way of travel.
        // Neither concern survives into re-validating a hold that already exists — the edges were
        // acyclic when built and a held agent is not travelling — while post-settle PushApart drift
        // of a few centimetres routinely flips both (observed: an agent blocked at 42.0cm relaxed
        // to 38.4cm, ending up NEARER the goal than its own still-standing blocker, and resumed
        // into the pack). So a hold re-validation only asks whether the body still occupies the
        // ground.
        enum class EAnchorTestPurpose
        {
            BlockDecision,
            HoldRevalidation
        };

        // One anchor qualification, two callers — Get_CrowdedGoalBlocker's cache scan and
        // BlockedRecheck's remembered-blocker re-validation — so the two can never drift apart.
        // Returns the anchor's depth when the neighbour qualifies, unset otherwise.
        auto Get_AnchorQualifyingDepth(
            const FVector& InSelfLoc,
            const FVector& InSelfGoal,
            float InSelfRadius,
            float InArrivalRadius,
            float InContactPadCm,
            bool InMarkupAnchorsEnabled,
            EAnchorTestPurpose InPurpose,
            const FCk_Handle& InNeighbourHandle,
            const FVector& InNeighbourCentre) -> TOptional<int32>
        {
            const auto NeighbourAgent = UCk_Utils_CrowdAgent_UE::Cast(InNeighbourHandle);
            if (ck::Is_NOT_Valid(NeighbourAgent))
            { return {}; }

            // A markup anchor can unpaint by moving again, leaving dependents held on an
            // obstruction that has left; BlockedRecheck resumes them within its 1s cadence, so
            // the churn is bounded and deliberately accepted. Being Walking-but-jammed is not a
            // contradiction here: the strictly-nearer test below means such an anchor never
            // blocks on its own dependents.
            const auto NeighbourHasSettled =
                ck_crowd_agent_settled_algorithm::Is_NeighbourSettled(
                    NeighbourAgent, InMarkupAnchorsEnabled);
            if (NOT NeighbourHasSettled)
            { return {}; }

            const auto NeighbourRadius = NeighbourAgent.Has<FFragment_CrowdAgent_Params>()
                ? NeighbourAgent.Get<FFragment_CrowdAgent_Params>().Get_Radius()
                : InSelfRadius;

            const auto NeighbourDistanceToGoal =
                static_cast<float>(FVector::Dist2D(InNeighbourCentre, InSelfGoal));

            // Parked ON the destination — close enough that this agent could not stand where
            // the neighbour does and still be short of its own arrival tolerance — OR already
            // settled as part of the pack occupying it, in which case the allowance grows by
            // one body diameter per ring already stacked up. That is the exact rate at which a
            // physical pile grows outward, and because a chain only extends through agents that
            // are themselves GoalCrowded, every chain bottoms out at a body genuinely standing
            // on the destination.
            const auto AnchorDepth = Get_AnchorCrowdDepth(InNeighbourHandle);
            const auto GoalRegionRadius =
                InSelfRadius + NeighbourRadius + InArrivalRadius + InContactPadCm +
                (static_cast<float>(AnchorDepth) * (InSelfRadius + NeighbourRadius));

            if (NeighbourDistanceToGoal > GoalRegionRadius)
            { return {}; }

            const auto NeighbourDistance =
                static_cast<float>(FVector::Dist2D(InNeighbourCentre, InSelfLoc));
            if (NeighbourDistance > InSelfRadius + NeighbourRadius + InContactPadCm)
            { return {}; }

            if (InPurpose == EAnchorTestPurpose::BlockDecision)
            {
                const auto SelfDistanceToGoal =
                    static_cast<float>(FVector::Dist2D(InSelfLoc, InSelfGoal));
                if (NeighbourDistanceToGoal >= SelfDistanceToGoal)
                { return {}; }

                const auto DirectionsAreMeasurable =
                    SelfDistanceToGoal > DegenerateDirectionCm &&
                    NeighbourDistance > DegenerateDirectionCm;

                if (DirectionsAreMeasurable)
                {
                    const auto ToGoal = FVector2D{InSelfGoal - InSelfLoc}.GetSafeNormal();
                    const auto ToNeighbour = FVector2D{InNeighbourCentre - InSelfLoc}.GetSafeNormal();

                    if (FVector2D::DotProduct(ToGoal, ToNeighbour) < FrontalConeCosine)
                    { return {}; }
                }
            }

            return AnchorDepth;
        }

        // Returns the settled neighbour the agent must come to rest behind, or an invalid handle.
        //
        // Get_GoalBlocker above answers only for whoever is in contact with the GOAL, so in a crowd
        // commanded to one point only the innermost ring ever learns anything. This is the
        // transitive half: contact with a neighbour already settled ON the ground this agent is
        // walking to, standing nearer the goal, and standing in the way, is itself proof the
        // destination cannot be reached.
        //
        // WHY the neighbour is parked there is deliberately not asked — its own goal is never read.
        // A stranger stopped on the spot obstructs exactly as much as a rival for the same
        // destination, and goal identity would miss every crowd whose members were sent to nearby
        // but unequal points. What bounds the rule instead is geometry: the neighbour must be inside
        // the agent's own goal region (condition 2), which is what keeps a picket line parked metres
        // away from anyone's destination inert — such a stranger is not itself GoalCrowded, so it
        // contributes no depth and only ever anchors at the base radius.
        //
        // The strictly-nearer test is what makes the settled-anchor relation acyclic — a settled
        // agent BEHIND self can never stop it short of ground that is still free, and two agents can
        // never hold each other. It is also what terminates the depth chain: depth strictly
        // increases along a strictly-decreasing sequence of distances to the goal.
        auto Get_CrowdedGoalBlocker(
            const FVector& InSelfLoc,
            const FVector& InSelfGoal,
            float InSelfRadius,
            float InArrivalRadius,
            float InContactPadCm,
            bool InMarkupAnchorsEnabled,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache) -> FCk_Handle
        {
            auto BestAnchor = FCk_Handle{};
            auto BestAnchorDepth = TNumericLimits<int32>::Max();

            for (const auto& Nbr : InNeighborCache.Get_Neighbors())
            {
                const auto NeighbourCentre = InSelfLoc + Nbr.Get_RelativeOffset();
                const auto AnchorDepth = Get_AnchorQualifyingDepth(
                    InSelfLoc, InSelfGoal, InSelfRadius, InArrivalRadius, InContactPadCm,
                    InMarkupAnchorsEnabled, EAnchorTestPurpose::BlockDecision,
                    Nbr.Get_Handle(), NeighbourCentre);
                if (NOT AnchorDepth.IsSet())
                { continue; }

                // Shallowest qualifier wins: stamping off a deeper anchor than we had to would
                // inflate the allowance for everyone behind us through a chain we never needed.
                // Neighbours arrive distance-sorted, so an equal-depth tie keeps the nearest.
                if (AnchorDepth.GetValue() >= BestAnchorDepth)
                { continue; }

                BestAnchor = Nbr.Get_Handle();
                BestAnchorDepth = AnchorDepth.GetValue();
            }

            return BestAnchor;
        }

        // BlockedRecheck's scans read the top-N neighbour cache, and a rim holder in a settled
        // pack no longer carries the goal's occupant among its N nearest bodies — the scans go
        // blind exactly when everything is at rest, and the agent resumes into a crowd that never
        // moved (observed: a lone resume half a second into the BunchUp quiet window). The body
        // that actually caused the hold is therefore re-validated FIRST, by direct read; the
        // scans only get to declare the goal clear once that body no longer qualifies under the
        // same geometry that blocked on it.
        auto Does_RememberedBlockerStillObstruct(
            const FVector& InSelfLoc,
            const FVector& InSelfGoal,
            float InSelfRadius,
            float InArrivalRadius,
            float InContactPadCm,
            float InStationarySpeedThreshold,
            bool InMarkupAnchorsEnabled,
            ECk_CrowdAgent_BlockedReason InCause,
            const FCk_Handle& InBlockedBy) -> bool
        {
            if (ck::Is_NOT_Valid(InBlockedBy))
            { return false; }

            const auto BlockerTransform = UCk_Utils_Transform_UE::Cast(InBlockedBy);
            if (ck::Is_NOT_Valid(BlockerTransform))
            { return false; }

            const auto BlockerCentre =
                UCk_Utils_Transform_UE::Get_EntityCurrentLocation(BlockerTransform);

            switch (InCause)
            {
                case ECk_CrowdAgent_BlockedReason::GoalOccupied:
                {
                    // Get_GoalBlocker's per-candidate test, sourced from the handle instead of a
                    // cache row — minus the strictly-nearer guard, which binds the BLOCK decision
                    // only (see EAnchorTestPurpose).
                    const auto BlockerVelocity = UCk_Utils_Velocity_UE::Cast(InBlockedBy);
                    const auto BlockerVel = ck::IsValid(BlockerVelocity)
                        ? UCk_Utils_Velocity_UE::Get_CurrentVelocity(BlockerVelocity)
                        : FVector::ZeroVector;
                    if (BlockerVel.Size2D() >= InStationarySpeedThreshold)
                    { return false; }

                    const auto CombinedRadius = InSelfRadius + InSelfRadius;
                    const auto ClosestApproach = CombinedRadius - InArrivalRadius;
                    if (ClosestApproach <= 0.0f)
                    { return false; }

                    return FVector::Dist2D(BlockerCentre, InSelfGoal) < ClosestApproach;
                }
                case ECk_CrowdAgent_BlockedReason::GoalCrowded:
                {
                    return Get_AnchorQualifyingDepth(
                        InSelfLoc, InSelfGoal, InSelfRadius, InArrivalRadius, InContactPadCm,
                        InMarkupAnchorsEnabled, EAnchorTestPurpose::HoldRevalidation,
                        InBlockedBy, BlockerCentre).IsSet();
                }
                default:
                {
                    return false;
                }
            }
        }

        // What the agent still has to walk: the leg to its current waypoint plus the polyline tail.
        // Lateral sliding along a wall — the exact motion FindMoveAlongSurface produces when an
        // agent presses into one — leaves this untouched, and so does orbiting a corner.
        auto Get_RemainingPathDistance(
            const FVector& InSelfLoc,
            const TArray<FVector>& InWaypoints,
            int32 InWaypointIndex) -> float
        {
            if (InWaypoints.IsEmpty())
            { return 0.0f; }

            const auto FirstIdx = FMath::Clamp(InWaypointIndex, 0, InWaypoints.Num() - 1);

            auto Remaining = FVector::Dist(InSelfLoc, InWaypoints[FirstIdx]);
            for (auto Idx = FirstIdx; Idx < InWaypoints.Num() - 1; ++Idx)
            { Remaining += FVector::Dist(InWaypoints[Idx], InWaypoints[Idx + 1]); }

            return static_cast<float>(Remaining);
        }

        auto Get_DistanceFromCurrentSegment2D(
            const FVector& InSelfLoc,
            const FVector& InSegmentStart,
            const FVector& InSegmentEnd) -> float
        {
            const auto Self2D = FVector2D{InSelfLoc};
            const auto Closest = FMath::ClosestPointOnSegment2D(
                Self2D, FVector2D{InSegmentStart}, FVector2D{InSegmentEnd});

            return static_cast<float>(FVector2D::Distance(Self2D, Closest));
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_BlockDetect::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            const FFragment_Nav_PathResult& InPathResult,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            FFragment_CrowdAgent_BlockDetect& InBlockDetect,
            FFragment_CrowdAgent_DesiredVelocity& InDesired) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_BlockDetectProc);

        if (UCk_Utils_Crowd_Settings_UE::Get_BlockDetectionMode() == ECk_CrowdBlockDetectionMode::Disabled)
        { return; }

        const auto& Waypoints = InPathResult.Get_Waypoints();
        if (Waypoints.IsEmpty())
        { return; }

        const auto SelfLoc = InTransform.Get_Transform().GetLocation();

        // The path's END, not _ActiveGoal — for a Partial path the two differ.
        const auto& FinalWaypoint = Waypoints.Last();
        const auto DistanceToFinal = static_cast<float>(FVector::Dist(SelfLoc, FinalWaypoint));

        const auto* Settings = UCk_Utils_Crowd_Settings_UE::Get();
        if (NOT IsValid(Settings))
        { return; }

        const auto MaxSpeed = InParams.Get_MaxSpeed();
        const auto MaxAccel = InParams.Get_MaxAcceleration();
        const auto SelfRadius = InParams.Get_Radius();
        const auto ArrivalRadius = InPathFollow.Get_ActiveArrivalRadius();

        // ---- Geometric detector (primary) --------------------------------------------------------
        // Only meaningful near the destination — a neighbour parked on a goal 30m away blocks nothing yet.
        const auto BrakingDistance = MaxAccel > 0.0f
            ? (MaxSpeed * MaxSpeed) / (2.0f * MaxAccel)
            : 0.0f;
        constexpr auto EngagementSlackCm = 20.0f;
        const auto EngagementRange =
            (2.0f * SelfRadius) + ArrivalRadius + BrakingDistance + EngagementSlackCm;

        if (DistanceToFinal <= EngagementRange)
        {
            auto SelfVelocity = UCk_Utils_Velocity_UE::Cast(InHandle);
            const auto SelfVel = ck::IsValid(SelfVelocity)
                ? UCk_Utils_Velocity_UE::Get_CurrentVelocity(SelfVelocity)
                : FVector::ZeroVector;

            const auto Blocker = ck_crowdagent_blockdetect::Get_GoalBlocker(
                SelfLoc,
                SelfVel,
                FinalWaypoint,
                SelfRadius,
                ArrivalRadius,
                Settings->Get_BlockedStationarySpeedThreshold(),
                InNeighborCache);

            if (ck::IsValid(Blocker))
            {
                DoBlock(InHandle, InParams, InBlockDetect, InDesired,
                    ECk_CrowdAgent_BlockedReason::GoalOccupied, Blocker, DistanceToFinal);
                return;
            }
        }

        // ---- Sampling cadence --------------------------------------------------------------------
        InBlockDetect._SampleAccumulatorSec += static_cast<float>(InDeltaT.Get_Seconds());
        if (InBlockDetect._SampleAccumulatorSec < Settings->Get_BlockDetectionInterval())
        { return; }

        const auto SecondsSinceLastSample = InBlockDetect._SampleAccumulatorSec;
        InBlockDetect._SampleAccumulatorSec = 0.0f;

        // ---- Cluster detector (propagation) ------------------------------------------------------
        // Deliberately OUTSIDE the engagement window the geometric detector uses: a crowd stacks far
        // deeper than one braking distance from its goal, and the agents this rule exists for are
        // exactly the ones that never come near the goal at all. Contact with a settled obstruction
        // standing on the destination IS the engagement.
        if (Settings->Get_CrowdedGoalBlockMode() == ECk_CrowdCrowdedGoalBlockMode::Enabled)
        {
            const auto CrowdBlocker = ck_crowdagent_blockdetect::Get_CrowdedGoalBlocker(
                SelfLoc,
                InPathFollow.Get_ActiveGoal(),
                SelfRadius,
                ArrivalRadius,
                Settings->Get_CrowdedGoalContactPadCm(),
                Settings->Get_StationaryMarkupMode() == ECk_CrowdStationaryMarkupMode::Enabled,
                InNeighborCache);

            if (ck::IsValid(CrowdBlocker))
            {
                DoBlock(InHandle, InParams, InBlockDetect, InDesired,
                    ECk_CrowdAgent_BlockedReason::GoalCrowded, CrowdBlocker, DistanceToFinal);
                return;
            }
        }

        // ---- Off-path re-path --------------------------------------------------------------------
        // Nothing in the pipeline notices that the agent is no longer near the corridor it is
        // following: a teleport, a save restore or an external shove leaves Steering aiming at a
        // waypoint the installed polyline no longer connects to.
        const auto OffPathThreshold = Settings->Get_BlockDetectionOffPathRepathThresholdCm();
        if (OffPathThreshold > 0.0f)
        {
            const auto TargetIdx =
                FMath::Clamp(InPathFollow.Get_WaypointIndex(), 0, Waypoints.Num() - 1);
            const auto OffPathDistance =
                ck_crowdagent_blockdetect::Get_DistanceFromCurrentSegment2D(
                    SelfLoc, InPathFollow.Get_CurrentSegmentStart(), Waypoints[TargetIdx]);

            if (OffPathDistance > OffPathThreshold)
            {
                // Drift spends a rung of the same ladder a stall does: an agent that keeps being
                // clipped or shoved off its corridor would otherwise re-path here every cadence
                // forever, never reaching an outcome a caller can observe. A one-off teleport
                // still heals for free — the progress branch below refunds the whole budget.
                if (InBlockDetect._StallRepathCount < Settings->Get_BlockDetectionMaxStallRepaths())
                {
                    ++InBlockDetect._StallRepathCount;

                    ck::crowd::Log(
                        TEXT("CrowdAgent [{}] drifted {}cm off its path segment — re-pathing to {}, attempt {} of {}"),
                        InHandle,
                        OffPathDistance,
                        InPathFollow.Get_ActiveGoal(),
                        InBlockDetect._StallRepathCount,
                        Settings->Get_BlockDetectionMaxStallRepaths());

                    DoRepathAtActiveGoal(InHandle, InParams, InPathFollow, InBlockDetect, InDesired);
                    return;
                }

                DoBlock(InHandle, InParams, InBlockDetect, InDesired,
                    ECk_CrowdAgent_BlockedReason::NoProgress, FCk_Handle{}, DistanceToFinal);
                return;
            }
        }

        // ---- No-progress detector (safety net) ---------------------------------------------------
        const auto RemainingPathDistance = ck_crowdagent_blockdetect::Get_RemainingPathDistance(
            SelfLoc, Waypoints, InPathFollow.Get_WaypointIndex());

        // The first sample after a window reset only SEEDS the baseline. Treating it as progress
        // would refund the stall ladder's budget on every re-path — the re-path itself resets the
        // window — and the ladder would never escalate to a block.
        if (InBlockDetect._BestRemainingPathDistanceCm == TNumericLimits<float>::Max())
        {
            InBlockDetect._BestRemainingPathDistanceCm = RemainingPathDistance;
            return;
        }

        const auto MadeProgress = RemainingPathDistance <=
            InBlockDetect._BestRemainingPathDistanceCm - Settings->Get_BlockDetectionProgressEpsilonCm();
        if (MadeProgress)
        {
            InBlockDetect._BestRemainingPathDistanceCm = RemainingPathDistance;
            InBlockDetect._SecondsWithoutProgress = 0.0f;
            InBlockDetect._StallRepathCount = 0;

            // Genuine advance along THIS corridor means the earlier wedge is behind us: a long
            // move that clears one obstruction and later meets an unrelated second one deserves a
            // fresh blocked-retry budget, not a premature fail on spent budget.
            InBlockDetect._BlockedRetryCount = 0;
            return;
        }

        InBlockDetect._SecondsWithoutProgress += SecondsSinceLastSample;
        if (InBlockDetect._SecondsWithoutProgress <
            Settings->Get_BlockDetectionNoProgressWindowSeconds())
        { return; }

        // A frozen polyline planned against geometry that has since changed is the common cause, so
        // spend the re-path budget before declaring a goal unreachable.
        if (InBlockDetect._StallRepathCount < Settings->Get_BlockDetectionMaxStallRepaths())
        {
            ++InBlockDetect._StallRepathCount;

            ck::crowd::Log(
                TEXT("CrowdAgent [{}] made no path progress for {}s — re-path attempt {} of {} to {}"),
                InHandle,
                InBlockDetect._SecondsWithoutProgress,
                InBlockDetect._StallRepathCount,
                Settings->Get_BlockDetectionMaxStallRepaths(),
                InPathFollow.Get_ActiveGoal());

            DoRepathAtActiveGoal(InHandle, InParams, InPathFollow, InBlockDetect, InDesired);
            return;
        }

        DoBlock(InHandle, InParams, InBlockDetect, InDesired,
            ECk_CrowdAgent_BlockedReason::NoProgress, FCk_Handle{}, DistanceToFinal);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_BlockDetect::
        DoRepathAtActiveGoal(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_BlockDetect& InBlockDetect,
            FFragment_CrowdAgent_DesiredVelocity& InDesired) const
        -> void
    {
        auto NonConstHandle = InHandle;

        NonConstHandle.Try_Remove<FTag_CrowdAgent_Walking>();
        NonConstHandle.AddOrGet<FTag_CrowdAgent_PathPending>();

        InPathFollow._WaypointIndex = 0;
        InPathFollow._ProtectedLeadingWaypointCount = 0;
        InBlockDetect.DoResetProgressWindow();

        // A pinned agent must not keep pressing while its re-path is in flight: PathPending drops
        // it from Steering's view but not from AccelClamp's or VelocityBridge's, which would ship
        // the stalled velocity for the whole window. BOTH fields, because AccelClamp's target
        // self-feeds from _Velocity — zeroing that alone buys one MaxAccel step, then plateaus.
        InDesired._Velocity = FVector::ZeroVector;
        InDesired._LastVelocity = FVector::ZeroVector;

        FProcessor_CrowdAgent_HandleRequests::RequestPathForActiveGoal(
            NonConstHandle, InParams, InPathFollow);
    }

    auto
        FProcessor_CrowdAgent_BlockDetect::
        DoBlock(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_BlockDetect& InBlockDetect,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            ECk_CrowdAgent_BlockedReason InReason,
            FCk_Handle InBlocker,
            float InDistanceToGoal) const
        -> void
    {
        auto NonConstHandle = InHandle;

        // Idle is what actually halts the agent (AccelClamp's Idle branch ramps its velocity down);
        // GoalBlocked records that it still WANTS the goal, which is what lets BlockedRecheck resume it.
        NonConstHandle.Try_Remove<FTag_CrowdAgent_Walking>();
        NonConstHandle.AddOrGet<FTag_CrowdAgent_Idle>();
        NonConstHandle.AddOrGet<FTag_CrowdAgent_GoalBlocked>();

        InBlockDetect._BlockedBy = InBlocker;
        InBlockDetect._BlockedCause = InReason;
        InBlockDetect._RecheckAccumulatorSec = 0.0f;

        // Depth chains only through a crowd block, and only off the anchor we actually stopped
        // behind. Every other reason is the foot of a chain, so it resets — a stale depth would
        // widen the goal region for whoever settles behind us on a pack that no longer exists.
        InBlockDetect._CrowdedGoalDepth = InReason == ECk_CrowdAgent_BlockedReason::GoalCrowded
            ? ck_crowdagent_blockdetect::Get_AnchorCrowdDepth(InBlocker) + 1
            : 0;

        // The stall ladder is per Walking stretch: a resumed agent gets its re-path budget back,
        // while BlockedRecheck's own budget is what bounds the episode as a whole.
        InBlockDetect._StallRepathCount = 0;
        InBlockDetect.DoResetProgressWindow();

        // Once per blocked EPISODE, not once per re-check — a holding agent re-blocks every cadence.
        if (NOT InBlockDetect._BlockedSignalSent)
        {
            InBlockDetect._BlockedSignalSent = true;

            UUtils_Signal_CrowdAgent_OnGoalBlocked::Broadcast(
                NonConstHandle,
                MakePayload(NonConstHandle,
                    FCk_CrowdAgent_GoalBlockedInfo{InReason, InBlocker, InDistanceToGoal}));

            ck::crowd::Verbose(TEXT("CrowdAgent [{}] goal BLOCKED ({}) at {}cm — blocker [{}]"),
                InHandle, InReason, InDistanceToGoal, InBlocker);
        }

        if (InParams.Get_BlockedPolicy() == ECk_CrowdAgent_BlockedPolicy::FailMove)
        {
            NonConstHandle.Try_Remove<FTag_CrowdAgent_GoalBlocked>();
            NonConstHandle.AddOrGet<FTag_CrowdAgent_GoalFailedHold>();

            // A failed move is not a crowd hold, so it anchors nobody.
            InBlockDetect._CrowdedGoalDepth = 0;

            InDesired._Velocity = FVector::ZeroVector;
            InDesired._LastVelocity = FVector::ZeroVector;

            const auto& PathFollow = NonConstHandle.Get<FFragment_CrowdAgent_PathFollow>();
            UUtils_Signal_CrowdAgent_OnGoalFailed::Broadcast(
                NonConstHandle,
                MakePayload(NonConstHandle,
                    FCk_CrowdAgent_GoalFailedInfo{
                        ECk_CrowdAgent_GoalFailReason::BlockedFailMovePolicy,
                        ECk_Nav_PathFailReason::None,
                        PathFollow.Get_StrictPlanFailed(),
                        PathFollow.Get_ActiveGoal()}));
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_BlockedRecheck::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_BlockDetect& InBlockDetect,
            FFragment_CrowdAgent_DesiredVelocity& InDesired) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_BlockedRecheckProc);

        const auto* Settings = UCk_Utils_Crowd_Settings_UE::Get();
        if (NOT IsValid(Settings))
        { return; }

        InBlockDetect._RecheckAccumulatorSec += static_cast<float>(InDeltaT.Get_Seconds());
        if (InBlockDetect._RecheckAccumulatorSec < Settings->Get_BlockedRecheckInterval())
        { return; }

        InBlockDetect._RecheckAccumulatorSec = 0.0f;

        const auto SelfLoc = InTransform.Get_Transform().GetLocation();

        if (ck_crowdagent_blockdetect::Does_RememberedBlockerStillObstruct(
                SelfLoc,
                InPathFollow.Get_ActiveGoal(),
                InParams.Get_Radius(),
                InPathFollow.Get_ActiveArrivalRadius(),
                Settings->Get_CrowdedGoalContactPadCm(),
                Settings->Get_BlockedStationarySpeedThreshold(),
                Settings->Get_StationaryMarkupMode() == ECk_CrowdStationaryMarkupMode::Enabled,
                InBlockDetect.Get_BlockedCause(),
                InBlockDetect.Get_BlockedBy()))
        { return; }

        // A held agent is stationary, so neighbour relative velocity IS absolute velocity — hence
        // the zero self-velocity argument.
        const auto Blocker = ck_crowdagent_blockdetect::Get_GoalBlocker(
            SelfLoc,
            FVector::ZeroVector,
            InPathFollow.Get_ActiveGoal(),
            InParams.Get_Radius(),
            InPathFollow.Get_ActiveArrivalRadius(),
            Settings->Get_BlockedStationarySpeedThreshold(),
            InNeighborCache);

        if (ck::IsValid(Blocker))
        { return; }  // still taken — keep holding

        // Symmetric with BlockDetect's cluster rule, and load-bearing: an agent whose anchor is a
        // settled NEIGHBOUR rather than the goal itself has nothing here that can see the pack, so
        // it would resume every cadence, walk back into it, re-block, and — on a NoProgress cause —
        // spend the whole retry budget doing it before failing outright. Holding costs no retry:
        // the pack is made of agents and will drain, exactly like an occupied goal.
        if (Settings->Get_CrowdedGoalBlockMode() == ECk_CrowdCrowdedGoalBlockMode::Enabled)
        {
            const auto CrowdBlocker = ck_crowdagent_blockdetect::Get_CrowdedGoalBlocker(
                SelfLoc,
                InPathFollow.Get_ActiveGoal(),
                InParams.Get_Radius(),
                InPathFollow.Get_ActiveArrivalRadius(),
                Settings->Get_CrowdedGoalContactPadCm(),
                Settings->Get_StationaryMarkupMode() == ECk_CrowdStationaryMarkupMode::Enabled,
                InNeighborCache);

            if (ck::IsValid(CrowdBlocker))
            { return; }
        }

        // A GoalOccupied block holds for as long as the blocker stands there: that IS the queue
        // behaviour callers depend on, and it is not a failure. A NoProgress block has no blocker
        // to wait out — the obstruction is static — so each re-check spends one bounded attempt
        // instead of resuming into the same wedge forever.
        if (InBlockDetect.Get_BlockedCause() == ECk_CrowdAgent_BlockedReason::NoProgress)
        {
            if (InBlockDetect._BlockedRetryCount >= Settings->Get_BlockedMaxRetries())
            {
                DoFailMove(InHandle, InPathFollow, InBlockDetect, InDesired);
                return;
            }

            ++InBlockDetect._BlockedRetryCount;

            // This re-check IS the retry's re-path attempt, so the resumed walk gets no fresh
            // stall ladder on top — re-granting it would multiply every retry cycle by the full
            // ladder (each rung a wasted pathfind at a goal this re-check just probed) and
            // stretch a bounded failure into tens of seconds. One window of no progress after
            // this resume re-blocks directly; genuine progress still refunds everything.
            InBlockDetect._StallRepathCount = Settings->Get_BlockDetectionMaxStallRepaths();
        }

        // The goal is free: a FULL re-path, not a resumed cursor — the agent may have been shoved
        // around while it held.
        auto NonConstHandle = InHandle;

        NonConstHandle.Try_Remove<FTag_CrowdAgent_GoalBlocked>();
        NonConstHandle.Try_Remove<FTag_CrowdAgent_Idle>();
        NonConstHandle.AddOrGet<FTag_CrowdAgent_PathPending>();

        InPathFollow._WaypointIndex = 0;

        InBlockDetect._BlockedBy = FCk_Handle{};
        InBlockDetect._CrowdedGoalDepth = 0;
        InBlockDetect.DoResetProgressWindow();

        // _BlockedSignalSent is deliberately NOT reset: same goal means same episode. Only an
        // external MoveTo/Stop starts a new one.

        const auto Goal = InPathFollow.Get_ActiveGoal();
        FProcessor_CrowdAgent_HandleRequests::AdvanceNavigationRequestRevision(InPathFollow);

        if (UCk_Utils_PathNetworkFollower_UE::Has(NonConstHandle))
        {
            FCk_Nav_Algorithm::MarkPathPending(
                NonConstHandle, InPathFollow.Get_ActiveNavigationRequestRevision());
            NonConstHandle.Try_Remove<FFragment_CrowdAgent_InstalledRoute>();

            auto Follower = UCk_Utils_PathNetworkFollower_UE::CastChecked(NonConstHandle);
            auto Request = FCk_Request_PathNetworkFollower_FindRoute{Goal};
            Request.Set_NavQueryFilter(InParams.Get_NavQueryFilter());
            Request.Set_RequestRevision(InPathFollow.Get_ActiveNavigationRequestRevision());
            UCk_Utils_PathNetworkFollower_UE::Request_FindRoute(Follower, Request, {});
        }
        else
        {
            InPathFollow._ProtectedLeadingWaypointCount = 0;
            // Park the slot at Pending so OnPathResolved can't consume the PREVIOUS Ready corridor.
            FCk_Nav_Algorithm::MarkPathPending(
                NonConstHandle, InPathFollow.Get_ActiveNavigationRequestRevision());

            // The resume itself is the new evidence — the recheck just saw the blocker gone — so
            // the strict phase gets a fresh attempt.
            InPathFollow._StrictPlanFailed = false;

            auto Request = FCk_Request_Nav_FindPath{Goal};
            FProcessor_CrowdAgent_HandleRequests::ApplyPlanPhase(InParams, InPathFollow, Request);
            Request.Set_RequestRevision(InPathFollow.Get_ActiveNavigationRequestRevision());

            // A held agent may have been shoved inside painted stationary markup — resuming from
            // inside the band would pick "through". See Get_EscapedQueryStart.
            const auto Escaped = FProcessor_CrowdAgent_PathRefresh::Get_EscapedQueryStart(
                NonConstHandle, InHandle.Get_Entity(), SelfLoc, Goal, InParams.Get_Radius());
            if (Escaped.IsSet())
            {
                Request.Set_StartOverride(ECk_EnableDisable::Enable)
                       .Set_StartOverrideLocation(*Escaped);
            }

            UCk_Utils_Nav_UE::Request_FindPath(NonConstHandle, Request, {});
        }

        ck::crowd::Verbose(TEXT("CrowdAgent [{}] goal CLEARED — resuming to {}"), InHandle, Goal);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_BlockedRecheck::
        DoFailMove(
            HandleType InHandle,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_BlockDetect& InBlockDetect,
            FFragment_CrowdAgent_DesiredVelocity& InDesired) const
        -> void
    {
        auto NonConstHandle = InHandle;

        // GoalBlocked goes with the hold it ends; the agent is already Idle from DoBlock.
        NonConstHandle.Try_Remove<FTag_CrowdAgent_GoalBlocked>();
        NonConstHandle.AddOrGet<FTag_CrowdAgent_Idle>();
        NonConstHandle.AddOrGet<FTag_CrowdAgent_GoalFailedHold>();

        InPathFollow._WaypointIndex = 0;
        InPathFollow._ProtectedLeadingWaypointCount = 0;

        InBlockDetect._BlockedBy = FCk_Handle{};
        InBlockDetect._CrowdedGoalDepth = 0;
        InBlockDetect._RecheckAccumulatorSec = 0.0f;
        InBlockDetect.DoResetProgressWindow();

        InDesired._Velocity = FVector::ZeroVector;
        InDesired._LastVelocity = FVector::ZeroVector;

        // Log, not Warning: an unreachable goal is a legitimate gameplay outcome (a player can
        // wall off any destination), and the caller is informed through OnGoalFailed.
        ck::crowd::Log(
            TEXT("CrowdAgent [{}] made no progress toward {} across {} re-path attempts — reporting OnGoalFailed"),
            InHandle, InPathFollow.Get_ActiveGoal(), InBlockDetect._BlockedRetryCount);

        UUtils_Signal_CrowdAgent_OnGoalFailed::Broadcast(
            NonConstHandle,
            MakePayload(NonConstHandle,
                FCk_CrowdAgent_GoalFailedInfo{
                    ECk_CrowdAgent_GoalFailReason::NoProgressRetriesExhausted,
                    ECk_Nav_PathFailReason::None,
                    InPathFollow.Get_StrictPlanFailed(),
                    InPathFollow.Get_ActiveGoal()}));
    }
}

// --------------------------------------------------------------------------------------------------------------------
