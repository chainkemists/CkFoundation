#include "CkCrowdAgent_TransientPersonalSpace_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_TransientPersonalSpace);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_TransientPersonalSpace::
        ForEachEntity(
            TimeType InDeltaT,
            FCk_Handle_CrowdAgent,
            FFragment_CrowdAgent_TransientPersonalSpace& InPersonalSpace)
        -> void
    {
        if (InPersonalSpace.Get_RemainingSeconds() <= 0.0f)
        { return; }

        InPersonalSpace._RemainingSeconds = FMath::Max(
            0.0f,
            InPersonalSpace.Get_RemainingSeconds() - static_cast<float>(InDeltaT.Get_Seconds()));
        if (InPersonalSpace.Get_RemainingSeconds() <= 0.0f)
        { InPersonalSpace._Scale = 1.0f; }
    }
}

// --------------------------------------------------------------------------------------------------------------------
