#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Tag/CkTag.h"

#include "CkQueue/Queue/CkQueue_Fragment_Data.h"

class UCk_Utils_CrowdQueueAdapter_UE;

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_CrowdQueueAdapter_LeaveRequested);

    struct CKCROWD_API FFragment_CrowdQueueAdapter
    {
        CK_GENERATED_BODY(FFragment_CrowdQueueAdapter);

        friend class FProcessor_CrowdQueueAdapter_Dispatch;
        friend class FProcessor_CrowdQueueAdapter_ObserveOutcome;
        friend class FProcessor_CrowdQueueAdapter_EndPlay;
        friend class ::UCk_Utils_CrowdQueueAdapter_UE;

    private:
        FCk_Handle_Queue _Queue;
        bool _JoinPending = false;
        int32 _PendingQueueRevision = 0;
        int32 _IssuedQueueAssignmentRevision = 0;
        int32 _IssuedCrowdCorrelationId = 0;
        int32 _ReportedQueueAssignmentRevision = 0;
        int32 _NextCrowdCorrelationId = 1;

    public:
        CK_PROPERTY_GET(_Queue);
        CK_PROPERTY_GET(_JoinPending);
        CK_PROPERTY_GET(_PendingQueueRevision);
        CK_PROPERTY_GET(_IssuedQueueAssignmentRevision);
        CK_PROPERTY_GET(_IssuedCrowdCorrelationId);
        CK_PROPERTY_GET(_ReportedQueueAssignmentRevision);
    };
}
