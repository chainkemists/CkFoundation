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
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKECS_API FSchedulerDebug_FrameSnapshot
    {
        CK_GENERATED_BODY(FSchedulerDebug_FrameSnapshot);

        TArray<FSchedulerDebug_ProcessorTiming> ProcessorTimings;
        int32 PumpIterationCount = 0;
        double TotalFrameTimeMs = 0.0;
        uint64 FrameNumber = 0;
    };
}

// --------------------------------------------------------------------------------------------------------------------
