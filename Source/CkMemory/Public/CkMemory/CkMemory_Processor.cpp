#include "CkMemory_Processor.h"

#if CK_ENABLE_MEMORY_TRACKING

#include <CoreMinimal.h>

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Memory_Stats);

DECLARE_MEMORY_STAT(TEXT("ECS Memory"), STAT_EcsMemoryUsage, STATGROUP_Memory);

namespace ck
{
    auto
        FProcessor_Memory_Stats::
        Tick(
            FCk_Time InDeltaT)
        -> void
    {
        SET_MEMORY_STAT(STAT_EcsMemoryUsage, ck::detail::BytesAllocated);
    }
}

// --------------------------------------------------------------------------------------------------------------------

#endif