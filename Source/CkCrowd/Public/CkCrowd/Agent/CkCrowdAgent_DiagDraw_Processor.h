#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkCrowd/Agent/CkCrowdAgent_Diag_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Diag_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Incremental retained breadcrumb maintenance. Newly recorded movement is appended to bounded
    // PMG line-set chunks; unchanged history causes no line work and completed chunks never rebake.
    class CKCROWD_API FProcessor_CrowdAgent_DiagDrawBreadcrumb : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DiagDrawBreadcrumb,
            FCk_Handle_CrowdAgent,
            FTag_CrowdDiag_Tracked,
            ck::TReadOnly<FFragment_CrowdAgent_DiagRecorder>,
            ck::TReadWrite<FFragment_CrowdAgent_DiagBreadcrumb>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_DiagRecorder>;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_DiagRecorder& InRecorder,
            FFragment_CrowdAgent_DiagBreadcrumb& InBreadcrumb) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
