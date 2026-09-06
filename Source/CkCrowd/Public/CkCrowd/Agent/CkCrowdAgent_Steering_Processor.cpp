#include "CkCrowdAgent_Steering_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/Agent/CkCrowdAgent_HandleRequests_Processor.h"
#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkGroundNav/Bake/CkGroundNav_LinkTypes.h"
#include "CkGroundNav/Path/CkGroundNavPath_Utils.h"

#include "CkNavigation/NavSurface/CkNavSurface_Fragment_Data.h"
#include "CkNavigation/NavSurface/CkNavSurface_Utils.h"

#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_Steering);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::Steering"), STAT_CkCrowd_SteeringProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_steering_processor
{
    // Standing ON the corner, the chord to the next waypoint IS the path segment, and a navmesh
    // raycast running exactly along a boundary edge can report a spurious hit — so the last few
    // uu retire unconditionally rather than risk a corner that can never be given up. Kept well
    // under the lateral offsets that produce the defect this gate exists for: a measured 4.6uu
    // offset beside a corner had a blocked chord, and waving that through is the bug.
    constexpr auto WaypointRetirementNavQueryEpsilonUu = 3.0;

    // The plan decides a crossing's way in at the entry waypoint and carries it onto the exit, so it
    // is only ever Forward or Backward; Bidirectional is the unstamped default and reads as Forward
    // rather than inventing a third neutral value nothing downstream could act on.
    auto Get_NeutralEntryDirection(
        ECk_GroundNav_LinkDirection InDirection) -> ECk_NavSurface_LinkEntryDirection
    {
        return InDirection == ECk_GroundNav_LinkDirection::Backward
            ? ECk_NavSurface_LinkEntryDirection::Backward
            : ECk_NavSurface_LinkEntryDirection::Forward;
    }
}

