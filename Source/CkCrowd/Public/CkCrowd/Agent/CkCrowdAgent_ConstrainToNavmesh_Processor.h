#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_PushApart_Processor.h"

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
    class CKCROWD_API FProcessor_CrowdAgent_ConstrainToNavmesh : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_ConstrainToNavmesh,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadWrite<FFragment_CrowdAgent_PendingDisplacement>,
            ck::TReadWrite<FFragment_CrowdAgent_Grounding>,
            TExclude<FTag_CrowdAgent_Asleep>,
            TExclude<FTag_CrowdAgent_Flying>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_PushApart>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PendingDisplacement& InPending,
            FFragment_CrowdAgent_Grounding& InGrounding) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
