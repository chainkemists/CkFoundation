#include "CkProcessorScheduler.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/Settings/CkEcs_Settings.h"

#include "CkProfile/Stats/CkStats.h"

#include "HAL/PlatformTime.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"
#endif

// --------------------------------------------------------------------------------------------------------------------

#if !UE_BUILD_SHIPPING
int32 ck::GDebug_LastProcessedEntityCount = 0;

static TAutoConsoleVariable<bool> CVar_SchedulerDebugTiming(
    TEXT("ck.Scheduler.DebugTiming"),
    true,
    TEXT("Collect per-processor wall-clock timings for the Scheduler Debugger (default on). Set 0 to skip ")
    TEXT("the 2x QueryPerformanceCounter-per-processor cost that otherwise shows as Scheduler::Dispatch self-time."),
    ECVF_Default);

static TAutoConsoleVariable<int32> CVar_SchedulerMaxPumpIterations(
    TEXT("ck.Scheduler.MaxPumpIterations"),
    30,
    TEXT("Per-frame pump-pass budget. Read at the top of every scheduler tick, so A/B measurements of ")
    TEXT("pump-cascade behaviour need no rebuild."),
    ECVF_Default);

static TAutoConsoleVariable<bool> CVar_SchedulerVerifyEmptyViewSkip(
    TEXT("ck.Scheduler.VerifyEmptyViewSkip"),
    false,
    TEXT("Re-scan every empty-view-skipped processor each frame and ensure the cached verdict still holds. ")
    TEXT("Catches registry write paths that bypass the mutation version counters. Dev diagnosis only."),
    ECVF_Default);
#endif

// --------------------------------------------------------------------------------------------------------------------

DECLARE_STATS_GROUP(TEXT("CkScheduler"), STATGROUP_CkScheduler, STATCAT_Advanced);

DECLARE_CYCLE_STAT(TEXT("Scheduler::MainPass"),          STAT_Scheduler_MainPass,          STATGROUP_CkScheduler);
DECLARE_CYCLE_STAT(TEXT("Scheduler::Dispatch"),          STAT_Scheduler_Dispatch,          STATGROUP_CkScheduler);
DECLARE_CYCLE_STAT(TEXT("Scheduler::Pump"),              STAT_Scheduler_Pump,              STATGROUP_CkScheduler);
DECLARE_CYCLE_STAT(TEXT("Scheduler::PumpDispatch"),      STAT_Scheduler_PumpDispatch,      STATGROUP_CkScheduler);
DECLARE_CYCLE_STAT(TEXT("Scheduler::PumpDirtyCheck"),    STAT_Scheduler_PumpDirtyCheck,    STATGROUP_CkScheduler);

DECLARE_CYCLE_STAT(TEXT("Scheduler::EmptyViewCheck"),    STAT_Scheduler_EmptyViewCheck,    STATGROUP_CkScheduler);
DECLARE_CYCLE_STAT(TEXT("Scheduler::DebugRecord"),       STAT_Scheduler_DebugRecord,       STATGROUP_CkScheduler);

// --------------------------------------------------------------------------------------------------------------------

static constexpr double GCk_Scheduler_PumpWarningThrottleSeconds = 5.0;

// --------------------------------------------------------------------------------------------------------------------

