#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Projects each agent's world position onto the navmesh and draws a circle there: green when
    // the projection succeeded, red when it failed (the agent is in a void the nav system cannot
    // snap to — the usual cause of a "floating" agent).
    class CKCROWD_API FProcessor_CrowdAgent_DrawNavProjection : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DrawNavProjection,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            CK_IGNORE_PENDING_KILL>
    {
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
            const FFragment_CrowdAgent_Params& InParams) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
