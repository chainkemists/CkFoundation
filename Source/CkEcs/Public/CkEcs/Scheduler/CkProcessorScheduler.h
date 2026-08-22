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

    struct FProcessorLocalSettlePlan
    {
        CK_GENERATED_BODY(FProcessorLocalSettlePlan);

        FName _AfterGroupName;
        int32 _MainPassInsertIndex = INDEX_NONE;
        int32 _LoadPassInsertIndex = INDEX_NONE;
        bool _RunsDuringLoad = false;
        bool _IsValid = true;

        TArray<int32> _ParticipantNodeIndices;
        TArray<int32> _TriggerNodeIndices;
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

        auto DoRunLocalSettleBarriers(
            int32 InMainPassInsertIndex,
            const FCk_Registry& InRegistry,
            ECk_SchedulerTickScope InScope) -> void;

        auto DoRunLocalSettle(
            FProcessorLocalSettlePlan& InPlan,
            const FCk_Registry& InRegistry) -> void;

        auto DoHasDirtyLocalSettleTrigger(
            const FProcessorLocalSettlePlan& InPlan,
            const FCk_Registry& InRegistry) const -> bool;

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

        TArray<FProcessorLocalSettlePlan> _LocalSettlePlans;

        int32 _MaxPumpIterations = 30;
        bool _IsTickInProgress = false;

        // Cached from UCk_Ecs_ProjectSettings_UE at construction — neither is expected to change at runtime.
        bool _UseDirtyMarkerVersionShortCircuit = false;
        bool _UseEmptyViewMainPassSkip = false;

    private:
        // SHARED BUDGET, not a report: the global pump loop and every group-local settle loop both
        // advance this so the two together respect _MaxPumpIterations. What each spent separately is
        // what consumers actually want, so local settle keeps its own tally.
        int32 _LastFramePumpCount = 0;
        int32 _LastFrameLocalSettleCount = 0;
        double _LastGraphBuildTimeMs = 0.0;

        double _LastPumpWarningTime = 0.0;
        TArray<FName> _LastWarnedStillDirtyNames;

#if !UE_BUILD_SHIPPING
    private:
        int32 _DebugFrameHistoryMax = 300;
        TArray<FSchedulerDebug_FrameSnapshot> _DebugFrameHistory;
        FSchedulerDebug_FrameSnapshot _DebugCurrentFrame;

        // Stamped by Get_DebugFrameHistory so per-processor timing collection runs only while
        // something is actually reading it. Mutable because reading the history is logically const.
        mutable uint64 _LastDebugHistoryReadFrame = 0;

        auto DoDebugBeginFrame() -> void;
        auto DoDebugRecordProcessorTick(int32 InNodeIndex, double InElapsedMs, int32 InEntityCount) -> void;
        auto DoDebugRecordProcessorPump(int32 InNodeIndex, int32 InPumpPass, double InElapsedMs, int32 InEntityCount) -> void;
        auto DoDebugRecordProcessorSkippedEmptyView(int32 InNodeIndex) -> void;
        auto DoDebugEndFrame() -> void;
#endif

    public:
        CK_PROPERTY_GET(_Partition);
        CK_PROPERTY_GET(_LastFramePumpCount);
        CK_PROPERTY_GET(_LastFrameLocalSettleCount);
        CK_PROPERTY_GET(_LastGraphBuildTimeMs);
        CK_PROPERTY(_MaxPumpIterations);

#if !UE_BUILD_SHIPPING
        CK_PROPERTY(_DebugFrameHistoryMax);

        /**
         * Reading the history is what keeps per-processor timing collection alive, so this is
         * hand-written rather than CK_PROPERTY_GET. See Get_IsDebugTimingWanted.
         */
        auto
        Get_DebugFrameHistory() const
            -> const TArray<FSchedulerDebug_FrameSnapshot>&;

        /**
         * Whether to pay for per-processor wall-clock timing this frame.
         *
         * Collection costs two FPlatformTime::Seconds calls plus a record call per processor per
         * frame (and, in a STATS build, as many extra stat scopes) that nobody looks at on a normal
         * frame. It runs only while a consumer is reading the history — the Scheduler Debugger
         * polls it every frame its window is open — or while ck.Scheduler.DebugTiming forces it on.
         */
        auto
        Get_IsDebugTimingWanted() const
            -> bool;
#endif
    };
}

// --------------------------------------------------------------------------------------------------------------------
