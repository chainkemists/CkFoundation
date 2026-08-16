#include "CkCrowdAgent_DiagNavClip_Processor.h"

#include "CkCore/Format/CkFormat.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkNavigation/Nav/CkNav_Fragment.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_DiagNavClip_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Utils.h"

#include "HAL/IConsoleManager.h"

#include "NavigationSystem.h"
#include "NavigationData.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DiagNavClip);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::DiagNavClip"), STAT_CkCrowd_DiagNavClipProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_diag_nav_clip_processor
{
    static TAutoConsoleVariable<int32> CVarDiagNavClip(
        TEXT("ck.Crowd.DiagNavClip"),
        0,
        TEXT("Report crowd agents whose staged displacement is being eaten before it lands.\n")
        TEXT("0 = off, and the processor returns before any nav query or state write.\n")
        TEXT("1 = on: one [CrowdNavClip] START / HOLD / END line per clipping episode.\n")
        TEXT("ECVF_Default (not Cheat) on purpose: FConsoleManager refuses to apply an ini-deferred value to a\n")
        TEXT("Cheat CVar, and [SystemSettings] is the only switch a manifest-driven Gauntlet run has."),
        ECVF_Default);

    // Mirrors FProcessor_CrowdAgent_ConstrainToNavmesh's own recovery widening, so the diagnostic
    // reports the same on-mesh verdict the clamp acts on.
    constexpr auto RecoveryExtentRadiusMultiplier = 4.0f;

    // Below this the staged displacement is noise and no verdict is meaningful.
    constexpr auto MinStagedUu = 0.5f;

    // The surface walk ate at least this much of the staged displacement.
    constexpr auto ClipFractionThreshold = 0.75f;

    // Below this it plainly did not, which is what makes a stalled agent somebody else's doing.
    constexpr auto HealthyClipFractionThreshold = 0.5f;

    // Fraction of the staged displacement that must actually land, measured against the position
    // the agent held at the previous sample.
    constexpr auto LandedFractionThreshold = 0.15f;

    constexpr auto EpisodeOpenSeconds = 0.5f;
    constexpr auto EpisodeHoldLogSeconds = 2.0f;

    enum class ERayVerdict : uint8
    {
        Unavailable,
        Clear,
        Blocked
    };

    auto Get_RayVerdictName(ERayVerdict InVerdict) -> const TCHAR*
    {
        switch (InVerdict)
        {
            case ERayVerdict::Clear:   return TEXT("clear");
            case ERayVerdict::Blocked: return TEXT("blocked");
            default:                   return TEXT("na");
        }
    }

    // The reflected display names carry spaces ("Path Pending"), which would break the one-token
    // key=value grammar these lines exist to be grepped with.
    auto Get_MovementStateName(ECk_CrowdAgent_MovementState InState) -> const TCHAR*
    {
        switch (InState)
        {
            case ECk_CrowdAgent_MovementState::Idle:        return TEXT("Idle");
            case ECk_CrowdAgent_MovementState::PathPending: return TEXT("PathPending");
            case ECk_CrowdAgent_MovementState::Walking:     return TEXT("Walking");
            default:                                        return TEXT("None");
        }
    }

    auto Get_PathStatusName(ECk_Nav_PathStatus InStatus) -> const TCHAR*
    {
        switch (InStatus)
        {
            case ECk_Nav_PathStatus::Pending: return TEXT("Pending");
            case ECk_Nav_PathStatus::Ready:   return TEXT("Ready");
            case ECk_Nav_PathStatus::Failed:  return TEXT("Failed");
            case ECk_Nav_PathStatus::Partial: return TEXT("Partial");
            default:                          return TEXT("None");
        }
    }

    // Ordered most-specific first: an off-mesh agent has no surface walk to blame, a failed walk
    // has no landed-displacement question to ask, and only once the walk is provably healthy does
    // a stalled agent point at a second Transform writer rather than at the clamp.
    auto Get_ClassName(
        bool InProjectOk,
        bool InRecoveryOk,
        bool InMoveOk,
        bool InExternalHold,
        ERayVerdict InAgentToWaypoint,
        ERayVerdict InSegmentStartToWaypoint) -> const TCHAR*
    {
        if (NOT InProjectOk && NOT InRecoveryOk)
        { return TEXT("OffMesh"); }

        if (NOT InMoveOk)
        { return TEXT("SurfaceWalkFailed"); }

        if (InExternalHold)
        { return TEXT("ExternalHold"); }

        if (InAgentToWaypoint == ERayVerdict::Blocked &&
            InSegmentStartToWaypoint == ERayVerdict::Blocked)
        { return TEXT("PathCrossesBoundary"); }

        if (InAgentToWaypoint == ERayVerdict::Blocked &&
            InSegmentStartToWaypoint == ERayVerdict::Clear)
        { return TEXT("SteeringOffPath"); }

        return TEXT("Other");
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_DiagNavClip::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_PendingDisplacement& InPending,
            const FFragment_CrowdAgent_DesiredVelocity& InDesiredVelocity,
            const FFragment_CrowdAgent_PathFollow& InPathFollow)
        -> void
    {
        using namespace ck_crowd_agent_diag_nav_clip_processor;

        if (CVarDiagNavClip.GetValueOnGameThread() == 0)
        { return; }

        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_DiagNavClipProc);

        const auto MovementState = UCk_Utils_CrowdAgent_UE::Get_MovementState(InHandle);
        if (MovementState != ECk_CrowdAgent_MovementState::Walking &&
            MovementState != ECk_CrowdAgent_MovementState::PathPending)
        { return; }

        const auto Position = InTransform.Get_Transform().GetLocation();
        const auto Displacement = InPending.Get_Displacement();
        const auto StagedUu = static_cast<float>(Displacement.Size2D());

        auto AgentMutable = InHandle;
        auto& State = AgentMutable.AddOrGet<FFragment_CrowdAgent_DiagNavClip>();

        // One frame stale by construction: this is the displacement that landed between the last
        // sample and now, compared against what is being staged for the frame ahead. Irrelevant at
        // the half-second granularity an episode needs, and it is the only reading that survives
        // every Transform writer.
        const auto HasAppliedSample = State._HasLastPosition;
        const auto AppliedUu = HasAppliedSample
            ? static_cast<float>((Position - State._LastPosition).Size2D())
            : 0.0f;

        State._LastPosition = Position;
        State._HasLastPosition = true;

        const auto Dt = static_cast<float>(InDeltaT.Get_Seconds());
        const auto StateName = Get_MovementStateName(MovementState);

        const auto DoCloseEpisode = [&]() -> void
        {
            State._ClippedSeconds = 0.0f;

            if (NOT State._EpisodeOpen)
            { return; }

            ck::crowd::Log(
                TEXT("[CrowdNavClip] END agent={} state={} pos=X={:.1f} Y={:.1f} Z={:.1f} ")
                TEXT("staged={:.2f} applied={:.2f} dur={:.2f} episodes={}"),
                InHandle,
                StateName,
                Position.X, Position.Y, Position.Z,
                StagedUu,
                AppliedUu,
                State._EpisodeSeconds,
                State._EpisodeCount);

            State._EpisodeOpen = false;
            State._EpisodeSeconds = 0.0f;
            State._SecondsSinceHoldLog = 0.0f;
        };

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        const auto NavSys = ck::IsValid(World)
            ? UNavigationSystemV1::GetCurrent(World)
            : static_cast<UNavigationSystemV1*>(nullptr);
        const auto NavData = ck::IsValid(NavSys)
            ? NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate)
            : static_cast<ANavigationData*>(nullptr);

        // A world without nav data passes every displacement through untouched, exactly as the
        // clamp does — there is nothing here to clip.
        if (ck::Is_NOT_Valid(NavData))
        {
            DoCloseEpisode();
            return;
        }

        const auto HorizontalExtent = InParams.Get_Radius();
        const auto VerticalExtent = InParams.Get_Height();
        const auto ProjectionExtent = FVector{HorizontalExtent, HorizontalExtent, VerticalExtent};

        auto StartOnMesh = FNavLocation{};
        const auto ProjectOk = NavSys->ProjectPointToNavigation(Position, StartOnMesh, ProjectionExtent);

        auto RecoveryOk = false;
        if (NOT ProjectOk)
        {
            const auto RecoveryExtent = FVector
            {
                HorizontalExtent * RecoveryExtentRadiusMultiplier,
                HorizontalExtent * RecoveryExtentRadiusMultiplier,
                VerticalExtent
            };

            auto Recovered = FNavLocation{};
            RecoveryOk = NavSys->ProjectPointToNavigation(Position, Recovered, RecoveryExtent);

            if (RecoveryOk)
            { StartOnMesh = Recovered; }
        }

        const auto DesiredTarget = StartOnMesh.Location + FVector{Displacement.X, Displacement.Y, 0.0f};

        auto Constrained = FNavLocation{};
        const auto MoveOk = (ProjectOk || RecoveryOk) &&
            NavData->FindMoveAlongSurface(StartOnMesh, DesiredTarget, Constrained);

        const auto ClipUu = MoveOk
            ? static_cast<float>((DesiredTarget - Constrained.Location).Size2D())
            : 0.0f;
        const auto ClipFrac = StagedUu > 0.0f ? ClipUu / StagedUu : 0.0f;

        const auto SurfaceWalkHealthy = ProjectOk && MoveOk && ClipFrac < HealthyClipFractionThreshold;
        const auto DisplacementDidNotLand = HasAppliedSample && AppliedUu < (LandedFractionThreshold * StagedUu);
        const auto ExternalHold = SurfaceWalkHealthy && DisplacementDidNotLand;

        const auto ClampIsClipping = ClipFrac >= ClipFractionThreshold || NOT MoveOk || NOT ProjectOk;
        const auto IsClipped = StagedUu >= MinStagedUu && (ClampIsClipping || ExternalHold);

        if (NOT IsClipped)
        {
            DoCloseEpisode();
            return;
        }

        State._ClippedSeconds += Dt;

        const auto EpisodeJustOpened = NOT State._EpisodeOpen && State._ClippedSeconds >= EpisodeOpenSeconds;
        if (NOT State._EpisodeOpen && NOT EpisodeJustOpened)
        { return; }

        auto ShouldLogHold = false;
        if (EpisodeJustOpened)
        {
            State._EpisodeOpen = true;
            State._EpisodeSeconds = State._ClippedSeconds;
            State._SecondsSinceHoldLog = 0.0f;
            ++State._EpisodeCount;
        }
        else
        {
            State._EpisodeSeconds += Dt;
            State._SecondsSinceHoldLog += Dt;

            ShouldLogHold = State._SecondsSinceHoldLog >= EpisodeHoldLogSeconds;
            if (ShouldLogHold)
            { State._SecondsSinceHoldLog = 0.0f; }
        }

        if (NOT EpisodeJustOpened && NOT ShouldLogHold)
        { return; }

        const auto WaypointIndex = InPathFollow.Get_WaypointIndex();
        const auto SegmentStart = InPathFollow.Get_CurrentSegmentStart();
        const auto ActiveGoal = InPathFollow.Get_ActiveGoal();

        auto PathStatus = ECk_Nav_PathStatus::None;
        auto WaypointCount = 0;
        auto CurrentWaypoint = FVector::ZeroVector;
        auto HasCurrentWaypoint = false;

        if (InHandle.Has<FFragment_Nav_PathResult>())
        {
            const auto& PathResult = InHandle.Get<FFragment_Nav_PathResult>();
            const auto& Waypoints = PathResult.Get_Waypoints();

            PathStatus = PathResult.Get_Status();
            WaypointCount = Waypoints.Num();

            if (Waypoints.IsValidIndex(WaypointIndex))
            {
                CurrentWaypoint = Waypoints[WaypointIndex];
                HasCurrentWaypoint = true;
            }
        }

        const auto DoRaycast = [&](const FVector& InFrom, const FVector& InTo) -> ERayVerdict
        {
            auto HitLocation = FVector::ZeroVector;
            return NavData->Raycast(InFrom, InTo, HitLocation, FSharedConstNavQueryFilter{})
                ? ERayVerdict::Blocked
                : ERayVerdict::Clear;
        };

        const auto AgentToWaypoint = HasCurrentWaypoint
            ? DoRaycast(Position, CurrentWaypoint)
            : ERayVerdict::Unavailable;
        const auto SegmentStartToWaypoint = HasCurrentWaypoint
            ? DoRaycast(SegmentStart, CurrentWaypoint)
            : ERayVerdict::Unavailable;
        const auto AgentToGoal = NOT ActiveGoal.IsZero()
            ? DoRaycast(Position, ActiveGoal)
            : ERayVerdict::Unavailable;

        const auto DesiredVelocity = InDesiredVelocity.Get_Velocity();

        const auto Detail = ck::Format_UE(
            TEXT("state={} pos=X={:.1f} Y={:.1f} Z={:.1f} staged=X={:.2f} Y={:.2f} (uu) ")
            TEXT("desired=X={:.1f} Y={:.1f} Z={:.1f} (uu/s) ")
            TEXT("projectOk={} recoveryOk={} moveOk={} clipUu={:.2f} clipFrac={:.2f} applied={:.2f} ")
            TEXT("wp={}/{} curWp=X={:.1f} Y={:.1f} Z={:.1f} segStart=X={:.1f} Y={:.1f} Z={:.1f} ")
            TEXT("goal=X={:.1f} Y={:.1f} Z={:.1f} pathStatus={} ")
            TEXT("rayAgentToWp={} raySegStartToWp={} rayAgentToGoal={} class={}"),
            StateName,
            Position.X, Position.Y, Position.Z,
            Displacement.X, Displacement.Y,
            DesiredVelocity.X, DesiredVelocity.Y, DesiredVelocity.Z,
            ProjectOk ? 1 : 0,
            RecoveryOk ? 1 : 0,
            MoveOk ? 1 : 0,
            ClipUu,
            ClipFrac,
            AppliedUu,
            WaypointIndex,
            WaypointCount,
            CurrentWaypoint.X, CurrentWaypoint.Y, CurrentWaypoint.Z,
            SegmentStart.X, SegmentStart.Y, SegmentStart.Z,
            ActiveGoal.X, ActiveGoal.Y, ActiveGoal.Z,
            Get_PathStatusName(PathStatus),
            Get_RayVerdictName(AgentToWaypoint),
            Get_RayVerdictName(SegmentStartToWaypoint),
            Get_RayVerdictName(AgentToGoal),
            Get_ClassName(ProjectOk, RecoveryOk, MoveOk, ExternalHold, AgentToWaypoint, SegmentStartToWaypoint));

        if (EpisodeJustOpened)
        {
            ck::crowd::Log(TEXT("[CrowdNavClip] START agent={} {}"), InHandle, Detail);
            return;
        }

        ck::crowd::Log(
            TEXT("[CrowdNavClip] HOLD agent={} {} dur={:.2f}"),
            InHandle,
            Detail,
            State._EpisodeSeconds);
    }
}

// --------------------------------------------------------------------------------------------------------------------
