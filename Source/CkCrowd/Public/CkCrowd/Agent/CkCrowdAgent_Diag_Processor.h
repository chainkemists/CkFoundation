#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Diag_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Per-frame recorder for tracked agents. Reads transform + desired velocity + neighbor cache,
    // appends a sample to the recorder fragment at the cadence set by ck.Crowd.DiagSampleHz
    // (default 10Hz), and updates the per-cycle running metrics (min-sep, dir-reversals, max
    // angular delta, time-to-goal). View-membership-keyed on FTag_CrowdDiag_Tracked so untagged
    // agents pay no overhead — the recorder is opt-in per agent.
    //
    // Group: default. Runs each frame regardless of physics — sample timing is independent of
    // the steering pipeline (we capture what the simulation produced, we don't drive it).
    class CKCROWD_API FProcessor_CrowdAgent_DiagRecorder : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DiagRecorder,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            FTag_CrowdDiag_Tracked,
            ck::TReadOnly<FFragment_CrowdAgent_NeighborCache>,
            ck::TReadWrite<FFragment_CrowdAgent_DiagRecorder>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            FFragment_CrowdAgent_DiagRecorder& InRecorder) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