namespace ck
{
    auto
        FProcessor_CrowdAgent_Steering::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            const FFragment_Nav_PathResult& InPathResult,
            const FFragment_CrowdAgent_SeparationForce& InSeparationForce,
            FFragment_CrowdAgent_DesiredVelocity& InDesired)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_SteeringProc);

        InDesired._CloseGoalStrafeActive = false;

        const auto DoZeroDesiredVelocity = [&]()
        {
            InDesired._Velocity = FVector::ZeroVector;
        };

        if (InPathResult.Get_Status() != ECk_Nav_PathStatus::Ready &&
            InPathResult.Get_Status() != ECk_Nav_PathStatus::Partial)
        {
            DoZeroDesiredVelocity();
            return;
        }

        const auto& Waypoints = InPathResult.Get_Waypoints();
        if (Waypoints.Num() == 0)
        {
            DoZeroDesiredVelocity();
            return;
        }

        const auto CurrentLoc = InTransform.Get_Transform().GetLocation();

        // BOTH retirement tests below are laterally blind — the plane through the corner is
        // unbounded, and proximity says nothing about which side of the corridor the agent stands
        // on. The polyline is FROZEN (Detour re-string-pulls from the agent's current position
        // every frame; this solver cannot), so the chord to the next waypoint is only walkable
        // while the agent is still ON the corridor. Give a corner up from beside a UNavArea_Null
        // hole and steering aims through the hole face, ConstrainToNavmesh eats the whole
        // displacement, and the agent walks on the spot until block detection notices. The corner
        // is on-mesh by construction, so holding it costs one turn.
        //
        // Resolved on the first frame a retirement condition actually fires — at corners, not on
        // every frame of a straight run — so an agent between corners pays nothing for the gate.
        auto GateWorldIsResolved = false;
        auto GateWorld = static_cast<UWorld*>(nullptr);
        auto QueryFilterIsResolved = false;
        auto QueryFilterTag = FGameplayTag{};
        auto QueryFilterOverlay = FCk_Nav_QueryFilterOverlay{};

        const auto Get_WorldForGate = [&]() -> UWorld*
        {
            if (GateWorldIsResolved)
            { return GateWorld; }

            GateWorldIsResolved = true;

            if (UCk_Utils_Crowd_Settings_UE::Get_WaypointRetirementLineOfSight() ==
                ECk_CrowdWaypointRetirementLineOfSightMode::Disabled)
            { return GateWorld; }

            // A flying agent's corridor comes from a volumetric provider, so a surface ray through
            // free space says nothing about it and would strand it on a waypoint it can reach.
            if (InHandle.Has<FTag_CrowdAgent_Flying>())
            { return GateWorld; }

            GateWorld = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            return GateWorld;
        };

        // The plane test does the real work: the minimum turning radius (MaxSpeed / MaxTurnRate,
        // 60cm at defaults) structurally EXCEEDS _WaypointArrivalRadius (25cm), so proximity alone
        // cannot retire a waypoint the agent's turn-limited arc missed and path-follow would then
        // aim BACKWARD at it. The FINAL waypoint is deliberately never retired here (Num() - 1
        // bound) — goal arrival belongs to the final-stop branch, which is what fires OnGoalReached.
        const auto WaypointArrivalRadius = InParams.Get_WaypointArrivalRadius();
        while (InPathFollow._WaypointIndex < Waypoints.Num() - 1)
        {
            const auto& Waypoint = Waypoints[InPathFollow._WaypointIndex];

            const auto DistanceToWaypoint = FVector::Dist(CurrentLoc, Waypoint);
            const auto WithinArrivalRadius = DistanceToWaypoint <= WaypointArrivalRadius;

            const auto CrossedWaypointPlane = [&]() -> bool
            {
                const auto SegmentDir = (Waypoint - InPathFollow.Get_CurrentSegmentStart()).GetSafeNormal();

                // Degenerate segment: fall back to proximity rather than trust a zero normal.
                if (SegmentDir.IsNearlyZero())
                { return false; }

                return FVector::DotProduct(Waypoint - CurrentLoc, SegmentDir) < 0.0;
            }();

            if (NOT (WithinArrivalRadius || CrossedWaypointPlane))
            { break; }

            const auto ChordIsNavigable = [&]() -> bool
            {
                if (DistanceToWaypoint <=
                    ck_crowd_agent_steering_processor::WaypointRetirementNavQueryEpsilonUu)
                { return true; }

                auto* GateWorldForChord = Get_WorldForGate();

                // No world (or the gate switched off) is the pre-gate world, where the navmesh
                // constraint is a pass-through too — nothing here to disagree with.
                if (ck::Is_NOT_Valid(GateWorldForChord))
                { return true; }

                if (NOT QueryFilterIsResolved)
                {
                    QueryFilterIsResolved = true;
                    QueryFilterTag =
                        InPathFollow.Get_ActiveProvider() == ECk_CrowdAgent_PathProvider::PathNetwork
                        ? InParams.Get_NavQueryFilter()
                        : FProcessor_CrowdAgent_HandleRequests::GetPlanQueryFilterTag(
                            InParams, InPathFollow);
                    QueryFilterOverlay = UCk_Utils_CrowdAvoidanceVolume_UE::Get_NavQueryFilterOverlay(
                        InPathFollow.Get_PlanPhase() == ECk_CrowdAgent_PlanPhase::Strict
                            ? ECk_CrowdAvoidanceVolume_QueryPhase::Strict
                            : ECk_CrowdAvoidanceVolume_QueryPhase::Permissive);
                }

                const auto ChordRaycast = FCk_NavSurface_RaycastQuery{
                        CurrentLoc, Waypoints[InPathFollow._WaypointIndex + 1]}
                    .Set_QueryFilter(QueryFilterTag)
                    .Set_QueryFilterOverlay(QueryFilterOverlay);

                return UCk_Utils_NavSurface_UE::Try_SurfaceRaycast(GateWorldForChord, ChordRaycast)
                    .Get_Status() != ECk_NavSurface_QueryStatus::Blocked;
            }();

            if (NOT ChordIsNavigable)
            { break; }

            InPathFollow._CurrentSegmentStart = Waypoint;
            ++InPathFollow._WaypointIndex;
        }

        if (InPathFollow.Get_ProtectedLeadingWaypointCount() > 0 &&
            InPathFollow.Get_WaypointIndex() >=
                InPathFollow.Get_ProtectedLeadingWaypointCount())
        {
            InPathFollow._ProtectedLeadingWaypointCount = 0;
        }

        // The spans describe the GROUND route that stamped them, and no other provider's install
        // clears them, so they are read only while the episode in flight is the ground one. A crossing
        // still recorded from an earlier ground route is ended rather than carried into a corridor
        // that does not contain it.
        const auto DoDriveLinks = [&](int32 InCursor) -> void
        {
            auto LinkHandle = InHandle.ConvertToHandle();

            if (InPathFollow.Get_ActiveProvider() == ECk_CrowdAgent_PathProvider::GroundNav)
            { DoDriveLinkTraversalCursor(LinkHandle, InPathFollow, InCursor, Waypoints.Num()); }
            else
            { DoCancelActiveLinkTraversal(LinkHandle, InPathFollow); }
        };

        DoDriveLinks(InPathFollow.Get_WaypointIndex());

        // Defensive: a single frame's offset can cross the whole path tail. The arrival side
        // effects stay owned by the final-stop branch below.
        if (InPathFollow._WaypointIndex >= Waypoints.Num())
        {
            DoZeroDesiredVelocity();
            return;
        }

        const auto& TargetLoc = Waypoints[InPathFollow._WaypointIndex];
        const auto ToTarget = TargetLoc - CurrentLoc;
        const auto DistanceToNext = ToTarget.Size();
        if (DistanceToNext <= KINDA_SMALL_NUMBER)
        {
            DoZeroDesiredVelocity();
            return;
        }
        const auto Direction = ToTarget / DistanceToNext;

        // Along the POLYLINE, not bird's-eye: a straight-line distance under-estimates whenever
        // the path bends, and the braking ramp below then overshoots.
        auto DistanceToFinal = DistanceToNext;
        for (auto i = InPathFollow._WaypointIndex; i < Waypoints.Num() - 1; ++i)
        {
            DistanceToFinal += FVector::Dist(Waypoints[i], Waypoints[i + 1]);
        }

        // _ActiveArrivalRadius was cached by HandleRequests from the params default or the
        // per-MoveTo override.
        const auto IsTargetingFinal = (InPathFollow._WaypointIndex == Waypoints.Num() - 1);
        if (IsTargetingFinal && DistanceToNext <= InPathFollow.Get_ActiveArrivalRadius())
        {
            InPathFollow._WaypointIndex = Waypoints.Num();
            DoZeroDesiredVelocity();

            // The cursor has just walked off the end of the route, which is what closes a crossing
            // whose exit was the final waypoint — and what abandons one the route stopped on.
            DoDriveLinks(InPathFollow.Get_WaypointIndex());

            auto NonConstHandle = InHandle;
            NonConstHandle.Try_Remove<FTag_CrowdAgent_Walking>();
            NonConstHandle.AddOrGet<FTag_CrowdAgent_Idle>();

            // A partial path's final waypoint is the closest REACHABLE point, not the goal —
            // reaching it is a FAILURE to reach the goal (verdict computed at install).
            if (InPathFollow.Get_ActivePathEndsShortOfGoal())
            {
                NonConstHandle.AddOrGet<FTag_CrowdAgent_GoalFailedHold>();
                ck::crowd::Verbose(
                    TEXT("CrowdAgent [{}] walked its partial path to the end but the goal {} is unreachable from there — reporting OnGoalFailed"),
                    NonConstHandle, InPathFollow.Get_ActiveGoal());

                UUtils_Signal_CrowdAgent_OnGoalFailed::Broadcast(
                    NonConstHandle,
                    MakePayload(NonConstHandle,
                        FCk_CrowdAgent_GoalFailedInfo{
                            ECk_CrowdAgent_GoalFailReason::PathEndsShortOfGoal,
                            ECk_Nav_PathFailReason::None,
                            InPathFollow.Get_StrictStandingCrowdPlanFailed(),
                            InPathFollow.Get_ActiveGoal()}));
                return;
            }

            UUtils_Signal_CrowdAgent_OnGoalReached::Broadcast(
                NonConstHandle,
                MakePayload(NonConstHandle));
            return;
        }

        const auto MaxSpeed = InParams.Get_MaxSpeed();
        const auto MaxAccel = InParams.Get_MaxAcceleration();
        const auto MaxTurnRate = InParams.Get_MaxTurnRate();

        // Inverse of the v²/(2a) stopping distance: the fastest we can still stop in time.
        auto BrakingSpeedCap = MaxSpeed;
        if (MaxAccel > 0.0f)
        {
            BrakingSpeedCap = FMath::Sqrt(2.0f * MaxAccel * DistanceToFinal);
        }

        // Without this, a turning radius (v / MaxTurnRate) larger than the remaining distance makes
        // the agent physically unable to curve onto its goal, so it ORBITS instead. Only bites in
        // the final ~60cm at defaults.
        const auto IsCloseGoalStrafe = InParams.Get_CloseGoalStrafe() == ECk_EnableDisable::Enable
            && InPathResult.Get_Status() == ECk_Nav_PathStatus::Ready
            && IsTargetingFinal
            && DistanceToNext > InPathFollow.Get_ActiveArrivalRadius()
            && DistanceToNext <= InParams.Get_CloseGoalStrafeDistanceUu();
        auto TurnRadiusSpeedCap = MaxSpeed;
        if (MaxTurnRate > 0.0f && NOT IsCloseGoalStrafe)
        {
            TurnRadiusSpeedCap = MaxTurnRate * DistanceToFinal;
        }

        // Raw target velocity — AccelClamp downstream brings it into the per-frame budget.
        const auto NewSpeed = FMath::Min3(MaxSpeed, BrakingSpeedCap, TurnRadiusSpeedCap);

        // INVARIANT: separation may REDIRECT forward motion, never CANCEL it. An opposing
        // component is redirected to pure lateral instead of being subtracted, because a desired
        // velocity that collapses to zero under pressure is a fixed point the AvoidanceSample
        // scoring below then locks in ("stay stopped" becomes the cheapest candidate).
        // See "Invariants this port must preserve" in CkCrowd/CLAUDE.md.
        const auto& RawSeparation = InSeparationForce.Get_Force();
        const auto SeparationAlongPath = FVector::DotProduct(RawSeparation, Direction);

        auto SeparationVec = RawSeparation;
        if (SeparationAlongPath < 0.0)
        {
            const auto LateralComponent = RawSeparation - (SeparationAlongPath * Direction);
            const auto SideDir = LateralComponent.GetSafeNormal();

            if (NOT SideDir.IsNearlyZero())
            {
                SeparationVec = LateralComponent + SideDir * (-SeparationAlongPath);
            }
            else
            {
                const auto LeftPerp = FVector{-Direction.Y, Direction.X, 0.0}.GetSafeNormal();
                if (NOT LeftPerp.IsNearlyZero())
                {
                    SeparationVec = LeftPerp * (-SeparationAlongPath);
                }
                // else: Direction is near-vertical. Leave the raw force rather than invent an axis.
            }
        }

        const auto Combined = Direction * NewSpeed + SeparationVec;
        InDesired._Velocity = Combined.GetClampedToMaxSize(MaxSpeed);
        InDesired._CloseGoalStrafeActive = IsCloseGoalStrafe;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_Steering::
        DoStampLinkSpans(
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            const FCk_GroundNavPath_Result&  InResult)
        -> void
    {
        InPathFollow._LinkSpans = ck::groundnav::Get_LinksOnPath(InResult);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_Steering::
        Get_LinkTraversalCorrelator(
            const FFragment_CrowdAgent_PathFollow& InPathFollow,
            int32                                  InLinkId)
        -> int32
    {
        // _PathSerial alone does not separate two routes: the ground install stamps it from
        // FProcessor_CrowdAgent_PathRefresh::Get_CurrentConfirmationSerial, which is the PROCESS-WIDE
        // stationary-markup confirmation counter, so back-to-back routes with no disc confirmed
        // between them carry the same value. The episode's navigation-request revision is the
        // per-agent monotonic that does separate them, so both are combined and a crossing on the
        // re-planned route can never be ended by the replaced route's Complete.
        const auto Hash = HashCombine(
            HashCombine(
                GetTypeHash(InPathFollow.Get_PathSerial()),
                GetTypeHash(InPathFollow.Get_ActiveNavigationRequestRevision())),
            GetTypeHash(InLinkId));

        // The sign bit is dropped rather than the value truncated: INDEX_NONE is the handshake's "no
        // crossing", and a correlator that landed on it would read as none.
        return static_cast<int32>(Hash & 0x7FFFFFFFu);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_Steering::
        DoCancelActiveLinkTraversal(
            FCk_Handle&                      InHandle,
            FFragment_CrowdAgent_PathFollow& InPathFollow)
        -> void
    {
        // Ahead of the correlator gate, and unconditional. The tag is what licenses
        // ConstrainToNavmesh to stand its surface walk down and to report the body ON the mesh, so a
        // tag left standing beside a correlator that has already gone is a body nothing grounds and
        // nothing reports — the two must never be able to disagree.
        InHandle.Try_Remove<FTag_CrowdAgent_TraversingLink>();

        if (InPathFollow.Get_ActiveLinkCorrelator() == INDEX_NONE)
        { return; }

        UCk_Utils_NavSurface_LinkTraversal_UE::Request_CancelLinkTraversal(InHandle,
            FCk_Request_NavSurface_CancelLinkTraversal{InPathFollow.Get_ActiveLinkCorrelator()}, {});

        InPathFollow._ActiveLinkId = INDEX_NONE;
        InPathFollow._ActiveLinkCorrelator = INDEX_NONE;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_Steering::
        DoDriveLinkTraversalCursor(
            FCk_Handle&                      InHandle,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            int32                            InCursor,
            int32                            InWaypointCount)
        -> void
    {
        // Reconciled against where the cursor stands rather than edge-detected on the advance. The
        // cursor is reset to 0 and skipped forward by more than one waypoint in several places
        // (install normalisation, block detection, path refresh), so an edge test would miss an entry
        // a jump stepped straight over and would never see one the route already stood on at install.
        const FCk_GroundNavPath_LinkSpan* DesiredSpan = nullptr;

        for (const auto& Span : InPathFollow.Get_LinkSpans())
        {
            if (Span.Get_EntryWaypointIndex() == INDEX_NONE ||
                InCursor < Span.Get_EntryWaypointIndex())
            { continue; }

            // An OPEN span is a route that stopped ON the link: it is being crossed until the
            // waypoints run out, and it never gets an exit to be past.
            const auto CursorIsWithinTheSpan = Span.Get_ExitWaypointIndex() == INDEX_NONE
                ? InCursor < InWaypointCount
                : InCursor <= Span.Get_ExitWaypointIndex();

            if (NOT CursorIsWithinTheSpan)
            { continue; }

            DesiredSpan = &Span;
            break;
        }

        // Written out rather than left to the ternary: INDEX_NONE is an unnamed enumerator, and the
        // deduced common type of the two arms would not be int32.
        const auto DesiredCorrelator = DesiredSpan != nullptr
            ? Get_LinkTraversalCorrelator(InPathFollow, DesiredSpan->Get_LinkId())
            : static_cast<int32>(INDEX_NONE);

        if (DesiredCorrelator == InPathFollow.Get_ActiveLinkCorrelator())
        { return; }

        if (InPathFollow.Get_ActiveLinkCorrelator() != INDEX_NONE)
        {
            const auto* ActiveSpan = InPathFollow.Get_LinkSpans().FindByPredicate(
                [&](const FCk_GroundNavPath_LinkSpan& InSpan) -> bool
                {
                    return InSpan.Get_LinkId() == InPathFollow.Get_ActiveLinkId();
                });

            // Completed only where the cursor actually walked off the far end. Everything else — the
            // route ran out on an unfinished span, the cursor rewound onto ground before the entry,
            // the spans no longer name this link — is an ABANDONED crossing, and a listener deciding
            // whether the body arrived is owed that difference.
            const auto CrossingWasWalkedOff = ActiveSpan != nullptr &&
                ActiveSpan->Get_ExitWaypointIndex() != INDEX_NONE &&
                InCursor > ActiveSpan->Get_ExitWaypointIndex();

            if (CrossingWasWalkedOff)
            {
                UCk_Utils_NavSurface_LinkTraversal_UE::Request_CompleteLinkTraversal(InHandle,
                    FCk_Request_NavSurface_CompleteLinkTraversal{
                        InPathFollow.Get_ActiveLinkCorrelator()}, {});

                InPathFollow._ActiveLinkId = INDEX_NONE;
                InPathFollow._ActiveLinkCorrelator = INDEX_NONE;

                InHandle.Try_Remove<FTag_CrowdAgent_TraversingLink>();
            }
            else
            {
                DoCancelActiveLinkTraversal(InHandle, InPathFollow);
            }
        }

        if (DesiredSpan == nullptr)
        { return; }

        UCk_Utils_NavSurface_LinkTraversal_UE::Request_BeginLinkTraversal(InHandle,
            FCk_Request_NavSurface_BeginLinkTraversal{DesiredSpan->Get_LinkId(), DesiredCorrelator}
                .Set_EntryDirection(ck_crowd_agent_steering_processor::Get_NeutralEntryDirection(
                    DesiredSpan->Get_EntryDirection())),
            {});

        InPathFollow._ActiveLinkId = DesiredSpan->Get_LinkId();
        InPathFollow._ActiveLinkCorrelator = DesiredCorrelator;

        InHandle.AddOrGet<FTag_CrowdAgent_TraversingLink>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
