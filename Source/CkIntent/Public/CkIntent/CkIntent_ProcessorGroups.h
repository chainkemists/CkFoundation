#pragma once

#include "CkInput/CkInput_ProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FGroup_Intent_Collect;
    struct FGroup_Intent_Sample;
    struct FGroup_Intent_Match;

    // ----------------------------------------------------------------------------------------------------------------

    // Every render frame's routed events are buffered here, unrated. The split from Sample is not cosmetic: the
    // router's retention is per-RENDER-frame state, the record is written on a FIXED cadence, and one processor
    // cannot be both. Two groups rather than two processors in one because registration order WITHIN a group is
    // not an ordering guarantee — a consumer can only order against a named group.
    struct FGroup_Intent_Collect
    {
        using RunAfter  = TDepList<FGroup_Input_Route>;
        using RunBefore = TDepList<FGroup_Intent_Sample>;
    };

    // The frame record is written here, and the buffer drained. Before Gameplay so a consumer reading the record
    // acts on the frame the input arrived on rather than the one after.
    struct FGroup_Intent_Sample
    {
        using RunBefore = TDepList<FGroup_Gameplay>;
    };

    // Compiled sets are matched against the rows the sampler just wrote. A separate group rather than a second
    // processor in Sample for the same reason Collect is separate from Sample: the scan's whole input is the row
    // Sample appends, and registration order within a group is not an ordering guarantee. Before Gameplay so a
    // consumer polling an intent phase acts on the frame the input arrived on rather than the one after.
    struct FGroup_Intent_Match
    {
        using RunAfter  = TDepList<FGroup_Intent_Sample>;
        using RunBefore = TDepList<FGroup_Gameplay>;
    };
}

// --------------------------------------------------------------------------------------------------------------------
