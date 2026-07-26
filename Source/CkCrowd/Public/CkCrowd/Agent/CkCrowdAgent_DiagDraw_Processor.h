#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkCrowd/Agent/CkCrowdAgent_Diag_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Immediate-mode breadcrumb polyline over the recorder's sample buffer, off by default. Opt-in
    // per agent: FTag_CrowdDiag_Tracked is stamped by Track(), so untagged agents pay nothing.
    class CKCROWD_API FProcessor_CrowdAgent_DiagDrawBreadcrumb : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DiagDrawBreadcrumb,
            FCk_Handle_CrowdAgent,
            FTag_CrowdDiag_Tracked,
            ck::TReadOnly<FFragment_CrowdAgent_DiagRecorder>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_DiagRecorder& InRecorder) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
