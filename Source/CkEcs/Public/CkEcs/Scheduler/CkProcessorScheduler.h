#pragma once

#include "CkProcessorGraph.h"
#include "CkSchedulerDebugData.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Selects which precomputed order the scheduler iterates this tick. Full = the whole graph (normal frames);
    // LoadKernel = only RunsDuringLoad nodes, for the frames a CkSnapshot load spends rebuilding the world (spec §4.3).
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

        // Precomputed at construction from _Partition._ExecutionOrder (which stays intact for
        // diagnostics/debugger use): _MainPassOrder holds the instantiated, non-ghost nodes the
        // main pass dispatches; _PumpOrder additionally requires a dirty marker and no SkipPump
        // opt-out. Avoids re-scanning and branch-skipping the full node list every pass.
        TArray<int32> _MainPassOrder;
        TArray<int32> _PumpOrder;

        // Load-kernel subsets of the two orders above: the RunsDuringLoad nodes only (spec §4.3). Iterated by
        // Tick/DoPump when InScope == LoadKernel — the frames a CkSnapshot load spends rebuilding the world.
        TArray<int32> _LoadPassOrder;
        TArray<int32> _LoadPumpOrder;

        int32 _MaxPumpIterations = 30;
        bool _IsTickInProgress = false;

        // Cached once at construction from UCk_Ecs_ProjectSettings_UE — this setting is not expected
        // to change at runtime, so reading it per-Tick (let alone per-pump) would be wasteful.
        bool _UseDirtyMarkerVersionShortCircuit = false;

        // Cached like the above. Gates the main pass' empty-view skip (see ECk_ProcessorEmptyViewPolicy
        // in CkProcessorDescriptor.h): eligible nodes whose view is provably empty are not dispatched.
        bool _UseEmptyViewMainPassSkip = false;

    private:
        int32 _LastFramePumpCount = 0;
        double _LastGraphBuildTimeMs = 0.0;

        // Throttle state for the pump-pressure warnings (see GCk_Scheduler_PumpWarningThrottleSeconds in the .cpp).
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
