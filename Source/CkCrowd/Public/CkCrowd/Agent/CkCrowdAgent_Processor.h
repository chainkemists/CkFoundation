#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Gate 0: consumes FTag_CrowdAgent_NeedsSetup. Body is empty — structural fragments
    // are already on the entity by the time this fires (Add() does the structural work
    // before stamping the dirty tag). Subsequent gates flesh out one-time setup work
    // here: Gate 3 spawns the agent's probe child entity (Cylinder + Probe), Gate 5
    // wires up player-yield flag interpretation, etc.
    class CKCROWD_API FProcessor_CrowdAgent_Setup : public ck_exp::TProcessor<
        FProcessor_CrowdAgent_Setup,
        FCk_Handle_CrowdAgent,
        ck::TReadOnly<FFragment_CrowdAgent_Params>,
        FTag_CrowdAgent_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_CrowdAgent_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
