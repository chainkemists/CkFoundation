#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

#include "CkGroundNav/Path/CkGroundNavPath_Fragment.h"

#include "CkNavigation/Nav/CkNav_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Draws the shadow-mode A/B evidence: the Recast polyline that was actually installed against the
    // GroundNav polyline that was searched for the same query and deliberately never installed.
    //
    // Deliberately a SEPARATE processor from FProcessor_CrowdAgent_DrawNavStatus rather than a fourth
    // fragment on that one's view: requiring FFragment_GroundNavPath_Result there would narrow it to
    // agents carrying the GroundNav feature and silently stop drawing path trouble for everyone else.
    class CKCROWD_API FProcessor_CrowdAgent_DrawShadowRoutes : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DrawShadowRoutes,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_Nav_PathResult>,
            ck::TReadOnly<FFragment_GroundNavPath_Result>,
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
            const FFragment_GroundNavPath_Result& InGroundNavResult) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
