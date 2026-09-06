#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Separation_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Immediate-mode per-agent separation diagnostics (radius circle, neighbour lines, force arrow),
    // gated by UCk_Utils_Crowd_DebugSettings_UE::Get_DrawSeparation() and off by default.
    class CKCROWD_API FProcessor_CrowdAgent_DebugDraw : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DebugDraw,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            FTag_CrowdAgent_HasProbe,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_CrowdAgent_NeighborCache>,
            ck::TReadOnly<FFragment_CrowdAgent_SeparationForce>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_Separation>;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            FCk_Time InDeltaT) -> void;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            const FFragment_CrowdAgent_SeparationForce& InSeparationForce) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
