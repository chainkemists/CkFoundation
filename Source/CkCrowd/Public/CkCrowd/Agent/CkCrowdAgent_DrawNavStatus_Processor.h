#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

#include "CkNavigation/Nav/CkNav_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Draws the current or retained path-trouble evidence when the per-user world-overlay toggle is on.
    class CKCROWD_API FProcessor_CrowdAgent_DrawNavStatus : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DrawNavStatus,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_Nav_PathResult>,
            ck::TReadOnly<FFragment_CrowdAgent_PathFollow>,
            ck::TReadOnly<FFragment_CrowdAgent_PathTrouble>,
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
            const FFragment_Nav_PathResult& InPathResult,
            const FFragment_CrowdAgent_PathFollow& InPathFollow,
            const FFragment_CrowdAgent_PathTrouble& InPathTrouble) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
