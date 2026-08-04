#include "CkCrowdAgent_OnVoxelPathResolved_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_PathFollow_Algorithm.h"
#include "CkCrowd/Agent/CkCrowdAgent_PathRefresh_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkNavigation/Nav/CkNav_Algorithm.h"
#include "CkNavigation/Nav/CkNav_Fragment.h"

#include "CkVoxelNav/Path/CkVoxelNavPath_Utils.h"
#include "CkVoxelNav/Volume/CkVoxelNavVolume_Utils.h"

#include "HAL/PlatformTime.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_OnVoxelPathResolved);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::OnVoxelPathResolved"), STAT_CkCrowd_OnVoxelPathResolvedProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_OnVoxelPathResolved::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_VoxelNavPath_Params& InPathParams,
            const FFragment_VoxelNavPath_Result& InPathResult,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_PathTrouble& InPathTrouble)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_OnVoxelPathResolvedProc);

        // The path result fragment persists after arrival/stop, so only an agent with an active goal
        // may consume it.
        const auto IsPathPending = InHandle.Has<FTag_CrowdAgent_PathPending>();
        const auto IsWalking     = InHandle.Has<FTag_CrowdAgent_Walking>();

        if (NOT IsPathPending && NOT IsWalking)
        { return; }

        // An agent whose volume binding went away is being pathed by CkNavigation instead, and must
        // never be transitioned by a result left over from when it was not.
        if (ck::Is_NOT_Valid(InPathParams.Get_Volume()))
        { return; }

        // A result computed for the PREVIOUS goal must never transition the agent. The result carries no
        // goal of its own to compare against, but the request queue answers the same question exactly:
        // the search runs to completion inside its drain and the queue fragment is removed the moment it
        // empties, so a queue that still exists means this result predates the request in flight.
        if (InHandle.Has<FFragment_VoxelNavPath_Requests>())
        { return; }

        // A repair or a rebake bumps the volume's epoch underneath an agent that is ALREADY walking.
        // Its installed route was planned through free space that may no longer be free, and the Ready
        // branch below correctly refuses to re-install the stale result it still holds — so without
        // this, nothing ever asks for a fresh one and the agent flies the pre-repair route to its end.
        // Asked once per epoch: the request queue answers "in flight" and the recorded epoch answers
        // "already asked", which together survive a replan that comes back Failed.
        if (IsWalking && InHandle.Has<FFragment_CrowdAgent_InstalledVoxelPath>())
        {
            const auto CurrentVolumeEpoch =
                UCk_Utils_VoxelNavVolume_UE::Get_BuildEpoch(InPathParams.Get_Volume());
            const auto& Installed = InHandle.Get<FFragment_CrowdAgent_InstalledVoxelPath>();

            if (CurrentVolumeEpoch != Installed.Get_VolumeEpoch() &&
                CurrentVolumeEpoch != Installed.Get_RequestedAgainstEpoch())
            {
                auto NonConstHandle = InHandle;
                NonConstHandle.Get<FFragment_CrowdAgent_InstalledVoxelPath>()._RequestedAgainstEpoch =
                    CurrentVolumeEpoch;

                auto Path = UCk_Utils_VoxelNavPath_UE::CastChecked(NonConstHandle);
                UCk_Utils_VoxelNavPath_UE::Request_FindPath(
                    Path,
                    FCk_Request_VoxelNavPath_FindPath{
                        InPathParams.Get_Volume(),
                        InTransform.Get_Transform().GetLocation(),
                        InPathFollow.Get_ActiveGoal()},
                    {});

                ck::crowd::Verbose(
                    TEXT("CrowdAgent [{}] walking a route planned against volume epoch {} — the volume "
                         "is now at epoch {}; replanning toward {}"),
                    InHandle,
                    Installed.Get_VolumeEpoch(),
                    CurrentVolumeEpoch,
                    InPathFollow.Get_ActiveGoal());

                return;
            }
        }

        switch (InPathResult.Get_Status())
        {
            case ECk_VoxelNav_PathStatus::Ready:
            {
                if (InHandle.Has<FFragment_CrowdAgent_InstalledVoxelPath>())
                {
                    const auto& Installed = InHandle.Get<FFragment_CrowdAgent_InstalledVoxelPath>();

                    constexpr auto GoalMatchEpsilonCm = 25.0f;
                    if (Installed.Get_VolumeEpoch() == InPathResult.Get_PlannedAgainstEpoch() &&
                        FVector::Dist(Installed.Get_GoalLocation(), InPathFollow.Get_ActiveGoal()) <= GoalMatchEpsilonCm)
                    { return; }
                }

                auto NonConstHandle = InHandle;
                auto WaypointsToInstall = InPathResult.Get_Waypoints();

                FCk_Nav_Algorithm::InstallExternalPath(
                    NonConstHandle,
                    MoveTemp(WaypointsToInstall),
                    InPathFollow.Get_ActiveGoal());
                const auto& InstalledWaypoints =
                    NonConstHandle.Get<FFragment_Nav_PathResult>().Get_Waypoints();

                // Fresh polyline, fresh cursor — on a mid-walk swap the old index may point past
                // the new waypoint array.
                InPathFollow._WaypointIndex = 0;

                // The route's leading waypoint has no predecessor, so the incoming direction for
                // Steering's plane-crossing retirement comes from where the agent IS at install time.
                InPathFollow._CurrentSegmentStart = InTransform.Get_Transform().GetLocation();

                // PathPending installs are normalized by OnPathResolved before they begin walking.
                // A rebake swap is already Walking, so it bypasses that processor: retire any
                // request-time prefix that is behind the agent at install time here. Otherwise the
                // reset cursor aims back at an obsolete lateral projection before turning forward.
                if (IsWalking)
                {
                    const auto SkippedWaypointCount =
                        ck_crowd_agent_path_follow_algorithm::SkipAlreadyPassedLeadingWaypoints(
                            InPathFollow.Get_CurrentSegmentStart(),
                            InstalledWaypoints,
                            InPathFollow._WaypointIndex,
                            InPathFollow._CurrentSegmentStart);

                    if (SkippedWaypointCount > 0)
                    {
                        ck::crowd::Verbose(
                            TEXT("CrowdAgent [{}] voxel route swap skipped {} already-passed "
                                 "leading corner(s)"),
                            InHandle,
                            SkippedWaypointCount);
                    }
                }

                // A volumetric route is not planned against Recast, so nothing about it can be
                // invalidated by a stationary-markup disc — adopt the current serial so PathRefresh
                // never re-paths a freshly installed one.
                InPathFollow._PathSerial =
                    FProcessor_CrowdAgent_PathRefresh::Get_CurrentConfirmationSerial();

                auto& Installed = NonConstHandle.AddOrGet<FFragment_CrowdAgent_InstalledVoxelPath>();
                Installed._GoalLocation = InPathFollow.Get_ActiveGoal();
                Installed._VolumeEpoch = InPathResult.Get_PlannedAgainstEpoch();

                ck::crowd::Verbose(
                    TEXT("CrowdAgent [{}] voxel route ready ({} raw wps, {} installed wps, "
                         "length={}, epoch={}) — installed as nav path"),
                    InHandle,
                    InPathResult.Get_RawWaypointCount(),
                    InstalledWaypoints.Num(),
                    InPathResult.Get_PathLengthUu(),
                    InPathResult.Get_PlannedAgainstEpoch());
                break;
            }
            case ECk_VoxelNav_PathStatus::Failed:
            {
                // Failed while WALKING = a rebake replan came back unroutable; keep walking the
                // already-installed waypoints rather than hard-stopping mid-air.
                if (NOT IsPathPending)
                { break; }

                // The failed result persists. Without this guard the fallback would enqueue a fresh
                // nav request every frame until OnPathResolved consumed one of them.
                if (InHandle.Has<FTag_CrowdAgent_VoxelPathFallbackPending>())
                { break; }

                InPathTrouble._AgentLocation = InTransform.Get_Transform().GetLocation();
                InPathTrouble._GoalLocation = InPathFollow.Get_ActiveGoal();
                InPathTrouble._EventTimeSeconds = FPlatformTime::Seconds();
                InPathTrouble._NavigationStatus = ECk_Nav_PathStatus::Pending;
                InPathTrouble._NavigationFailReason = ECk_Nav_PathFailReason::None;
                InPathTrouble._HasEvent = true;

                auto NonConstHandle = InHandle;
                NonConstHandle.AddOrGet<FTag_CrowdAgent_VoxelPathFallbackPending>();
                FProcessor_CrowdAgent_HandleRequests::Request_NavigationPath(
                    NonConstHandle,
                    InParams,
                    InPathFollow.Get_ActiveGoal());

                ck::crowd::Display(
                    TEXT("CrowdAgent [{}] voxel route failed ({}) — trying CkNavigation fallback"),
                    InHandle,
                    InPathResult.Get_Outcome());
                break;
            }
            case ECk_VoxelNav_PathStatus::None:
            case ECk_VoxelNav_PathStatus::Stale:
            default:
                break;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
