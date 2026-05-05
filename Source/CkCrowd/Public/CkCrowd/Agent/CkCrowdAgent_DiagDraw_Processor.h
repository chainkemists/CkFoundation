#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkCrowd/Agent/CkCrowdAgent_Diag_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Immediate-mode breadcrumb draw for diagnostic-tracked agents. Each frame, walks the
    // recorder's sample buffer and draws an orange polyline at agent body height connecting
    // consecutive samples. Gated by the `ck.Crowd.DiagDrawBreadcrumb` CVar; defaults off so
    // there's zero cost outside the diag debugging loop.
    //
    // Group: default. Runs alongside the recorder; uses TReadOnly on the recorder fragment so
    // it doesn't conflict with FProcessor_CrowdAgent_DiagRecorder writing samples in the same
    // frame. View-membership-keyed on FTag_CrowdDiag_Tracked, so untagged agents are invisible
    // to this processor — the gym opts each spawned agent in via Track().
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
