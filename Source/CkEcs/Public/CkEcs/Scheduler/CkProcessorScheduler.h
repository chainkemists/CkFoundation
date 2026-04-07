#pragma once

#include "CkProcessorGraph.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKECS_API FProcessorScheduler
    {
    public:
        CK_GENERATED_BODY(FProcessorScheduler);

        explicit FProcessorScheduler(
            FProcessorGraphPartition&& InPartition);

    public:
        auto Tick(FCk_Time InDeltaTime, const FCk_Registry& InRegistry) -> void;

        auto Get_IsTickInProgress() const -> bool;

    private:
        auto DoPump(
            const FCk_Registry& InRegistry,
            int32 InPumpIndex) -> bool;

        auto DoLogPumpLimitReached(
            const FCk_Registry& InRegistry) -> void;

    private:
        FProcessorGraphPartition _Partition;
        int32 _MaxPumpIterations = 15;
        bool _IsTickInProgress = false;

    private:
        int32 _LastFramePumpCount = 0;
        double _LastGraphBuildTimeMs = 0.0;

    public:
        CK_PROPERTY_GET(_LastFramePumpCount);
        CK_PROPERTY(_MaxPumpIterations);
    };
}

// --------------------------------------------------------------------------------------------------------------------
