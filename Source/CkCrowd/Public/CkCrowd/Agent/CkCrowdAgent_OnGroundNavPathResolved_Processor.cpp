#include "CkCrowdAgent_OnGroundNavPathResolved_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_GroundNavInstall_Algorithm.h"
#include "CkCrowd/Agent/CkCrowdAgent_PathRefresh_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkNavigation/Nav/CkNav_Algorithm.h"
#include "CkNavigation/Nav/CkNav_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_OnGroundNavPathResolved);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::OnGroundNavPathResolved"), STAT_CkCrowd_OnGroundNavPathResolvedProc, STATGROUP_CkCrowd);

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
                ck::crowd::Log(
                    TEXT("CrowdAgent [{}] dropped a GroundNav path result ({}) with no active "
                         "movement tags — its episode ended without releasing the query"),
                    InHandle, InPathResult.Get_Result().Get_Status());
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
                if (InHandle.Has<FFragment_CrowdAgent_InstalledGroundNavPath>())
                {
                    const auto& Installed = InHandle.Get<FFragment_CrowdAgent_InstalledGroundNavPath>();

                    constexpr auto GoalMatchEpsilonCm = 25.0f;
                    if (Installed.Get_PlannedAgainstEpoch() == Result.Get_PlannedAgainstEpoch() &&
                        FVector::Dist(Installed.Get_GoalLocation(), InPathFollow.Get_ActiveGoal()) <= GoalMatchEpsilonCm)
                    { return; }
                }

                auto NonConstHandle = InHandle;
                auto WaypointsToInstall = Result.Get_Waypoints();
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

                // The route's leading waypoint has no predecessor, so the incoming direction for
                // Steering's plane-crossing retirement comes from where the agent IS at install time.
                InPathFollow._CurrentSegmentStart = InTransform.Get_Transform().GetLocation();

                // A ground route is not planned against Recast, so nothing about it can be invalidated
                // by a stationary-markup disc — adopt the current serial so PathRefresh never re-paths
                // a freshly installed one.
                InPathFollow._PathSerial =
                    FProcessor_CrowdAgent_PathRefresh::Get_CurrentConfirmationSerial();

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

                FCk_Nav_Algorithm::FailPath(NonConstHandle, Verdict._Reason, ActiveRevision);

                // The status write alone drives the crowd's failure sequence — OnPathResolved owns the
                // tag transition and the single OnGoalFailed. The nav-layer signal is separate and is
                // NOT emitted by that processor: CkNavigation broadcasts it beside its own status
                // write, and the install seam broadcasts Nav_OnPathReady beside its own, so a provider
                // that skipped this would leave a GroundNav failure silent to every consumer bound to
                // the shared slot while a Recast failure is not.
                auto BaseHandle = NonConstHandle.ConvertToHandle();
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
