#pragma once

#include "CkCrowd/Agent/CkCrowdAgent_Fragment_Data.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Reflected params alias — entity-side fragment is the same struct as the BP-visible data.
    // Gate 0 carries only the structural fields (Radius, Height, Tags). Subsequent gates add fields
    // to FCk_Fragment_CrowdAgent_ParamsData per the staged plan in PLAN.md.
    using FFragment_CrowdAgent_Params = FCk_Fragment_CrowdAgent_ParamsData;

    // Marks an agent as needing one-time setup (Gate 0: stamped by Add(), consumed by FProcessor_CrowdAgent_Setup
    // on the next tick). Gate 3+ uses the consumption to spawn the agent's probe child entity.
    CK_DEFINE_ECS_TAG(FTag_CrowdAgent_NeedsSetup);
}

// --------------------------------------------------------------------------------------------------------------------
