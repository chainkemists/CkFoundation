#pragma once

#include "CkProcessorGraph.h"
#include "CkSchedulerDebugData.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // LoadKernel is for the frames a CkSnapshot load spends rebuilding the world.
    enum class ECk_SchedulerTickScope : uint8
    {
        Full,
        LoadKernel,
    };

    // ----------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessorScheduler
    {
    public:
        CK_GENERATED_BODY(FProcessorScheduler);

        explicit FProcessorScheduler(
            FProcessorGraphPartition&& InPartition);

    public:
        auto Tick(FCk_Time InDeltaTime, const FCk_Registry& InRegistry,
            ECk_SchedulerTickScope InScope = ECk_SchedulerTickScope::Full) -> void;

        auto Get_IsTickInProgress() const -> bool;

    private:
        auto DoPump(
            const FCk_Registry& InRegistry,
            int32 InPumpIndex,
            ECk_SchedulerTickScope InScope) -> bool;

        auto DoLogPumpLimitReached(
            const FCk_Registry& InRegistry,
            double InNow) -> void;

    private:
        FProcessorGraphPartition _Partition;

        // Precomputed at construction; _Partition._ExecutionOrder stays intact for diagnostics/debugger use.
        TArray<int32> _MainPassOrder;
        TArray<int32> _PumpOrder;

        TArray<int32> _LoadPassOrder;
        TArray<int32> _LoadPumpOrder;

        int32 _MaxPumpIterations = 30;
        bool _IsTickInProgress = false;

        // Cached from UCk_Ecs_ProjectSettings_UE at construction — neither is expected to change at runtime.
        bool _UseDirtyMarkerVersionShortCircuit = false;
        bool _UseEmptyViewMainPassSkip = false;

    private:
        int32 _LastFramePumpCount = 0;
        double _LastGraphBuildTimeMs = 0.0;

        double _LastPumpWarningTime = 0.0;
        TArray<FName> _LastWarnedStillDirtyNames;

#if !UE_BUILD_SHIPPING
    private:
        int32 _DebugFrameHistoryMax = 300;
        TArray<FSchedulerDebug_FrameSnapshot> _DebugFrameHistory;
        FSchedulerDebug_FrameSnapshot _DebugCurrentFrame;

        auto DoDebugBeginFrame() -> void;
        auto DoDebugRecordProcessorTick(int32 InNodeIndex, double InElapsedMs, int32 InEntityCount) -> void;
        auto DoDebugRecordProcessorPump(int32 InNodeIndex, int32 InPumpPass, double InElapsedMs, int32 InEntityCount) -> void;
        auto DoDebugRecordProcessorSkippedEmptyView(int32 InNodeIndex) -> void;
        auto DoDebugEndFrame() -> void;
#endif

    public:
        CK_PROPERTY_GET(_Partition);
        CK_PROPERTY_GET(_LastFramePumpCount);
        CK_PROPERTY_GET(_LastGraphBuildTimeMs);
        CK_PROPERTY(_MaxPumpIterations);

#if !UE_BUILD_SHIPPING
        CK_PROPERTY_GET(_DebugFrameHistory);
        CK_PROPERTY(_DebugFrameHistoryMax);
#endif
    };
}

// --------------------------------------------------------------------------------------------------------------------
