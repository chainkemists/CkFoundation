#pragma once

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::ck_crowd_agent_settled_algorithm
{
    // "This body is not going to move out of the way." Shared by the two tiers that must agree on
    // it: BlockDetect's cluster detector, which anchors a block on a settled neighbour, and the
    // avoidance sampler's final-approach envelope, which stands down at the settled pack's boundary
    // so that block can be reached. A divergence between the two would let the sampler hold an agent
    // off a pack the detector considers anchorable, which is exactly the standoff both exist to end.
    //
    // WHY the neighbour is parked is deliberately not asked, and its own goal is never read: a
    // stranger stopped on the spot obstructs precisely as much as a rival for the same destination.
    // Callers bound the rule with geometry instead.
    //
    // Markup is the BOOTSTRAP disjunct. It paints on windowed physical stillness alone, so a
    // Walking-but-jammed agent counts — without it a crowd whose innermost agent hovers just outside
    // its goal produces no reached/blocked anchor at all and nothing can ever settle.
    //
    // PRECONDITION: InNeighbour is a valid crowd agent. Get_HasReachedActiveGoal ensures on an
    // invalid handle, so callers filter with UCk_Utils_CrowdAgent_UE::Cast first.
    inline auto Is_NeighbourSettled(
        const FCk_Handle_CrowdAgent& InNeighbour,
        bool InMarkupAnchorsEnabled) -> bool
    {
        const auto NeighbourIsParked = InMarkupAnchorsEnabled &&
            InNeighbour.Has<FFragment_CrowdAgent_NavMarkup>() &&
            InNeighbour.Get<FFragment_CrowdAgent_NavMarkup>().Get_Markup().IsValid();

        return UCk_Utils_CrowdAgent_UE::Get_HasReachedActiveGoal(InNeighbour) ||
            InNeighbour.Has<FTag_CrowdAgent_GoalBlocked>() ||
            InNeighbour.Has<FTag_CrowdAgent_GoalFailedHold>() ||
            NeighbourIsParked;
    }
}

// --------------------------------------------------------------------------------------------------------------------
