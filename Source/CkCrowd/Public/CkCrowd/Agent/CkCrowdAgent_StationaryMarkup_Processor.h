#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Stationary-agent nav markup. The path planner only sees static geometry, so a path for an
    // agent headed past a standing crowd goes straight THROUGH it, and the local avoidance
    // sampler (short-horizon, greedy) cannot escape a line-shaped local minimum — the agent
    // presses into the crowd. This processor paints a Nav.Area.Crowd.Agent COST disc under any
    // agent that has stayed below _StationaryMarkupSpeedThreshold for
    // _StationaryMarkupDelaySeconds — measured by windowed displacement, NOT the Idle tag: a
    // blocked/pressing walker plugs a corridor exactly like an idle agent, and mutual pressers
    // never go Idle at all. The disc drops the moment the agent exceeds that threshold — so fresh
    // paths (a joiner's first FindPath, BlockedRecheck's re-path, PathRefresh) genuinely route
    // around standing crowds.
    //
    // Cost, never a hole: the mesh under the agent stays walkable, so the agent's own navmesh
    // clamp (ConstrainToNavmesh), its path starts, and slot projections are unaffected, and a
    // fully-plugged corridor still paths through rather than failing.
    //
    // Server-only: pathfinding is server-authoritative; client worlds skip painting.
    //
    // Provider-neutral: the disc is raised through the NavSurface facade as an area-tag markup, so
    // whichever provider answers the world paints it. The paint is a REQUEST — it lands on a later
    // drain, never in the frame it was raised — which is why eligibility is gated on _ConfirmedOnMesh
    // rather than on the handle existing.
    //
    // A flying agent is excluded: the disc is a cost area on the walkable surface, and a hovering
    // flyer standing still in mid-air blocks nothing a walker paths through.
    class CKCROWD_API FProcessor_CrowdAgent_StationaryMarkup : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_StationaryMarkup,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadWrite<FFragment_CrowdAgent_NavMarkup>,
            TExclude<FTag_CrowdAgent_Asleep>,
            TExclude<FTag_CrowdAgent_Flying>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_NavMarkup& InMarkup) -> void;

        // Shared with the EndPlay processor — lives here because this class is the fragment's friend.
        // Takes the handle so hard-body confirmation is dropped in the same instruction as the
        // mesh confirmation it mirrors.
        static auto
        Remove_Markup(
            FCk_Handle_CrowdAgent& InHandle,
            FFragment_CrowdAgent_NavMarkup& InMarkup) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Destroys the markup entity on agent teardown, so no stale paint remains on the world's
    // navigation surface.
    class CKCROWD_API FProcessor_CrowdAgent_NavMarkup_EndPlay : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_NavMarkup_EndPlay,
            FCk_Handle_CrowdAgent,
            ck::TReadWrite<FFragment_CrowdAgent_NavMarkup>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_CrowdAgent_NavMarkup& InMarkup) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
