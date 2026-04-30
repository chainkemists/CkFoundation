#pragma once

#include "CkNav_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

class ARecastNavMesh;
class dtCrowd;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Forward declarations so RunAfter dep lists (TDepList<...>) can reference processors
    // that appear later in this header.
    class FProcessor_Nav_HandleRequests;
    class FProcessor_Nav_CrowdSetup;
    class FProcessor_Nav_CrowdPushPosition;
    class FProcessor_Nav_CrowdUpdateTarget;
    class FProcessor_Nav_CrowdStep;
    class FProcessor_Nav_CrowdReadVelocity;
    class FProcessor_Nav_CrowdEndPlay;

    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Nav_HandleRequests
    //
    // Drains FFragment_Nav_Requests on each nav agent, runs FindPathSync per request, writes
    // FFragment_Nav_PathResult, broadcasts OnNavPathReady / OnNavPathFailed.
    //
    // Per-frame budget reset in DoTick (P3-2). MaxPathQueriesPerFrame from project settings;
    // 0 = disabled-with-warn. Tests stay queued (FTag_Nav_PathPending stays set) when budget
    // runs out and resume next frame.
    //
    // ForEachEntity is non-const because the budget counter is mutated during draining.
    // ----------------------------------------------------------------------------------------------------------------
    class CKNAVIGATION_API FProcessor_Nav_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Nav_HandleRequests,
            FCk_Handle_NavAgent,
            ck::TReadOnly<FFragment_Nav_AgentParams>,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadWrite<FFragment_Nav_Requests>,
            ck::TReadWrite<FFragment_Nav_PathResult>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group         = FGroup_PostTransform;
        using MarkedDirtyBy = FFragment_Nav_Requests;

    public:
        FProcessor_Nav_HandleRequests(
            const RegistryType& InRegistry,
            const TWeakPtr<dtCrowd>& InCrowdWeak,
            const TWeakObjectPtr<ARecastNavMesh>& InNavMeshWeak);

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Nav_AgentParams& InParams,
            const FFragment_Transform& InTransform,
            FFragment_Nav_Requests& InRequests,
            FFragment_Nav_PathResult& InPathResult) -> void;

    private:
        // P3-2 budget. Reset to project-settings value at the top of each frame in DoTick.
        // Decremented in ForEachEntity per query consumed. When 0 (and budget > 0 globally),
        // remaining agents skip this frame; their FTag_Nav_PathPending stays set so they
        // don't double-queue. When budget == 0 globally, treat as DISABLED (warn-once at init).
        int32 _RemainingQueriesThisFrame = 0;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Nav_CrowdSetup
    //
    // Registers nav agents with dtCrowd. Runs once per agent (gated by FTag_Nav_NeedsSetup).
    // On success: adds FFragment_Nav_CrowdAgent + FTag_Nav_CrowdRegistered, removes NeedsSetup.
    // On crowd-full failure: removes NeedsSetup + logs Error (do NOT add CrowdAgent or
    // CrowdRegistered — agent stays inert until the user destroys it or the crowd has room).
    //
    // Silent return on null crowd weak-pin: legitimate during the nav-regen debounce window
    // (P3-3), where DoExecuteCrowdRebuild deliberately drops registry context before re-init.
    // ----------------------------------------------------------------------------------------------------------------
    class CKNAVIGATION_API FProcessor_Nav_CrowdSetup : public ck_exp::TProcessor<
            FProcessor_Nav_CrowdSetup,
            FCk_Handle_NavAgent,
            ck::TReadOnly<FFragment_Nav_AgentParams>,
            ck::TReadOnly<FFragment_Transform>,
            FTag_Nav_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group         = FGroup_PostTransform;
        using MarkedDirtyBy = FTag_Nav_NeedsSetup;

    public:
        FProcessor_Nav_CrowdSetup(
            const RegistryType& InRegistry,
            const TWeakPtr<dtCrowd>& InCrowdWeak,
            const TWeakObjectPtr<ARecastNavMesh>& InNavMeshWeak);

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Nav_AgentParams& InParams,
            const FFragment_Transform& InTransform) const -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Nav_CrowdPushPosition
    //
    // Teleport detection only. dtCrowd is the velocity advisor; consumers apply velocity to
    // the entity transform, dtCrowd reads the resulting transform back via npos. We force-sync
    // npos only when the entity's transform jumps past TeleportThresholdUu — full
    // removeAgent + addAgent rebuild (A7: no fast-path direct npos write — corridor desync
    // produces avoidance bugs near walls that are nearly impossible to debug).
    //
    // In steady state this processor does ZERO work per agent per frame beyond a distance
    // check. The TReadWrite on FFragment_Nav_CrowdAgent is required: the rebuild path
    // overwrites _CrowdAgentIndex with the new index returned by addAgent.
    // ----------------------------------------------------------------------------------------------------------------
    class CKNAVIGATION_API FProcessor_Nav_CrowdPushPosition : public ck_exp::TProcessor<
            FProcessor_Nav_CrowdPushPosition,
            FCk_Handle_NavAgent,
            ck::TReadWrite<FFragment_Nav_CrowdAgent>,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_Nav_AgentParams>,
            FTag_Nav_CrowdRegistered,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group    = FGroup_PostTransform;
        using RunAfter = TDepList<FProcessor_Nav_CrowdSetup>;

    public:
        FProcessor_Nav_CrowdPushPosition(
            const RegistryType& InRegistry,
            const TWeakPtr<dtCrowd>& InCrowdWeak,
            const TWeakObjectPtr<ARecastNavMesh>& InNavMeshWeak);

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Nav_CrowdAgent& InCrowdAgent,
            const FFragment_Transform& InTransform,
            const FFragment_Nav_AgentParams& InParams) const -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Nav_CrowdUpdateTarget
    //
    // Pushes the entity's destination (last waypoint, or _DestinationLocation when waypoints
    // are empty) into dtCrowd via requestMoveTarget. Triggered by FTag_Nav_MoveTargetDirty —
    // HandleRequests sets it after a successful path query, so a query landing this frame
    // gets its move target applied this frame too (RunAfter HandleRequests in the same
    // FGroup_PostTransform group).
    //
    // P3-6: dtCrowd::requestMoveTarget can return false (disconnected nav components,
    // invalid agent index). Failure flips the agent to PathFailed / clears PathReady.
    // ----------------------------------------------------------------------------------------------------------------
    class CKNAVIGATION_API FProcessor_Nav_CrowdUpdateTarget : public ck_exp::TProcessor<
            FProcessor_Nav_CrowdUpdateTarget,
            FCk_Handle_NavAgent,
            ck::TReadOnly<FFragment_Nav_CrowdAgent>,
            ck::TReadOnly<FFragment_Nav_PathResult>,
            FTag_Nav_CrowdRegistered,
            FTag_Nav_PathReady,
            FTag_Nav_MoveTargetDirty,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group         = FGroup_PostTransform;
        using MarkedDirtyBy = FTag_Nav_MoveTargetDirty;
        using RunAfter      = TDepList<FProcessor_Nav_HandleRequests, FProcessor_Nav_CrowdSetup, FProcessor_Nav_CrowdPushPosition>;

    public:
        FProcessor_Nav_CrowdUpdateTarget(
            const RegistryType& InRegistry,
            const TWeakPtr<dtCrowd>& InCrowdWeak,
            const TWeakObjectPtr<ARecastNavMesh>& InNavMeshWeak);

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Nav_CrowdAgent& InCrowdAgent,
            const FFragment_Nav_PathResult& InPathResult) const -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Nav_CrowdReadVelocity
    //
    // Copies dtCrowd's per-agent steered velocity (vel) and desired velocity (dvel) into
    // FFragment_Nav_CrowdVelocity. Consumers read this fragment to integrate movement —
    // the consumer applies velocity to the entity transform, and CrowdPushPosition reads
    // the resulting transform back to keep dtCrowd's npos in sync (steady-state no-op).
    //
    // Runs after CrowdStep so the velocities reflect this frame's update.
    // ----------------------------------------------------------------------------------------------------------------
    class CKNAVIGATION_API FProcessor_Nav_CrowdReadVelocity : public ck_exp::TProcessor<
            FProcessor_Nav_CrowdReadVelocity,
            FCk_Handle_NavAgent,
            ck::TReadOnly<FFragment_Nav_CrowdAgent>,
            ck::TReadWrite<FFragment_Nav_CrowdVelocity>,
            FTag_Nav_CrowdRegistered,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group    = FGroup_PostTransform;
        using RunAfter = TDepList<FProcessor_Nav_CrowdStep>;

    public:
        FProcessor_Nav_CrowdReadVelocity(
            const RegistryType& InRegistry,
            const TWeakPtr<dtCrowd>& InCrowdWeak,
            const TWeakObjectPtr<ARecastNavMesh>& InNavMeshWeak);

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Nav_CrowdAgent& InCrowdAgent,
            FFragment_Nav_CrowdVelocity& InVelocity) const -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Nav_CrowdEndPlay
    //
    // Removes the agent from dtCrowd when the entity is being destroyed. Tolerates a null
    // crowd weak-pin: the world subsystem may tear down before entity teardown completes.
    // ----------------------------------------------------------------------------------------------------------------
    class CKNAVIGATION_API FProcessor_Nav_CrowdEndPlay : public ck_exp::TProcessor<
            FProcessor_Nav_CrowdEndPlay,
            FCk_Handle_NavAgent,
            ck::TReadOnly<FFragment_Nav_CrowdAgent>,
            FTag_Nav_CrowdRegistered,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        FProcessor_Nav_CrowdEndPlay(
            const RegistryType& InRegistry,
            const TWeakPtr<dtCrowd>& InCrowdWeak,
            const TWeakObjectPtr<ARecastNavMesh>& InNavMeshWeak);

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Nav_CrowdAgent& InCrowdAgent) const -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Nav_CrowdStep
    //
    // Per-frame, one dtCrowd::update call covers ALL agents — work happens in DoTick, not
    // ForEachEntity. The TProcessor template constraints require a Handle + at least one
    // fragment in the match set, so we match on the cheap CrowdAgent fragment that every
    // registered agent has anyway. ForEachEntity is intentionally empty; we deliberately
    // skip the trailing TProcessor::DoTick(InDeltaT) call — per-agent iteration would be
    // pure overhead since dtCrowd::update already covered all of them.
    //
    // Silent early-out on null crowd is intentional — common during the nav-regen debounce
    // window (P3-3), where the subsystem deliberately drops the weak ref before re-init.
    // ----------------------------------------------------------------------------------------------------------------
    class CKNAVIGATION_API FProcessor_Nav_CrowdStep : public ck_exp::TProcessor<
            FProcessor_Nav_CrowdStep,
            FCk_Handle_NavAgent,
            ck::TReadOnly<FFragment_Nav_CrowdAgent>,
            FTag_Nav_CrowdRegistered,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group    = FGroup_PostTransform;
        using RunAfter = TDepList<FProcessor_Nav_CrowdSetup, FProcessor_Nav_CrowdPushPosition, FProcessor_Nav_CrowdUpdateTarget>;

    public:
        FProcessor_Nav_CrowdStep(
            const RegistryType& InRegistry,
            const TWeakPtr<dtCrowd>& InCrowdWeak,
            const TWeakObjectPtr<ARecastNavMesh>& InNavMeshWeak);

    public:
        auto
        DoTick(TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Nav_CrowdAgent& InCrowdAgent) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
