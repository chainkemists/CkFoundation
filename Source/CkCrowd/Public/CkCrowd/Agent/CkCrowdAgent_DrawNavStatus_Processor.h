#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

#include "CkNavigation/Nav/CkNav_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Always-on path-failure marker. For every crowd agent whose nav request resolved to Failed,
    // draws a red X above the capsule and a fail-reason text label. NOT gated by any debug CVar
    // — this is critical visibility: an agent that can't path is broken, and "I clicked PIE and
    // nothing's moving" should produce an immediate visual hint pointing at why.
    //
    // The marker complements the debugger's Agent List status pill. World view says WHICH agent
    // is broken; debugger says WHY (full reason + diagnostics). Together they remove "agent's
    // not moving and I have no idea what happened" from the dev experience.
    class CKCROWD_API FProcessor_CrowdAgent_DrawNavStatus : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DrawNavStatus,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_Nav_PathResult>,
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
            const FFragment_Nav_PathResult& InPathResult) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
