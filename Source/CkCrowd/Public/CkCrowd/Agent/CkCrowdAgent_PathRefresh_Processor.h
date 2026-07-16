#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_StationaryMarkup_Processor.h"

#include "CkNavigation/Nav/CkNav_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Mid-walk path refresh — the second half of StationaryMarkup. The cost discs only influence
    // FindPath at PLAN time; a path computed before a crowd formed is a frozen polyline the agent
    // follows INTO the crowd (UE's own UPathFollowingComponent re-paths when the navmesh under its
    // path rebuilds; the dtCrowd port dropped that half of the mechanism). Each tick, a Walking
    // agent whose REMAINING path passes through a disc painted AFTER its path was installed is
    // re-pathed at its own goal (BlockedRecheck's resume dance), so the fresh plan sees the discs
    // and detours.
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
    class CKCROWD_API FProcessor_CrowdAgent_PathRefresh : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_PathRefresh,
            FCk_Handle_CrowdAgent,
            FTag_CrowdAgent_Walking,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_Nav_PathResult>,
            ck::TReadWrite<FFragment_CrowdAgent_PathFollow>,
            ck::TReadWrite<FFragment_CrowdAgent_BlockDetect>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_CrowdAgent_StationaryMarkup>;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(FCk_Time InDeltaT) -> void;

        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_Nav_PathResult& InPathResult,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_BlockDetect& InBlockDetect) const -> void;

        // When the agent stands INSIDE another agent's CONFIRMED cost disc and its goal is
        // OUTSIDE the band, returns a start just outside the band for the path query. The toll
        // is paid per distance crossed, so for an agent already inside, finishing the crossing
        // is cheaper than backing out PLUS detouring — planning from its feet re-picks
        // "through"; planning from outside restores the detour preference. Unset means plan
        // from the agent's real location: not inside any disc, goal also in-band (a queue
        // member hopping slots must not S-detour), escape failed to converge, or the
        // markup/refresh tiers are disabled. Shared by every re-path site (MoveTo,
        // BlockedRecheck, PathRefresh) — lives here because this class is the fragment's friend.
        static auto
        Get_EscapedQueryStart(
            FCk_Handle InAnyWorldHandle,
            FCk_Entity InSelfEntity,
            const FVector& InSelfLocation,
            const FVector& InGoal,
            float InAgentRadius) -> TOptional<FVector>;

    private:
        struct FSettledDisc
        {
            FCk_Entity _Owner;
            FVector _Center = FVector::ZeroVector;
            float _Radius = 0.0f;
            uint64 _PaintSerial = 0;
        };

        // Rebuilt in DoTick; read by the per-entity pass the same tick.
        TArray<FSettledDisc> _SettledDiscs;
        uint64 _MaxEligibleSerial = 0;
    };
}

// --------------------------------------------------------------------------------------------------------------------
