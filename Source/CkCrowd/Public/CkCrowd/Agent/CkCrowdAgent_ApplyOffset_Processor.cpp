#include "CkCrowdAgent_ApplyOffset_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkCrowd/CkCrowd_Stats.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_ApplyOffset);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::ApplyOffset"), STAT_CkCrowd_ApplyOffsetProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_ApplyOffset::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EulerIntegrator_Current& InIntegrator,
            FFragment_CrowdAgent_PendingDisplacement& InPending)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_ApplyOffsetProc);

        if (InIntegrator.Get_DistanceOffset().IsNearlyZero())
        { return; }

        InPending._Displacement += InIntegrator.Get_DistanceOffset();
    }
}

// --------------------------------------------------------------------------------------------------------------------
