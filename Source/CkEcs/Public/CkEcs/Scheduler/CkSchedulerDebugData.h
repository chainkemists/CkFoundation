#pragma once

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKECS_API FSchedulerDebug_ProcessorTiming
    {
        CK_GENERATED_BODY(FSchedulerDebug_ProcessorTiming);

        FName ProcessorName;
        double MainPassTimeMs = 0.0;
        TArray<double> PumpPassTimesMs;
        bool WasDirtyThisFrame = false;
        int32 PumpCountThisFrame = 0;
        int32 MainPassEntityCount = 0;
        TArray<int32> PumpPassEntityCounts;

        // Main pass skipped this processor because its view was provably empty (see
        // ECk_ProcessorEmptyViewPolicy). MainPassTimeMs/MainPassEntityCount stay 0.
        bool WasSkippedEmptyViewThisFrame = false;
    };

#if !UE_BUILD_SHIPPING
    extern CKECS_API int32 GDebug_LastProcessedEntityCount;
#endif

    // ----------------------------------------------------------------------------------------------------------------

    struct CKECS_API FSchedulerDebug_FrameSnapshot
    {
        CK_GENERATED_BODY(FSchedulerDebug_FrameSnapshot);

        TArray<FSchedulerDebug_ProcessorTiming> ProcessorTimings;
        int32 PumpIterationCount = 0;
        double TotalFrameTimeMs = 0.0;
        uint64 FrameNumber = 0;
        int32 SkippedEmptyViewCount = 0;
    };
}

// --------------------------------------------------------------------------------------------------------------------
