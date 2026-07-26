#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

#include "CkNavigation/Nav/CkNav_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Draws an X + fail-reason label above any agent whose nav request resolved to Failed or is
    // stuck Pending. Deliberately NOT CVar-gated: an agent that cannot path is broken, and that
    // must be visible the moment PIE starts.
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
