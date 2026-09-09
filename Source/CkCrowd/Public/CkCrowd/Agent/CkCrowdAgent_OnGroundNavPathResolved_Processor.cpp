#include "CkCrowdAgent_OnGroundNavPathResolved_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_GroundNavInstall_Algorithm.h"
#include "CkCrowd/Agent/CkCrowdAgent_PathRefresh_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_Steering_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkNavigation/Nav/CkNav_Algorithm.h"
#include "CkNavigation/Nav/CkNav_Fragment.h"

#include "CkGroundNav/Path/CkGroundNavPath_Fragment_Data.h"
#include "CkGroundNav/Query/CkGroundNav_Query_DynamicObstacles.h"

#include <HAL/IConsoleManager.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_OnGroundNavPathResolved);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::OnGroundNavPathResolved"), STAT_CkCrowd_OnGroundNavPathResolvedProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_on_ground_nav_path_resolved
{
    static TAutoConsoleVariable<int32> CVar_GroundNavStrictDiagnostics(
        TEXT("ck.Crowd.Debug.GroundNavStrictDiagnostics"), 0,
        TEXT("1 writes visible Crowd GroundNav strict-verdict and dropped-result diagnostics. It does "
             "not change route installation or movement state."));

    auto Get_ShouldLogGroundNavStrictDiagnostics() -> bool
    {
        return CVar_GroundNavStrictDiagnostics.GetValueOnGameThread() != 0;
    }

    struct FConfirmedDisc
    {
        FVector _Center = FVector::ZeroVector;
        float _Radius = 0.0f;
        float _VerticalHalfExtent = 0.0f;
    };

    auto Get_DoesSegmentCrossConfirmedDisc(
        const FVector&        InSegmentStart,
        const FVector&        InSegmentEnd,
        const FConfirmedDisc& InDisc) -> bool
    {
        const auto Discs = TArray<ck::groundnav::FCk_GroundNav_DynamicObstacleDisc>{
            ck::groundnav::FCk_GroundNav_DynamicObstacleDisc{
                InDisc._Center, InDisc._Radius, InDisc._VerticalHalfExtent}};
        const auto Snapshot = ck::groundnav::Try_MakeDynamicObstacleSnapshot(
            Discs,
            TConstArrayView<ck::groundnav::FCk_GroundNav_DynamicObstacleObb>{});
        if (NOT Snapshot.IsSet())
        { return true; }

        const auto UnionEdge = ck::groundnav::Get_DynamicUnionEdge(
            Snapshot.GetValue(), InSegmentStart, InSegmentEnd);
        if (NOT UnionEdge.IsSet())
        { return true; }

        return UnionEdge.GetValue() != ck::groundnav::ECk_GroundNav_DynamicUnionEdge::Clear;
    }

    auto Get_DoesStrictRouteCrossStandingCrowd(
        FCk_Handle_CrowdAgent                        InHandle,
        const ck::FFragment_CrowdAgent_Params&       InParams,
        const ck::FFragment_CrowdAgent_PathFollow&   InPathFollow,
        const FVector&                                InStart,
        const FCk_GroundNavPath_Result&              InResult,
        int32&                                        OutDiscCount) -> bool
    {
        const auto& Waypoints = InResult.Get_Waypoints();
        auto AuthoredLinkSegmentEnds = TSet<int32>{};
        const auto& LinkWaypoints = InResult.Get_LinkWaypoints();

        // Link metadata is an authority boundary: only a complete, ordered Entry -> Exit pair for
        // adjacent raw rows can exempt its authored traversal. A stale or hand-written result must
        // fail closed rather than silently turning an ordinary crowd-crossing leg into a link.
        if (NOT LinkWaypoints.IsEmpty())
        {
            if (LinkWaypoints.Num() % 2 != 0)
            { return true; }

            for (auto MetadataIndex = 0; MetadataIndex < LinkWaypoints.Num(); MetadataIndex += 2)
            {
                const auto& Entry = LinkWaypoints[MetadataIndex];
                const auto& Exit = LinkWaypoints[MetadataIndex + 1];
                const auto IsTraversalDirection =
                    Entry.Get_EntryDirection() == ECk_GroundNav_LinkDirection::Forward ||
                    Entry.Get_EntryDirection() == ECk_GroundNav_LinkDirection::Backward;

                if (Entry.Get_Role() != ECk_GroundNavPath_LinkWaypointRole::Entry ||
                    Exit.Get_Role() != ECk_GroundNavPath_LinkWaypointRole::Exit ||
                    Entry.Get_LinkId() == INDEX_NONE || Entry.Get_LinkId() != Exit.Get_LinkId() ||
                    Entry.Get_EntryDirection() != Exit.Get_EntryDirection() || NOT IsTraversalDirection ||
                    NOT Waypoints.IsValidIndex(Entry.Get_WaypointIndex()) ||
                    NOT Waypoints.IsValidIndex(Exit.Get_WaypointIndex()) ||
                    Exit.Get_WaypointIndex() != Entry.Get_WaypointIndex() + 1 ||
                    (MetadataIndex > 0 &&
                     Entry.Get_WaypointIndex() <= LinkWaypoints[MetadataIndex - 1].Get_WaypointIndex()))
                { return true; }

                AuthoredLinkSegmentEnds.Add(Exit.Get_WaypointIndex());
            }
        }

        auto Discs = TArray<FConfirmedDisc, TInlineAllocator<32>>{};
        const auto GoalExemptionPad = InPathFollow.Get_ActiveArrivalRadius() + InParams.Get_Radius();

        InHandle.View<ck::FFragment_CrowdAgent_NavMarkup>().ForEach(
            [&](FCk_Entity InEntity, const ck::FFragment_CrowdAgent_NavMarkup& InMarkup)
        {
            if (InEntity == InHandle.Get_Entity() ||
                NOT ck::IsValid(InMarkup.Get_Markup()) ||
                NOT InMarkup.Get_ConfirmedOnMesh())
            { return; }

            if (FVector::Dist2D(InMarkup.Get_MarkupLocation(), InPathFollow.Get_ActiveGoal()) <=
                InMarkup.Get_MarkupRadiusUu() + GoalExemptionPad)
            { return; }

            Discs.Add(FConfirmedDisc{
                InMarkup.Get_MarkupLocation(), InMarkup.Get_MarkupRadiusUu(), InMarkup.Get_MarkupVerticalHalfExtentUu()});
        });

        OutDiscCount = Discs.Num();

        auto SegmentStart = InStart;
        for (auto WaypointIndex = 0; WaypointIndex < Waypoints.Num(); ++WaypointIndex)
        {
            const auto& Waypoint = Waypoints[WaypointIndex];

            // This is the exact Entry -> Exit segment certified above. The approach to Entry and the
            // departure from Exit remain ordinary ground movement and are still checked against every
            // standing crowd disc.
            if (AuthoredLinkSegmentEnds.Contains(WaypointIndex))
            {
                SegmentStart = Waypoint;
                continue;
            }

            for (const auto& Disc : Discs)
            {
                // Reuse GroundNav's immutable 3D disc interval implementation. A whole-Z overlap
                // check is insufficient for a sloped segment, whose XY collision can happen outside
                // the disc's vertical interval.
                if (Get_DoesSegmentCrossConfirmedDisc(
                    SegmentStart, Waypoint, FConfirmedDisc{
                        Disc._Center, Disc._Radius + InParams.Get_Radius(), Disc._VerticalHalfExtent}))
                { return true; }
            }
            SegmentStart = Waypoint;
        }

        return false;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_OnGroundNavPathResolved::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_GroundNavPath_Result& InPathResult,
            FFragment_CrowdAgent_PathFollow& InPathFollow)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_OnGroundNavPathResolvedProc);

        // A shadow result is compared elsewhere and must never reach the install seam. FIRST, ahead
        // of the movement-tag gate: the agent is legitimately Idle for a shadow answer, and the
        // dropped-fresh-result Log below would read it as an episode that ended without releasing.
        if (InPathResult.Get_Result().Get_IsShadow() == ECk_EnableDisable::Enable)
        { return; }

        // The path result fragment persists after arrival/stop, so only an agent with an active goal
        // may consume it.
        const auto IsPathPending = InHandle.Has<FTag_CrowdAgent_PathPending>();
        const auto IsWalking     = InHandle.Has<FTag_CrowdAgent_Walking>();

        if (NOT IsPathPending && NOT IsWalking)
        {
            // See the twin guard in OnVoxelPathResolved: a superseded result is expected and silent,
            // an unconsumed CURRENT-revision answer means an episode ended without releasing the query.
            if (InPathResult.Get_HasFreshResult())
            {
                // _HasFreshResult has no consumer-side clear (see the comment above), so the drop below
                // would otherwise log every frame for as long as this stale result sits fresh. Keyed by
                // revision like FFragment_CrowdAgent_ShadowCompared, so a fresh drop is still reported.
                const auto DroppedRevision = InPathResult.Get_Result().Get_RequestRevision();

                auto NonConstHandle = InHandle;
                auto& DroppedSeen = NonConstHandle.AddOrGet<FFragment_CrowdAgent_DroppedResultSeen>();

                if (DroppedSeen.Get_LastDroppedRevision() != DroppedRevision)
                {
                    DroppedSeen._LastDroppedRevision = DroppedRevision;

                    ck::crowd::Log(
                        TEXT("CrowdAgent [{}] dropped a GroundNav path result ({}) with no active "
                             "movement tags — its episode ended without releasing the query"),
                        InHandle, InPathResult.Get_Result().Get_Status());

                    if (ck_crowd_agent_on_ground_nav_path_resolved::Get_ShouldLogGroundNavStrictDiagnostics())
                    {
                        ck::crowd::Log(
                            TEXT("CrowdAgent [{}] dropped GroundNav diagnostic: result rev [{}], active rev [{}], "
                                 "pending [{}], walking [{}], phase [{}], strict-standing [{}]"),
                            InHandle, DroppedRevision, InPathFollow.Get_ActiveNavigationRequestRevision(),
                            IsPathPending, IsWalking, static_cast<int32>(InPathFollow.Get_PlanPhase()),
                            InPathFollow.Get_PlanUsesStrictStandingCrowdFilter());
                    }
                }
            }
            return;
        }

        // A result computed for the PREVIOUS goal must never transition the agent. The request queue
        // answers the same question exactly: a queue that still exists holds a FindPath the drain has
        // not accepted yet, so whatever the slot currently reads predates the request in flight.
        if (InHandle.Has<FFragment_GroundNavPath_Requests>())
        { return; }

        // The slice processor clears this the moment a new episode is parked and sets it only when a
        // terminal verdict is published, so it is true exactly while the stored result belongs to no
        // in-flight search. There is no consumer-side seam to clear it, so an already-acted-on result
        // is recognised by the guards inside the branches below instead.
        if (NOT InPathResult.Get_HasFreshResult())
        { return; }

        const auto& Result = InPathResult.Get_Result();
        const auto ActiveRevision = InPathFollow.Get_ActiveNavigationRequestRevision();

        if (Result.Get_RequestRevision() != ActiveRevision)
        {
            ck::crowd::Verbose(
                TEXT("CrowdAgent [{}] ignored stale GroundNav result rev {} (active {})"),
                InHandle, Result.Get_RequestRevision(), ActiveRevision);
            return;
        }

        const auto& Verdict =
            ck_crowd_agent_ground_nav_install_algorithm::Get_GroundNavVerdict(Result.Get_Status());

        switch (Verdict._Action)
        {
            case ck_crowd_agent_ground_nav_install_algorithm::ECk_CrowdAgent_GroundNavInstallAction::Install:
            {
                // The fresh result stays fresh on every later tick, so an answer already installed is
                // recognised by its epoch and goal and not installed again. Only while nothing is
                // WAITING: a pending episode asked for this plan - a block-detect resume or a permissive
                // retry re-plans the same goal on the same field - and dropping its answer here would
                // park the episode until the watchdog fails it. The same goes for a nav slot a repair
                // parked at Pending with no tag transition ([REBUILD-REPLAN]): that slot is waiting for
                // exactly this answer, and a dropped one would leave it Pending until the next episode,
                // with Steering following a route the slot says is still in flight.
                const auto SlotIsWaiting = InHandle.Has<FFragment_Nav_PathResult>() &&
                    InHandle.Get<FFragment_Nav_PathResult>().Get_Status() == ECk_Nav_PathStatus::Pending;

                if (NOT IsPathPending && NOT SlotIsWaiting &&
                    InHandle.Has<FFragment_CrowdAgent_InstalledGroundNavPath>())
                {
                    const auto& Installed = InHandle.Get<FFragment_CrowdAgent_InstalledGroundNavPath>();

                    constexpr auto GoalMatchEpsilonCm = 25.0f;
                    if (Installed.Get_PlannedAgainstEpoch() == Result.Get_PlannedAgainstEpoch() &&
                        FVector::Dist(Installed.Get_GoalLocation(), InPathFollow.Get_ActiveGoal()) <= GoalMatchEpsilonCm)
                    { return; }
                }

                auto NonConstHandle = InHandle;
                auto WaypointsToInstall = Result.Get_Waypoints();

                const auto NeedsStrictStandingCrowdVerdict =
                    InPathFollow.Get_PlanUsesStrictStandingCrowdFilter() &&
                    InPathFollow.Get_PlanPhase() == ECk_CrowdAgent_PlanPhase::Strict &&
                    InPathFollow.Get_ActiveProvider() == ECk_CrowdAgent_PathProvider::GroundNav;

                if (NeedsStrictStandingCrowdVerdict)
                {
                    auto DiscCount = 0;
                    const auto CrossesStandingCrowd =
                        ck_crowd_agent_on_ground_nav_path_resolved::Get_DoesStrictRouteCrossStandingCrowd(
                            InHandle,
                            InHandle.Get<FFragment_CrowdAgent_Params>(),
                            InPathFollow,
                            InTransform.Get_Transform().GetLocation(),
                            Result,
                            DiscCount);

                    if (ck_crowd_agent_on_ground_nav_path_resolved::Get_ShouldLogGroundNavStrictDiagnostics())
                    {
                        ck::crowd::Log(
                            TEXT("CrowdAgent [{}] strict GroundNav diagnostic: result rev [{}], active rev [{}], "
                                 "pending [{}], walking [{}], discs [{}], crosses [{}]"),
                            InHandle, Result.Get_RequestRevision(), ActiveRevision, IsPathPending, IsWalking,
                            DiscCount, CrossesStandingCrowd);
                    }

                    if (CrossesStandingCrowd)
                    {
                        // A strict result that crosses a confirmed crowd disc, or whose authored-link
                        // metadata cannot prove the exempt traversal, is not installable.
                        ck::crowd::Verbose(
                            TEXT("CrowdAgent [{}] strict GroundNav standing-crowd verdict failed ({} discs)"),
                            InHandle, DiscCount);

                        if (NOT IsPathPending)
                        {
                            NonConstHandle.Try_Remove<FTag_CrowdAgent_Walking>();
                            NonConstHandle.AddOrGet<FTag_CrowdAgent_PathPending>();
                        }

                        FCk_Nav_Algorithm::FailPath(
                            NonConstHandle,
                            ECk_Nav_PathFailReason::FindPathNoPath,
                            ActiveRevision);

                        if (ck_crowd_agent_on_ground_nav_path_resolved::Get_ShouldLogGroundNavStrictDiagnostics())
                        {
                            ck::crowd::Log(
                                TEXT("CrowdAgent [{}] strict GroundNav failure diagnostic: result rev [{}], "
                                     "active rev [{}], pending [{}], walking [{}]"),
                                InHandle, Result.Get_RequestRevision(), ActiveRevision,
                                NonConstHandle.Has<FTag_CrowdAgent_PathPending>(),
                                NonConstHandle.Has<FTag_CrowdAgent_Walking>());
                        }
                        auto BaseHandle = NonConstHandle.ConvertToHandle();
                        FProcessor_CrowdAgent_Steering::DoCancelActiveLinkTraversal(
                            BaseHandle, InPathFollow);
                        UUtils_Signal_Nav_OnPathFailed::Broadcast(BaseHandle, MakePayload(BaseHandle));
                        break;
                    }
                    else
                    {
                        ck::crowd::Verbose(
                            TEXT("CrowdAgent [{}] strict GroundNav standing-crowd verdict crowd-free ({} discs)"),
                            InHandle, DiscCount);
                    }
                }
                InPathFollow._ProtectedLeadingWaypointCount = 0;

                FCk_Nav_Algorithm::InstallExternalPath(
                    NonConstHandle,
                    MoveTemp(WaypointsToInstall),
                    InPathFollow.Get_ActiveGoal(),
                    ActiveRevision,
                    Verdict._InstallAs);

                // Fresh polyline, fresh cursor — on a mid-walk swap the old index may point past
                // the new waypoint array.
                InPathFollow._WaypointIndex = 0;

                // Same reason, one layer up: a crossing in flight belonged to the polyline being
                // replaced, and nothing on the new one bounds it. A swap mid-ladder abandons it.
                auto LinkHandle = NonConstHandle.ConvertToHandle();
                FProcessor_CrowdAgent_Steering::DoCancelActiveLinkTraversal(LinkHandle, InPathFollow);

                // Stamped from the answer's own metadata, so a route no link put a waypoint on stamps
                // an empty array — which is also what clears the previous route's spans.
                FProcessor_CrowdAgent_Steering::DoStampLinkSpans(InPathFollow, Result);

                // The route's leading waypoint has no predecessor, so the incoming direction for
                // Steering's plane-crossing retirement comes from where the agent IS at install time.
                InPathFollow._CurrentSegmentStart = InTransform.Get_Transform().GetLocation();

                // PathRefresh re-paths only for a disc whose confirmation serial is NEWER than the
                // path's, so the serial stamped here is the confirmation serial as of INSTALL time:
                // every disc already confirmed when this route was planned is priced into it, and only
                // one confirmed afterwards is fresh evidence against it. A DISPATCH-time serial is the
                // wrong stamp: every route through a settling crowd would re-path on every later
                // confirmation, an unbounded chase - the in-flight window is the rebuild invalidator's
                // to cover, not this seam's.
                InPathFollow._PathSerial = FProcessor_CrowdAgent_PathRefresh::Get_CurrentConfirmationSerial();

                auto& Installed = NonConstHandle.AddOrGet<FFragment_CrowdAgent_InstalledGroundNavPath>();
                Installed._GoalLocation = InPathFollow.Get_ActiveGoal();
                Installed._PlannedAgainstEpoch = Result.Get_PlannedAgainstEpoch();

                ck::crowd::Verbose(
                    TEXT("CrowdAgent [{}] ground route {} ({} wps, length={}, expansions={}, epoch={}) "
                         "installed as nav path {}"),
                    InHandle,
                    Result.Get_Status(),
                    Result.Get_Waypoints().Num(),
                    Result.Get_LengthUu(),
                    Result.Get_ExpansionCount(),
                    Result.Get_PlannedAgainstEpoch(),
                    Verdict._InstallAs);
                break;
            }
            case ck_crowd_agent_ground_nav_install_algorithm::ECk_CrowdAgent_GroundNavInstallAction::Fail:
            {
                // The failed result stays fresh until the next episode is parked, and the slot keeps
                // reading Failed until OnPathResolved consumes it, which may be a frame away. Without
                // this the failure would be re-stamped and re-broadcast on every frame in between.
                if (InHandle.Has<FFragment_Nav_PathResult>())
                {
                    const auto& NavResult = InHandle.Get<FFragment_Nav_PathResult>();
                    const auto AlreadyFailedThisEpisode =
                        NavResult.Get_Status() == ECk_Nav_PathStatus::Failed &&
                        NavResult.Get_RequestRevision() == ActiveRevision;

                    if (AlreadyFailedThisEpisode)
                    { break; }
                }

                auto NonConstHandle = InHandle;

                // A repair of a LIVE route was dispatched without the PathPending tag so the body could
                // keep walking the stale polyline until the fresh one swapped in (PathRefresh). Its
                // failure ends the episode, and the one processor that owns an ending - the tag
                // transition and the single OnGoalFailed - views PathPending. This is where a failed
                // repair re-enters that view: the same Walking -> PathPending step every other re-plan
                // takes before it dispatches. Without it the body stood Walking on a Failed slot - no
                // hold, no failure reported, and the watchdog reads Walking-without-PathPending as live.
                if (NOT IsPathPending)
                {
                    NonConstHandle.Try_Remove<FTag_CrowdAgent_Walking>();
                    NonConstHandle.AddOrGet<FTag_CrowdAgent_PathPending>();
                }

                FCk_Nav_Algorithm::FailPath(NonConstHandle, Verdict._Reason, ActiveRevision);

                // Nav_OnPathFailed is the per-query nav contract, not the crowd episode's verdict:
                // both providers broadcast it once per FAILED QUERY, so a strict-phase failure the
                // crowd retries with the permissive filter fires it too — that query genuinely
                // failed, even though the episode itself has not ended. OnPathResolved owns the tag
                // transition and the single terminal OnGoalFailed; a consumer that wants the
                // EPISODE's verdict binds CrowdAgent_OnGoalFailed, not this signal.
                auto BaseHandle = NonConstHandle.ConvertToHandle();

                // The episode is over, so a crossing the dropped route was driving is over with it.
                FProcessor_CrowdAgent_Steering::DoCancelActiveLinkTraversal(BaseHandle, InPathFollow);

                UUtils_Signal_Nav_OnPathFailed::Broadcast(BaseHandle, MakePayload(BaseHandle));

                ck::crowd::Display(
                    TEXT("CrowdAgent [{}] ground route failed ({}) — nav path failed with reason {}"),
                    InHandle, Result.Get_Status(), Verdict._Reason);
                break;
            }
            case ck_crowd_agent_ground_nav_install_algorithm::ECk_CrowdAgent_GroundNavInstallAction::Defer:
            {
                // Not terminal: the slot stays parked and the GroundNav episode re-probes itself,
                // force-failing after its own deferral window. The crowd's pending watchdog bounds
                // the wait from this side.
                break;
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