ck::FProcessorScheduler::
    FProcessorScheduler(
        FProcessorGraphPartition&& InPartition)
    : _Partition(MoveTemp(InPartition))
    , _UseDirtyMarkerVersionShortCircuit(UCk_Utils_Ecs_Settings_UE::Get_EnableDirtyMarkerPumpShortCircuit())
    , _UseEmptyViewMainPassSkip(UCk_Utils_Ecs_Settings_UE::Get_EnableEmptyViewMainPassSkip())
{
    for (const auto NodeIndex : _Partition._ExecutionOrder)
    {
        const auto& Node = _Partition._Nodes[NodeIndex];

        if (NOT Node._Instance.IsSet() or Node._IsGhost)
        { continue; }

        _MainPassOrder.Add(NodeIndex);

        const auto RunsDuringLoad = Node._LoadPolicy == ECk_ProcessorLoadPolicy::RunsDuringLoad;
        if (RunsDuringLoad)
        { _LoadPassOrder.Add(NodeIndex); }

        if (Node._HasDirtyMarker and Node._PumpPolicy != ECk_ProcessorPumpPolicy::SkipPump)
        {
            _PumpOrder.Add(NodeIndex);
            if (RunsDuringLoad)
            { _LoadPumpOrder.Add(NodeIndex); }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorScheduler::
    Tick(
        FCk_Time InDeltaTime,
        const FCk_Registry& InRegistry,
        ECk_SchedulerTickScope InScope)
    -> void
{
    _IsTickInProgress = true;

    const auto& MainOrder = InScope == ECk_SchedulerTickScope::LoadKernel ? _LoadPassOrder : _MainPassOrder;

#if !UE_BUILD_SHIPPING
    const auto DebugTimingEnabled = CVar_SchedulerDebugTiming.GetValueOnGameThread();
    _MaxPumpIterations = FMath::Max(1, CVar_SchedulerMaxPumpIterations.GetValueOnGameThread());
    DoDebugBeginFrame();
    const auto FrameStartTime = FPlatformTime::Seconds();
#endif

    {
        SCOPE_CYCLE_COUNTER(STAT_Scheduler_MainPass);
        for (const auto NodeIndex : MainOrder)
        {
            SCOPE_CYCLE_COUNTER(STAT_Scheduler_Dispatch);

            auto& Node = _Partition._Nodes[NodeIndex];

            if (_UseEmptyViewMainPassSkip and Node._CanSkipWhenViewEmpty)
            {
                SCOPE_CYCLE_COUNTER(STAT_Scheduler_EmptyViewCheck);

                auto IncludeVersionSum = uint64{0};
                for (const auto IncludeHash : Node._ViewIncludeHashes)
                {
                    IncludeVersionSum += InRegistry.Get_DirtyMarkerVersion(IncludeHash);
                }

                if (IncludeVersionSum != Node._LastSeenIncludeVersionSum)
                {
                    Node._LastSeenIncludeVersionSum = IncludeVersionSum;
                    Node._LastKnownViewProvablyEmpty = Node._IsViewProvablyEmpty(InRegistry);
                }

#if !UE_BUILD_SHIPPING
                if (Node._LastKnownViewProvablyEmpty and CVar_SchedulerVerifyEmptyViewSkip.GetValueOnGameThread())
                {
                    CK_ENSURE_IF_NOT(Node._IsViewProvablyEmpty(InRegistry),
                        TEXT("Empty-view skip verdict for processor [{}] is STALE — its view gained entities without any "
                             "include-type version bump. Some registry write path bypasses the mutation counters."),
                        Node._ProcessorName)
                    { Node._LastKnownViewProvablyEmpty = false; }
                }
#endif

                if (Node._LastKnownViewProvablyEmpty)
                {
#if !UE_BUILD_SHIPPING
                    DoDebugRecordProcessorSkippedEmptyView(NodeIndex);
#endif
                    continue;
                }
            }

#if !UE_BUILD_SHIPPING
            auto ProcessorStartTime = 0.0;
            if (DebugTimingEnabled)
            {
                SCOPE_CYCLE_COUNTER(STAT_Scheduler_DebugRecord);
                ProcessorStartTime = FPlatformTime::Seconds();
                ck::GDebug_LastProcessedEntityCount = 0;
            }
#endif

            {
#if CPUPROFILERTRACE_ENABLED
                constexpr auto TraceUnconditionally = true;
                FCpuProfilerTrace::FEventScope ProcessorTraceScope{
                    Node._TraceSpecId, Node._TraceName, TraceUnconditionally, __FILE__, __LINE__};
#endif
                (*Node._Instance)->Tick(InDeltaTime);
            }

#if !UE_BUILD_SHIPPING
            if (DebugTimingEnabled)
            {
                SCOPE_CYCLE_COUNTER(STAT_Scheduler_DebugRecord);
                const auto ProcessorElapsedMs = (FPlatformTime::Seconds() - ProcessorStartTime) * 1000.0;
                DoDebugRecordProcessorTick(NodeIndex, ProcessorElapsedMs, ck::GDebug_LastProcessedEntityCount);
            }
#endif
        }
    }

    {
        SCOPE_CYCLE_COUNTER(STAT_Scheduler_Pump);
        _LastFramePumpCount = 0;
        for (auto PumpIndex = 0; PumpIndex < _MaxPumpIterations; ++PumpIndex)
        {
            const auto AnotherPumpNeeded = DoPump(InRegistry, PumpIndex, InScope);
            if (NOT AnotherPumpNeeded)
            { break; }
            ++_LastFramePumpCount;
        }
    }

    constexpr auto WarnThreshold = 8;
    const auto Now = FPlatformTime::Seconds();
    if (_LastFramePumpCount >= _MaxPumpIterations)
    {
        DoLogPumpLimitReached(InRegistry, Now);
    }
    else if (_LastFramePumpCount >= WarnThreshold)
    {
        // Clearing the cached still-dirty set lets the limit-reached path re-log immediately if the
        // frame escalates from "high" to "at limit".
        if (Now - _LastPumpWarningTime >= GCk_Scheduler_PumpWarningThrottleSeconds)
        {
            _LastPumpWarningTime = Now;
            _LastWarnedStillDirtyNames.Reset();
            ck::ecs::Warning(TEXT("High pump count this frame: [{}]"), _LastFramePumpCount);
        }
    }

#if !UE_BUILD_SHIPPING
    _DebugCurrentFrame.PumpIterationCount = _LastFramePumpCount;
    _DebugCurrentFrame.TotalFrameTimeMs = (FPlatformTime::Seconds() - FrameStartTime) * 1000.0;
    DoDebugEndFrame();
#endif

    _IsTickInProgress = false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorScheduler::
    Get_IsTickInProgress() const
    -> bool
{
    return _IsTickInProgress;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorScheduler::
    DoPump(
        const FCk_Registry& InRegistry,
        int32 InPumpIndex,
        ECk_SchedulerTickScope InScope)
    -> bool
{
    auto AnyProcessorTicked = false;

    const auto& PumpOrder = InScope == ECk_SchedulerTickScope::LoadKernel ? _LoadPumpOrder : _PumpOrder;

#if !UE_BUILD_SHIPPING
    const auto DebugTimingEnabled = CVar_SchedulerDebugTiming.GetValueOnGameThread();
#endif

    for (const auto NodeIndex : PumpOrder)
    {
        auto& Node = _Partition._Nodes[NodeIndex];

        auto VersionBeforePump = uint64{0};
        if (_UseDirtyMarkerVersionShortCircuit)
        {
            // Each per-hash version is monotonic, so their sum is too — an unchanged sum means no
            // marker moved, in this frame or any prior one.
            for (const auto MarkerHash : Node._DirtyMarkerHashes)
            {
                VersionBeforePump += InRegistry.Get_DirtyMarkerVersion(MarkerHash);
            }

            if (VersionBeforePump == Node._LastSeenDirtyVersion)
            { continue; }

            auto IsDirty = false;
            {
                SCOPE_CYCLE_COUNTER(STAT_Scheduler_PumpDirtyCheck);
                IsDirty = Node._IsDirtyChecker(InRegistry);
            }
            if (NOT IsDirty)
            {
                Node._LastSeenDirtyVersion = VersionBeforePump;
                continue;
            }
        }
        else
        {
            auto IsDirty = false;
            {
                SCOPE_CYCLE_COUNTER(STAT_Scheduler_PumpDirtyCheck);
                IsDirty = Node._IsDirtyChecker(InRegistry);
            }
            if (NOT IsDirty)
            { continue; }
        }

        {
            SCOPE_CYCLE_COUNTER(STAT_Scheduler_PumpDispatch);

#if !UE_BUILD_SHIPPING
            auto PumpStartTime = 0.0;
            if (DebugTimingEnabled)
            {
                SCOPE_CYCLE_COUNTER(STAT_Scheduler_DebugRecord);
                PumpStartTime = FPlatformTime::Seconds();
                ck::GDebug_LastProcessedEntityCount = 0;
            }
#endif

            auto VisitedCount = int32{};
            {
#if CPUPROFILERTRACE_ENABLED
                constexpr auto TraceUnconditionally = true;
                FCpuProfilerTrace::FEventScope ProcessorTraceScope{
                    Node._TraceSpecId, Node._TraceName, TraceUnconditionally, __FILE__, __LINE__};
#endif
                VisitedCount = (*Node._Instance)->Pump();
            }

            // A pump that provably visited zero entities cannot have produced new work. -1 means the
            // processor's custom DoTick body doesn't report a count; treat it as having done work.
            if (VisitedCount != 0)
            {
                AnyProcessorTicked = true;
            }

#if !UE_BUILD_SHIPPING
            if (DebugTimingEnabled)
            {
                SCOPE_CYCLE_COUNTER(STAT_Scheduler_DebugRecord);
                const auto PumpElapsedMs = (FPlatformTime::Seconds() - PumpStartTime) * 1000.0;
                DoDebugRecordProcessorPump(NodeIndex, InPumpIndex, PumpElapsedMs, ck::GDebug_LastProcessedEntityCount);
            }
#endif

            if (_UseDirtyMarkerVersionShortCircuit)
            {
                // PRE-pump, deliberately: work this pump added recursively must still be observed by
                // the next pass. Storing the POST-pump version absorbs it and defers it a frame.
                Node._LastSeenDirtyVersion = VersionBeforePump;
            }
        }
    }

    return AnyProcessorTicked;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorScheduler::
    DoLogPumpLimitReached(
        const FCk_Registry& InRegistry,
        double InNow)
    -> void
{
    // _PumpOrder, not _ExecutionOrder: a SkipPump processor can never be pumped, so counting it inflates
    // "Still dirty" into a feature-coverage number instead of a pump-pressure one.
    auto StillDirtyNames = TArray<FName>{};
    auto StillDirtyNodeIndices = TArray<int32>{};
    for (const auto NodeIndex : _PumpOrder)
    {
        const auto& Node = _Partition._Nodes[NodeIndex];
        if (Node._IsDirtyChecker(InRegistry))
        {
            StillDirtyNames.Add(Node._ProcessorName);
            StillDirtyNodeIndices.Add(NodeIndex);
        }
    }

    // StillDirtyNames is gathered in stable _ExecutionOrder, so element-wise inequality is a real change.
    const auto SetChanged = StillDirtyNames != _LastWarnedStillDirtyNames;
    const auto ThrottleElapsed = InNow - _LastPumpWarningTime >= GCk_Scheduler_PumpWarningThrottleSeconds;
    if (NOT SetChanged and NOT ThrottleElapsed)
    { return; }

    _LastPumpWarningTime = InNow;
    _LastWarnedStillDirtyNames = StillDirtyNames;

    // The breakdown must ride INSIDE the header's message: log-suppression consumers match one
    // Contains pattern against a whole entry, and a separate "  - [Name]" line slips the pattern.
    auto Breakdown = FString{};
    constexpr auto ApproxCharsPerEntry = 60;
    Breakdown.Reserve(StillDirtyNames.Num() * ApproxCharsPerEntry);
    for (auto Index = 0; Index < StillDirtyNames.Num(); ++Index)
    {
        Breakdown += TEXT("\n  - [");
        Breakdown += StillDirtyNames[Index].ToString();
        Breakdown += TEXT("]");

#if !UE_BUILD_SHIPPING
        // The warning is emitted BEFORE DoDebugEndFrame, so the current-frame snapshot is still the live one.
        if (const auto NodeIndex = StillDirtyNodeIndices[Index];
            _DebugCurrentFrame.ProcessorTimings.IsValidIndex(NodeIndex))
        {
            Breakdown += ck::Format_UE(TEXT(" pumps={}"), _DebugCurrentFrame.ProcessorTimings[NodeIndex].PumpCountThisFrame);
        }
#endif
    }

    ck::ecs::Warning(TEXT("Pump limit [{}] reached. Still dirty: [{}]{}"),
        _MaxPumpIterations, StillDirtyNames.Num(), Breakdown);
}

// --------------------------------------------------------------------------------------------------------------------

#if !UE_BUILD_SHIPPING

auto
    ck::FProcessorScheduler::
    DoDebugBeginFrame()
    -> void
{
    _DebugCurrentFrame = FSchedulerDebug_FrameSnapshot{};
    _DebugCurrentFrame.FrameNumber = GFrameCounter;
    _DebugCurrentFrame.ProcessorTimings.SetNum(_Partition._Nodes.Num());

    for (auto Index = 0; Index < _Partition._Nodes.Num(); ++Index)
    {
        _DebugCurrentFrame.ProcessorTimings[Index].ProcessorName = _Partition._Nodes[Index]._ProcessorName;
    }
}

auto
    ck::FProcessorScheduler::
    DoDebugRecordProcessorTick(
        int32 InNodeIndex,
        double InElapsedMs,
        int32 InEntityCount)
    -> void
{
    if (NOT _DebugCurrentFrame.ProcessorTimings.IsValidIndex(InNodeIndex))
    { return; }

    _DebugCurrentFrame.ProcessorTimings[InNodeIndex].MainPassTimeMs = InElapsedMs;
    _DebugCurrentFrame.ProcessorTimings[InNodeIndex].MainPassEntityCount = InEntityCount;
}

auto
    ck::FProcessorScheduler::
    DoDebugRecordProcessorSkippedEmptyView(
        int32 InNodeIndex)
    -> void
{
    if (NOT _DebugCurrentFrame.ProcessorTimings.IsValidIndex(InNodeIndex))
    { return; }

    _DebugCurrentFrame.ProcessorTimings[InNodeIndex].WasSkippedEmptyViewThisFrame = true;
    ++_DebugCurrentFrame.SkippedEmptyViewCount;
}

auto
    ck::FProcessorScheduler::
    DoDebugRecordProcessorPump(
        int32 InNodeIndex,
        int32 InPumpPass,
        double InElapsedMs,
        int32 InEntityCount)
    -> void
{
    if (NOT _DebugCurrentFrame.ProcessorTimings.IsValidIndex(InNodeIndex))
    { return; }

    auto& Timing = _DebugCurrentFrame.ProcessorTimings[InNodeIndex];
    Timing.WasDirtyThisFrame = true;
    ++Timing.PumpCountThisFrame;

    while (Timing.PumpPassTimesMs.Num() <= InPumpPass)
    {
        Timing.PumpPassTimesMs.Add(0.0);
    }
    Timing.PumpPassTimesMs[InPumpPass] = InElapsedMs;

    while (Timing.PumpPassEntityCounts.Num() <= InPumpPass)
    {
        Timing.PumpPassEntityCounts.Add(0);
    }
    Timing.PumpPassEntityCounts[InPumpPass] = InEntityCount;
}

auto
    ck::FProcessorScheduler::
    DoDebugEndFrame()
    -> void
{
    if (_DebugFrameHistory.Num() >= _DebugFrameHistoryMax)
    {
        _DebugFrameHistory.RemoveAt(0);
    }
    _DebugFrameHistory.Add(MoveTemp(_DebugCurrentFrame));
}

#endif // !UE_BUILD_SHIPPING

// --------------------------------------------------------------------------------------------------------------------
