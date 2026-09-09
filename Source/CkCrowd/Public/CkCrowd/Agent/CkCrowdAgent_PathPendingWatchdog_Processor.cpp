#include "CkCrowdAgent_PathPendingWatchdog_Processor.h"

#include "CkCrowd/Agent/CkCrowdAgent_HandleRequests_Processor.h"
#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_NavGameplayTags.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

#include "CkNavigation/Nav/CkNav_Algorithm.h"
#include "CkNavigation/Nav/CkNav_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkGroundNav/Path/CkGroundNavPath_Fragment.h"

#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_PathPendingWatchdog);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::PathPendingWatchdog"), STAT_CkCrowd_PathPendingWatchdogProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_path_pending_watchdog
{
    static TAutoConsoleVariable<int32> CVar_PendingTimeoutState(
        TEXT("ck.Crowd.Debug.PendingTimeoutState"), 0,
        TEXT("1 writes one GroundNav state snapshot for each Crowd pending-path watchdog timeout. "
             "It does not change timeout, path, or terminal-failure behavior."));

    auto Get_ShouldLogPendingTimeoutState() -> bool
    {
        return CVar_PendingTimeoutState.GetValueOnGameThread() != 0;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_PathPendingWatchdog::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Nav_PathResult& InPathResult,
            const FFragment_CrowdAgent_PathFollow& InPathFollow)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_PathPendingWatchdogProc);

        if (InPathResult.Get_Status() != ECk_Nav_PathStatus::Pending)
        { return; }

        auto NonConstHandle = InHandle;
        const auto ActiveRevision = InPathFollow.Get_ActiveNavigationRequestRevision();

        const auto IsPathPending = InHandle.Has<FTag_CrowdAgent_PathPending>();

        // Walking counts as live: a re-path keeps the agent following its previous corridor while
        // the replacement query is in flight, so it is not orphaned.
        const auto EpisodeIsLive = IsPathPending || InHandle.Has<FTag_CrowdAgent_Walking>();

        CK_ENSURE_IF_NOT(EpisodeIsLive,
            TEXT("CrowdAgent [{}] holds a Pending nav-path slot with no live movement episode. "
                 "Some terminal ended the episode without releasing its query, so every "
                 "Get_PathStatus consumer would be told a query is in flight forever."), InHandle)
        {
            FCk_Nav_Algorithm::AbandonPath(NonConstHandle, ActiveRevision);
            return;
        }

        // Only an agent actually WAITING on a query can be timed out. A Walking agent without
        // PathPending is following a corridor it already has — nothing is wedged, and failing it
        // would both report a healthy agent as failed and write a status no one consumes, since
        // FProcessor_CrowdAgent_OnPathResolved's view requires PathPending to transition it.
        // Restricting the row here is what keeps the terminal below honest.
        if (NOT IsPathPending)
        { return; }

        const auto* Settings = UCk_Utils_Crowd_Settings_UE::Get();
        if (NOT IsValid(Settings))
        { return; }

        // Zero means the slot was parked by something that predates the stamp; there is no
        // measurable age, so bounding it would be a guess.
        const auto PendingSince = InPathResult.Get_PendingSinceSeconds();
        if (PendingSince <= 0.0)
        { return; }

        const auto ElapsedSeconds = FPlatformTime::Seconds() - PendingSince;
        if (ElapsedSeconds < static_cast<double>(Settings->Get_PathPendingTimeoutSeconds()))
        { return; }

        if (ck_crowd_agent_path_pending_watchdog::Get_ShouldLogPendingTimeoutState())
        {
            const auto HasGroundNavCurrent = InHandle.Has<FFragment_GroundNavPath_Current>();
            const auto HasGroundNavResult = InHandle.Has<FFragment_GroundNavPath_Result>();
            auto TimeoutState = FCk_GroundNavPath_TimeoutState{};
            auto HasFreshResult = false;
            auto LastStatus = ECk_GroundNav_PathStatus::InProgress;
            auto LastRevision = 0;

            if (HasGroundNavCurrent)
            { TimeoutState = InHandle.Get<FFragment_GroundNavPath_Current>().Get_TimeoutState(); }

            if (HasGroundNavResult)
            {
                const auto& GroundNavResult = InHandle.Get<FFragment_GroundNavPath_Result>();
                HasFreshResult = GroundNavResult.Get_HasFreshResult();
                LastStatus = GroundNavResult.Get_Result().Get_Status();
                LastRevision = GroundNavResult.Get_Result().Get_RequestRevision();
            }

            ck::crowd::Warning(
                TEXT("[CROWD-PENDING-TIMEOUT-STATE] agent [{}] activeRev [{}] hasCurrent [{}] hasResult [{}] "
                      "pendingRev [{}] queue [{}] searchInFlight [{}] hasBegun [{}] fieldValid [{}] fieldEpoch [{}] searchStatus [{}] "
                      "searchExpansions [{}] searchSpentMs [{}] freshResult [{}] lastStatus [{}] lastRev [{}] "
                      "cellTiming [{}] cellDiscs [{}] cellObbs [{}] edgeBuilds [{}] edgeBuildMs [{}] stepAcross [{}] stepAcrossMs [{}] "
                      "staticRays [{}] staticRayMs [{}] dynamicAdmissions [{}] dynamicAdmissionMs [{}] "
                      "currentPlateBound [{}] goalPlateBound [{}] "
                      "pendingSince [{}] pendingAge [{}]"),
                InHandle, ActiveRevision, HasGroundNavCurrent, HasGroundNavResult, TimeoutState._PendingRequestRevision,
                InHandle.Has<FFragment_GroundNavPath_Requests>(),
                InHandle.Has<FTag_GroundNavPath_SearchInFlight>(), TimeoutState._HasBegun,
                TimeoutState._FieldIsValid, TimeoutState._FieldEpoch, TimeoutState._SearchStatus,
                TimeoutState._SearchExpansionCount, TimeoutState._SearchTimeSpent.Get_Milliseconds(),
                HasFreshResult, LastStatus, LastRevision,
                TimeoutState._CellSearchTiming._IsEnabled,
                TimeoutState._CellSearchTiming._SnapshotDiscCount, TimeoutState._CellSearchTiming._SnapshotObbCount,
                TimeoutState._CellSearchTiming._EdgeBuildCount, TimeoutState._CellSearchTiming._EdgeBuildTime.Get_Milliseconds(),
                TimeoutState._CellSearchTiming._StepAcrossCount, TimeoutState._CellSearchTiming._StepAcrossTime.Get_Milliseconds(),
                TimeoutState._CellSearchTiming._StaticRayCount, TimeoutState._CellSearchTiming._StaticRayTime.Get_Milliseconds(),
                TimeoutState._CellSearchTiming._DynamicAdmissionCount, TimeoutState._CellSearchTiming._DynamicAdmissionTime.Get_Milliseconds(),
                TimeoutState._CellSearchTiming._HasCurrentPlateCostBound,
                TimeoutState._CellSearchTiming._HasGoalPlateCostBound,
                TimeoutState._PendingSince.Get_Seconds(), ElapsedSeconds);

            if (HasGroundNavCurrent)
            {
                InHandle.Get<FFragment_GroundNavPath_Current>().Try_RunStrictCrowdCostTimeoutReplay(
                    UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle),
                    TAG_Nav_Area_Crowd_Agent,
                    TimeoutState._SearchExpansionCount);
            }
        }

        ck::crowd::Warning(
            TEXT("CrowdAgent [{}] path episode via [{}] never answered — failing it after {}s at goal {}. "
                 "The provider parked the shared nav slot and never wrote a terminal status."),
            InHandle, InPathFollow.Get_ActiveProvider(), ElapsedSeconds, InPathFollow.Get_ActiveGoal());

        // A timeout is a terminal, so it releases what the episode acquired — BOTH halves. The
        // CkNavigation query would otherwise sit in the deferral queue and, carrying this same
        // revision, overwrite this failure the moment it became serviceable; and the provider's own
        // corridor would stay parked at Pending forever, reproducing the very orphan this processor
        // exists to catch one layer down — on exactly the PathNetwork/VoxelNav branches that are
        // the reason it exists, since CkNavigation already bounds itself at ck.Nav.MaxDeferralSeconds.
        ck::nav::PurgeInFlightQueriesFor(NonConstHandle);
        FProcessor_CrowdAgent_HandleRequests::DoReleaseProviderQuery(
            InHandle, InPathFollow.Get_ActiveProvider(), ActiveRevision);

        // Then write the status only, keeping the CURRENT revision so the consumer recognises it
        // as its own answer. FProcessor_CrowdAgent_OnPathResolved owns the tag transition and the
        // single OnGoalFailed broadcast for a Failed result; doing either here would duplicate a
        // terminal the caller is bound to.
        FCk_Nav_Algorithm::FailPath(
            NonConstHandle, ECk_Nav_PathFailReason::PendingTimeout, ActiveRevision);
    }
}

// --------------------------------------------------------------------------------------------------------------------
