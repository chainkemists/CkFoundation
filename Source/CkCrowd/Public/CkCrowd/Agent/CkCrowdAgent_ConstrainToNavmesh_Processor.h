#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_PushApart_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_Steering_Processor.h"

#include "CkNavigation/NavSurface/CkNavSurface_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

struct FCk_NavSurface_ProviderTable;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // The stage the dtCrowd port originally dropped: Detour passes every integrated agent position
    // through dtPathCorridor::movePosition (DetourCrowd.cpp:1345, updateStepMove) — a
    // moveAlongSurface walk that constrains the agent to the polygon mesh, so a dtCrowd agent
    // CANNOT leave the navmesh; walls stop it and it slides. Without this stage, any lateral force
    // (separation redirect, avoidance velocity, push-apart shove) moves the transform straight
    // through a navmesh boundary — the wall-eroded band around geometry included.
    //
    // ApplyOffset (integrator delta) and PushApart (de-overlap shove) both stage into
    // FFragment_CrowdAgent_PendingDisplacement; this processor is the SINGLE Transform writer for
    // a crowd agent. It walks the accumulated displacement along the mesh via
    // ANavigationData::FindMoveAlongSurface and enqueues one Request_AddLocationOffset. The agent
    // Transform is its feet anchor, so XY comes from the surface walk and Z lands on the reached
    // navmesh surface instead of passing free-space integrator momentum through.
    //
    // Worlds with no nav data pass displacements through untouched (nav-less tests / gameplay).
    // An agent found off-mesh while nav data exists is snapped back if a horizontally wider,
    // body-height projection finds the mesh (self-healing — a one-frame corner leak must not
    // disable the clamp forever) — but the snap may LIFT at most _GroundingRecoveryMaxStepUpCm:
    // mesh higher than a step is an elevated island the agent could never have walked onto, and
    // recovering upward onto it ratchets the agent skyward island by island. Beyond recovery, the
    // agent is reported through FFragment_CrowdAgent_Grounding and — because agents have no
    // gravity, so free-space displacement is a constant-Z glide off every cliff edge — its
    // displacement is HELD, not applied (_OffMeshDisplacementMode).
    //
    // Grounding runs on a LEASE, never only on displacement: a zero-displacement agent is still
    // reconciled against the navmesh once per _GroundingVerifyIntervalSeconds (Z-only, dead-banded
    // — no planar correction, so a settled formation cannot creep). Do NOT reintroduce a plain
    // zero-displacement early-out as an optimisation: this processor is the ONLY thing that can
    // return an elevated agent's Z to the surface, and for years the only reason stationary agents
    // stayed grounded was the push-apart solver never terminating — sub-millimetre corrections
    // kept this processor running by accident. The push-apart slop ended that, stationary agents'
    // Z froze wherever it was, and every path they asked for returned NoRouteFound for the rest of
    // the session (the floating-NPC regression). The lease is the deliberate replacement for that
    // accident.
    //
    // Group: FGroup_Physics. RunAfter PushApart (the last staging writer). The resulting
    // AddLocationOffset request is drained by Transform_HandleRequests.
    //
    // A flying agent is not on a surface, so it is excluded and its staged displacement is applied
    // whole by FProcessor_CrowdAgent_ApplyDisplacement3D instead — the two views partition the agent
    // population, keeping exactly one Transform writer per agent.
    //
    // Being the single Transform writer is also what makes this the only honest place to ask the
    // per-frame containment question a shadow run needs answered: whether the position the constraint
    // just resolved is on walkable ground for BOTH providers. It is a question about a position, not
    // about a query pair, so the shadow comparison of routes cannot ask it — nothing else in the frame
    // knows where the agent ended up. The world's shadow mode and the provider pairing it names are
    // read once per tick, so a world that is not shadowing pays nothing per agent and a world that is
    // pays one registry read a frame rather than one per body.
    class CKCROWD_API FProcessor_CrowdAgent_ConstrainToNavmesh : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_ConstrainToNavmesh,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadWrite<FFragment_CrowdAgent_PendingDisplacement>,
            ck::TReadWrite<FFragment_CrowdAgent_Grounding>,
            ck::TReadWrite<FFragment_CrowdAgent_PathFollow>,
            TExclude<FTag_CrowdAgent_Asleep>,
            TExclude<FTag_CrowdAgent_Flying>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        // Steering is named alongside the staging chain because this pass now WRITES the same
        // path-follow fragment Steering owns, to end a crossing the route left behind.
        using RunAfter = TDepList<FProcessor_CrowdAgent_PushApart, FProcessor_CrowdAgent_Steering>;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(FCk_Time InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PendingDisplacement& InPending,
            FFragment_CrowdAgent_Grounding& InGrounding,
            FFragment_CrowdAgent_PathFollow& InPathFollow) const -> void;

        /**
         * Whether two providers disagree about whether a position is contained at all: one found
         * walkable ground under it and the other found none.
         *
         * Agreement is not an escape in EITHER direction - contained on both is the ordinary answer,
         * and off both is an agent genuinely in free space, which the grounding report already owns.
         * Only the split verdict says the two providers cover different ground, which is the whole
         * question a shadow run is asking.
         */
        static auto
        Get_IsContainmentEscape(
            ECk_NavSurface_QueryStatus InActiveStatus,
            ECk_NavSurface_QueryStatus InShadowStatus) -> bool;

    private:
        // The constraint proper. Hands back through InOutResolvedOffset everything it enqueued this
        // frame, so the containment check downstream asks about the position this pass RESOLVED
        // rather than the one it was handed - the Transform write is a queued request and has not
        // landed by the time the check runs.
        static auto
        DoConstrain(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PendingDisplacement& InPending,
            FFragment_CrowdAgent_Grounding& InGrounding,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FVector& InOutResolvedOffset) -> void;

        // Costs nothing at all while the world is not shadowing, and nothing extra per agent while
        // it is: the tick has already resolved the pairing, so this either holds two provider tables
        // to project onto or leaves on its first read. An agent part-way across an authored link is
        // skipped outright - it stands between the two points the link joins and on no walkable cell
        // in between, so whichever way that pair lands it measures the link and not the ground.
        auto
        DoRecord_ContainmentEscape(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FVector& InResolvedLocation) const -> void;

    private:
        // Resolves the world's shadow mode and the provider pairing it names into the two tables
        // below, or leaves both null when there is nothing to count.
        auto DoRefresh_ShadowPairing() -> void;

    private:
        // Rebuilt every tick; read by the per-entity pass the same tick. Which provider a world plans
        // on and whether it is shadowing cannot move under the pass, so both are asked once.
        const FCk_NavSurface_ProviderTable* _ActiveProviderTable = nullptr;
        const FCk_NavSurface_ProviderTable* _ShadowProviderTable = nullptr;

        // Said once for the life of the world. A world shadowing while it ALREADY plans on GroundNav
        // has no second answer to disagree with, so the counter cannot be collected under that
        // pairing at all - and a reader of a zero is owed the reason rather than left to infer one.
        bool _UncountablePairingAnnounced = false;
    };
}

// --------------------------------------------------------------------------------------------------------------------
