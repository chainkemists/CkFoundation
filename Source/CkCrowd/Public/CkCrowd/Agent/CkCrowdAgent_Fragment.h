#pragma once

#include "CkCrowd/Agent/CkCrowdAgent_Fragment_Data.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Reflected params alias — entity-side fragment is the same struct as the BP-visible data.
    // Gate 0 carries only the structural fields (Radius, Height, Tags). Subsequent gates add fields
    // to FCk_Fragment_CrowdAgent_ParamsData per the staged plan in PLAN.md.
    using FFragment_CrowdAgent_Params         = FCk_Fragment_CrowdAgent_ParamsData;
    using FFragment_CrowdAgent_PathFollow     = FCk_Fragment_CrowdAgent_PathFollowData;
    using FFragment_CrowdAgent_DesiredVelocity = FCk_Fragment_CrowdAgent_DesiredVelocityData;
    using FFragment_CrowdAgent_FaceAngle      = FCk_Fragment_CrowdAgent_FaceAngleData;

    // Marks an agent as needing one-time setup (Gate 0: stamped by Add(), consumed by FProcessor_CrowdAgent_Setup
    // on the next tick). Gate 3+ uses the consumption to spawn the agent's probe child entity.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_NeedsSetup);

    // Gate 2 — agent state tags. Mutually exclusive: an agent is either Idle (no goal), PathPending
    // (FindPath enqueued, awaiting result), or Walking (path-ready, steering active). Future gates may
    // add Failed (path-failed N times). Set by the move-request handler (sub-task 2E) and the path-ready
    // signal handler; consumed as an exclude on steering view membership.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_Idle);
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_PathPending);
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_Walking);

    // Forward-compatible exclude tag — never stamped in Gate 2; Gate 4 SleepEvaluator stamps it on idle agents.
    // Steering / separation / piercing / apply-offset processors carry TExclude<FTag_CrowdAgent_Asleep> so the
    // Gate 4 sleep optimization is wire-compatible without retro-fitting every view.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_Asleep);
}

// --------------------------------------------------------------------------------------------------------------------
