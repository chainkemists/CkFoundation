#pragma once

#include "CkProcessorGraph.h"
#include "CkSchedulerDebugData.h"

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
        int32 _MaxPumpIterations = 30;
        bool _IsTickInProgress = false;

        // Cached once at construction from UCk_Ecs_ProjectSettings_UE — this setting is not expected
        // to change at runtime, so reading it per-Tick (let alone per-pump) would be wasteful.
        bool _UseDirtyMarkerVersionShortCircuit = false;

    private:
        int32 _LastFramePumpCount = 0;
        double _LastGraphBuildTimeMs = 0.0;

#if !UE_BUILD_SHIPPING
    private:
        static constexpr int32 DebugFrameHistoryMax = 300;
        TArray<FSchedulerDebug_FrameSnapshot> _DebugFrameHistory;
        FSchedulerDebug_FrameSnapshot _DebugCurrentFrame;

        auto DoDebugBeginFrame() -> void;
        auto DoDebugRecordProcessorTick(int32 InNodeIndex, double InElapsedMs) -> void;
        auto DoDebugRecordProcessorPump(int32 InNodeIndex, int32 InPumpPass, double InElapsedMs) -> void;
        auto DoDebugEndFrame() -> void;
#endif

    public:
        CK_PROPERTY_GET(_Partition);
        CK_PROPERTY_GET(_LastFramePumpCount);
        CK_PROPERTY_GET(_LastGraphBuildTimeMs);
        CK_PROPERTY(_MaxPumpIterations);

#if !UE_BUILD_SHIPPING
        CK_PROPERTY_GET(_DebugFrameHistory);
#endif
    };
}

// --------------------------------------------------------------------------------------------------------------------
