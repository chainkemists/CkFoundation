#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_HandleRequests_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_OnGroundNavPathResolved_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_OnPathResolved_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_OnRouteResolved_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_OnVoxelPathResolved_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_StationaryMarkup_Processor.h"
#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Processor.h"
#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Utils.h"

#include "CkNavigation/Nav/CkNav_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Mid-walk path refresh — the second half of StationaryMarkup. The cost discs only influence
    // FindPath at PLAN time; a path computed before a crowd formed is a frozen polyline the agent
    // follows INTO the crowd (UE's own UPathFollowingComponent re-paths when the navmesh under its
    // path rebuilds; the dtCrowd port dropped that half of the mechanism). Each tick, a Walking
    // agent whose REMAINING path passes through a disc confirmed AFTER its path was installed is
    // re-pathed at its own goal (BlockedRecheck's resume dance), so the fresh plan sees the discs
    // and detours. Assigning the serial at confirmation, not paint, preserves the invalidation
    // when a route installs during the asynchronous paint-to-navmesh-rebuild window.
    //
    // One-shot per (path, disc-set): a path serial older than every eligible disc early-outs on a
    // single compare, and a clean scan fast-forwards the serial — the path and the discs are both
    // static, so a miss now is a miss forever. A path that legitimately chose to PAY a disc's cost
    // and cross (cost, never a hole) is therefore never re-planned for that disc again. Because
    // the one-shot is precious, a disc only becomes eligible once the REBUILT navmesh actually
    // reports the cost area at its location (tile rebuild is async and unbounded under churn —
    // a timer-gated refresh fired pre-rebake, got the same straight path, and burned the serial).
    //
    // Server-only in effect: only the server paints discs, so the per-tick gather is empty on
    // clients and every agent early-outs.
    //
    // A flying agent is excluded: its route came from a volumetric provider, so no navmesh cost disc
    // can invalidate it, and the re-path this processor issues would be planned against Recast.
    class CKCROWD_API FProcessor_CrowdAgent_PathRefresh : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_PathRefresh,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            FTag_CrowdAgent_Walking,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_Nav_PathResult>,
            ck::TReadWrite<FFragment_CrowdAgent_PathFollow>,
            ck::TReadWrite<FFragment_CrowdAgent_BlockDetect>,
            TExclude<FTag_CrowdAgent_Asleep>,
            TExclude<FTag_CrowdAgent_Flying>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<
            FProcessor_CrowdAgent_StationaryMarkup,
            FProcessor_CrowdAvoidanceVolume_Monitor,
            FProcessor_CrowdAgent_HandleRequests,
            FProcessor_CrowdAgent_OnPathResolved,
            FProcessor_CrowdAgent_OnGroundNavPathResolved,
            FProcessor_CrowdAgent_OnRouteResolved,
            FProcessor_CrowdAgent_OnVoxelPathResolved>;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(FCk_Time InDeltaT) -> void;

        // Latest process-wide serial assigned to a markup only after Recast confirms its cost area.
        // Path installers stamp this value; any later confirmation remains strictly newer.
        static auto
        Get_CurrentConfirmationSerial() -> uint64;

        // Shared by every nav-area producer so confirmation serials remain globally monotonic.
        static auto
        IssueConfirmationSerial() -> uint64;

        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_Nav_PathResult& InPathResult,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_BlockDetect& InBlockDetect) const -> void;

        // For an agent standing inside a painted cost disc whose goal is outside it, returns a
        // query start just outside the band — planning from its feet re-picks "through", since
        // the toll is per distance crossed. UNSET means "plan from the real location", which is
        // the correct answer for every other case, not a failure.
        static auto
        Get_EscapedQueryStart(
            FCk_Handle InAnyWorldHandle,
            FCk_Entity InSelfEntity,
            const FVector& InSelfLocation,
            const FVector& InGoal,
            float InAgentRadius) -> TOptional<FVector>;

        // Converts a geometry-only escape point into a physically followable Recast prefix from
        // the agent's real location. Returns false without mutating OutWaypoints when either end
        // cannot be projected, no complete path exists, or the projected endpoint falls back
        // inside another agent's expanded painted markup.
        static auto
        Try_BuildStationaryMarkupEscapePath(
            FCk_Handle InAnyWorldHandle,
            FCk_Entity InSelfEntity,
            const FVector& InSelfLocation,
            const FVector& InEscapedLocation,
            const FFragment_CrowdAgent_Params& InParams,
            ECk_CrowdAvoidanceVolume_QueryPhase InVolumeQueryPhase,
            TArray<FVector>& OutWaypoints) -> bool;

        // PathNetwork corridors are preferred geometry, not hard movement boundaries. When a
        // resolved corridor crosses confirmed stationary-agent markup, replace only the affected
        // span with a Recast path computed under the agent's own query filter. The untouched
        // prefix/suffix keep the agent on the authored route and make it rejoin immediately after
        // clearing the standing crowd. Returns false without mutating OutWaypoints when no splice
        // is needed or no valid detour exists.
        static auto
        Try_BuildStationaryMarkupDetour(
            FCk_Handle InAnyWorldHandle,
            FCk_Entity InSelfEntity,
            const FVector& InStartLocation,
            const FVector& InGoal,
            const FFragment_CrowdAgent_Params& InParams,
            float InArrivalRadius,
            const TArray<FVector>& InCorridorWaypoints,
            ECk_CrowdAvoidanceVolume_QueryPhase InVolumeQueryPhase,
            TArray<FVector>& OutWaypoints) -> bool;

    private:
        struct FSettledDisc
        {
            FCk_Entity _Owner;
            FVector _Center = FVector::ZeroVector;
            float _Radius = 0.0f;
            uint64 _ConfirmationSerial = 0;
        };

        struct FSettledVolume
        {
            FCk_Entity _Owner;
            crowd_avoidance_volume::FCk_Obb _PhysicalObb;
            crowd_avoidance_volume::FCk_Obb _PaintedObb;
            uint64 _ConfirmationSerial = 0;
        };

        struct FSettledRetirement
        {
            crowd_avoidance_volume::FCk_Obb _PhysicalObb;
            crowd_avoidance_volume::FCk_Obb _PaintedObb;
            uint64 _ConfirmationSerial = 0;
        };

        // Rebuilt in DoTick; read by the per-entity pass the same tick.
        TArray<FSettledDisc> _SettledDiscs;
        TArray<FSettledVolume> _SettledVolumes;
        TArray<FSettledRetirement> _SettledRetirements;
        uint64 _MaxConfirmationSerial = 0;
    };
}

// --------------------------------------------------------------------------------------------------------------------
