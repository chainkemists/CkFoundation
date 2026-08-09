#include "CkProcessor.h"

#include "CkEcs/CkEcsLog.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::processor
{
    auto
        Report_ClampedCatchUpReplay(
            const FString& InProcessorName,
            int32 InMaxReplayedTicks,
            int32 InDroppedTicks)
        -> void
    {
        // Deliberately not a Warning: clamping is the DESIGNED response to a hitch, not a fault, and the
        // AutoTest runner escalates warnings to test failures — a slow machine would then fail every test
        // that happened to stutter.
        ecs::Log
        (
            TEXT("Processor [{}] replayed its catch-up bound of [{}] ticks this frame and dropped [{}] further "
                 "elapsed intervals. Dropped time is not carried forward. A processor that must not miss a step "
                 "needs a higher MaxReplayedTicks; one that only samples wants TickCatchUpPolicy::SampleLatestOnly"),
            InProcessorName, InMaxReplayedTicks, InDroppedTicks
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------
