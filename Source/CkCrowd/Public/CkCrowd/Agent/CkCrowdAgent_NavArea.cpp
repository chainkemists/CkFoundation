#include "CkCrowdAgent_NavArea.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_NavArea_CrowdAgent::UCk_NavArea_CrowdAgent()
{
    // The toll is paid over the CHORD the path actually crosses, and the planner crosses at the
    // cheapest point — the thin seam BETWEEN two neighbouring discs, not through an agent's
    // centre. Any finite toll therefore has a BREAK-EVEN line length: once detouring around the
    // line costs more than crossing its ~170uu band, "through" wins again. 8x broke at ~2
    // agents (gym paths threaded the seams), 16x broke at ~20 (a long L-queue was crossed by
    // brand-new planners). 64x puts break-even at ~10,000uu of detour — an ~85-agent line,
    // beyond any plausible store queue. Still cost, never a hole: a fully-plugged corridor
    // remains traversable rather than unreachable, and a queue member's short hop (both ends
    // in-band) still beats any loop-around.
    DefaultCost = 64.0f;
    DrawColor = FColor::Orange;
}

// --------------------------------------------------------------------------------------------------------------------
