#pragma once

#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FGroup_Input_Collect;
    struct FGroup_Input_Bias;
    struct FGroup_Input_Route;

    // ----------------------------------------------------------------------------------------------------------------

    // Raw-input producers land their events in the input-source inbox here. Registration order WITHIN a
    // group is not an ordering guarantee, so the collect/route split has to be expressed as two groups
    // rather than as two processors sharing one — a consumer module can only order against a named group.
    struct FGroup_Input_Collect
    {
        using RunAfter = TDepList<FGroup_Gameplay_TimeDelta>;
    };

    // Per-game conditioning of the collected axes. It sits between collect and route because the router DRAINS
    // the inbox: conditioning has to read the raw rows while they still exist, and reading them before delivery
    // also means a consumer woken by routing already sees this frame's conditioned values rather than last
    // frame's. Both edges are needed — after Collect so the rows and any same-frame retune have landed, before
    // Route so the drain cannot outrun the read.
    struct FGroup_Input_Bias
    {
        using RunAfter  = TDepList<FGroup_Input_Collect>;
        using RunBefore = TDepList<FGroup_Input_Route>;
    };

    // The inbox is drained and delivered here. Anything that reacts to delivered input (intent matching,
    // gameplay) declares RunAfter<FGroup_Input_Route>; the RunBefore keeps the whole raw layer ahead of
    // gameplay so a press is visible on the frame it arrives rather than the one after.
    struct FGroup_Input_Route
    {
        using RunAfter  = TDepList<FGroup_Input_Collect>;
        using RunBefore = TDepList<FGroup_Gameplay>;
    };
}

// --------------------------------------------------------------------------------------------------------------------
