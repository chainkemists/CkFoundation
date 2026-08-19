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
    false,
    TEXT("FORCE per-processor wall-clock TIMING collection on. Off by default because timing is now ")
    TEXT("demand-driven: the 2x clock read per processor runs only while something is reading the ")
    TEXT("frame history (the Scheduler Debugger while its window is open). Entity counts, pump ")
    TEXT("counts, dirty flags and empty-view skips are ALWAYS recorded — only the elapsed-ms ")
    TEXT("numbers go dark. Set 1 to capture timings for a window you will inspect afterwards."),
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
DECLARE_CYCLE_STAT(TEXT("Scheduler::LocalSettle"),       STAT_Scheduler_LocalSettle,       STATGROUP_CkScheduler);

DECLARE_CYCLE_STAT(TEXT("Scheduler::EmptyViewCheck"),    STAT_Scheduler_EmptyViewCheck,    STATGROUP_CkScheduler);
DECLARE_CYCLE_STAT(TEXT("Scheduler::DebugRecord"),       STAT_Scheduler_DebugRecord,       STATGROUP_CkScheduler);

// --------------------------------------------------------------------------------------------------------------------

static constexpr double GCk_Scheduler_PumpWarningThrottleSeconds = 5.0;

namespace ck_processor_scheduler
{
    // Keep-alive window for timing collection after a history read. Sized ABOVE the slowest
    // consumer cadence: the Scheduler Debugger reads through FCkDebuggerRefreshGate, whose Hz5
    // rate lands roughly every 12 frames at 60fps (Hz15 every 4 — exactly on a 4-frame boundary,
    // so a 4-frame window would flicker). Over-covering only extends collection while a consumer
    // is genuinely polling.
    constexpr uint64 DebugTimingKeepAliveFrames = 24;
}

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

    auto PlanIndexByGroup = TMap<FName, int32>{};

    for (const auto NodeIndex : _Partition._ExecutionOrder)
    {
        const auto& Node = _Partition._Nodes[NodeIndex];
        if (Node._IsGhost or NOT Node._Instance.IsSet() or Node._LocalSettleAfterGroupName.IsNone())
        { continue; }

        auto* PlanIndex = PlanIndexByGroup.Find(Node._LocalSettleAfterGroupName);
        if (PlanIndex == nullptr)
        {
            const auto NewPlanIndex = _LocalSettlePlans.Emplace();
            auto& NewPlan = _LocalSettlePlans[NewPlanIndex];
            NewPlan._AfterGroupName = Node._LocalSettleAfterGroupName;
            PlanIndexByGroup.Add(Node._LocalSettleAfterGroupName, NewPlanIndex);
            PlanIndex = PlanIndexByGroup.Find(Node._LocalSettleAfterGroupName);
        }

        auto& Plan = _LocalSettlePlans[*PlanIndex];
        Plan._ParticipantNodeIndices.Add(NodeIndex);
        if (Node._IsLocalSettleTrigger)
        {
            const auto TriggerHasDirtyMarker = Node._HasDirtyMarker;
            CK_ENSURE_IF_NOT(TriggerHasDirtyMarker,
                TEXT("Processor [{}] activates local settle after [{}] but has no dirty marker."),
                Node._ProcessorName, Plan._AfterGroupName)
            { }
            if (TriggerHasDirtyMarker)
            { Plan._TriggerNodeIndices.Add(NodeIndex); }
            else
            { Plan._IsValid = false; }
        }

        if (Node._PumpPolicy == ECk_ProcessorPumpPolicy::SkipPump)
        {
            CK_TRIGGER_ENSURE(TEXT("Processor [{}] opts into local settle after [{}] but declares SkipPump. "
                                   "Local-settle participants must be safe to replay with DeltaT=0."),
                Node._ProcessorName, Plan._AfterGroupName);
            Plan._IsValid = false;
        }
    }

    auto MainPassInsertIndex = int32{0};
    auto LoadPassInsertIndex = int32{0};
    for (const auto NodeIndex : _Partition._ExecutionOrder)
    {
        const auto& Node = _Partition._Nodes[NodeIndex];

        if (Node._IsGroupEnd and Node._PairedGroupNodeIndex != INDEX_NONE)
        {
            const auto& GroupStart = _Partition._Nodes[Node._PairedGroupNodeIndex];
            if (const auto* PlanIndex = PlanIndexByGroup.Find(GroupStart._ProcessorName))
            {
                _LocalSettlePlans[*PlanIndex]._MainPassInsertIndex = MainPassInsertIndex;
                _LocalSettlePlans[*PlanIndex]._LoadPassInsertIndex = LoadPassInsertIndex;
            }
        }

        if (Node._Instance.IsSet() and NOT Node._IsGhost)
        {
            ++MainPassInsertIndex;
            if (Node._LoadPolicy == ECk_ProcessorLoadPolicy::RunsDuringLoad)
            { ++LoadPassInsertIndex; }
        }
    }

    for (auto& Plan : _LocalSettlePlans)
    {
        const auto HasAnchor = Plan._MainPassInsertIndex != INDEX_NONE;
        CK_ENSURE_IF_NOT(HasAnchor,
            TEXT("Local-settle participants name group [{}], but that group has no boundary in this scheduler partition."),
            Plan._AfterGroupName)
        { }
        if (NOT HasAnchor)
        { Plan._IsValid = false; }

        const auto HasTrigger = NOT Plan._TriggerNodeIndices.IsEmpty();
        CK_ENSURE_IF_NOT(HasTrigger,
            TEXT("Local-settle plan after [{}] has no explicit consumed-marker trigger."),
            Plan._AfterGroupName)
        { }
        if (NOT HasTrigger)
        { Plan._IsValid = false; }

        Plan._RunsDuringLoad = NOT Plan._ParticipantNodeIndices.IsEmpty();
        for (const auto ParticipantNodeIndex : Plan._ParticipantNodeIndices)
        {
            const auto& ParticipantNode = _Partition._Nodes[ParticipantNodeIndex];
            Plan._RunsDuringLoad &= ParticipantNode._LoadPolicy == ECk_ProcessorLoadPolicy::RunsDuringLoad;

            const auto ParticipantMainPassIndex = _MainPassOrder.Find(ParticipantNodeIndex);
            const auto ParticipantPrecedesBarrier = ParticipantMainPassIndex != INDEX_NONE
                and ParticipantMainPassIndex < Plan._MainPassInsertIndex;
            CK_ENSURE_IF_NOT(ParticipantPrecedesBarrier,
                TEXT("Local-settle participant [{}] must run before its [{}] barrier in the main graph."),
                ParticipantNode._ProcessorName, Plan._AfterGroupName)
            { }
            if (NOT ParticipantPrecedesBarrier)
            { Plan._IsValid = false; }

            if (Plan._RunsDuringLoad)
            {
                const auto ParticipantLoadPassIndex = _LoadPassOrder.Find(ParticipantNodeIndex);
                const auto ParticipantPrecedesLoadBarrier = ParticipantLoadPassIndex != INDEX_NONE
                    and ParticipantLoadPassIndex < Plan._LoadPassInsertIndex;
                CK_ENSURE_IF_NOT(ParticipantPrecedesLoadBarrier,
                    TEXT("Load-capable local-settle participant [{}] must run before its [{}] barrier in the load graph."),
                    ParticipantNode._ProcessorName, Plan._AfterGroupName)
                { }
                if (NOT ParticipantPrecedesLoadBarrier)
                { Plan._IsValid = false; }
            }
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
    _LastFramePumpCount = 0;

#if !UE_BUILD_SHIPPING
    const auto DebugTimingEnabled = Get_IsDebugTimingWanted();
    _MaxPumpIterations = FMath::Max(1, CVar_SchedulerMaxPumpIterations.GetValueOnGameThread());
    DoDebugBeginFrame();
    const auto FrameStartTime = FPlatformTime::Seconds();
#endif

    {
        SCOPE_CYCLE_COUNTER(STAT_Scheduler_MainPass);
        for (auto MainPassIndex = 0; MainPassIndex <= MainOrder.Num(); ++MainPassIndex)
        {
            DoRunLocalSettleBarriers(MainPassIndex, InRegistry, InScope);

            if (MainPassIndex == MainOrder.Num())
            { break; }

            SCOPE_CYCLE_COUNTER(STAT_Scheduler_Dispatch);

            const auto NodeIndex = MainOrder[MainPassIndex];
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
            // The entity counter is reset unconditionally: it is a plain int32 store, and the
            // visited-count it feeds is a correctness signal that tests and the debugger read for
            // PAST frames. Only the clock reads below are demand-driven.
            ck::GDebug_LastProcessedEntityCount = 0;

            auto ProcessorStartTime = 0.0;
            if (DebugTimingEnabled)
            {
                SCOPE_CYCLE_COUNTER(STAT_Scheduler_DebugRecord);
                ProcessorStartTime = FPlatformTime::Seconds();
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
            const auto ProcessorElapsedMs = DebugTimingEnabled
                ? (FPlatformTime::Seconds() - ProcessorStartTime) * 1000.0
                : 0.0;

            DoDebugRecordProcessorTick(NodeIndex, ProcessorElapsedMs, ck::GDebug_LastProcessedEntityCount);
#endif
        }
    }

    {
        SCOPE_CYCLE_COUNTER(STAT_Scheduler_Pump);
        for (; _LastFramePumpCount < _MaxPumpIterations; ++_LastFramePumpCount)
        {
            const auto AnotherPumpNeeded = DoPump(InRegistry, _LastFramePumpCount, InScope);
            if (NOT AnotherPumpNeeded)
            { break; }
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
    DoRunLocalSettleBarriers(
        int32 InMainPassInsertIndex,
        const FCk_Registry& InRegistry,
        ECk_SchedulerTickScope InScope)
    -> void
{
    for (auto& Plan : _LocalSettlePlans)
    {
        const auto BarrierInsertIndex = InScope == ECk_SchedulerTickScope::LoadKernel
            ? Plan._LoadPassInsertIndex
            : Plan._MainPassInsertIndex;
        if (NOT Plan._IsValid or BarrierInsertIndex != InMainPassInsertIndex)
        { continue; }

        if (InScope == ECk_SchedulerTickScope::LoadKernel and NOT Plan._RunsDuringLoad)
        { continue; }

        DoRunLocalSettle(Plan, InRegistry);
    }
}

auto
    ck::FProcessorScheduler::
    DoRunLocalSettle(
        FProcessorLocalSettlePlan& InPlan,
        const FCk_Registry& InRegistry)
    -> void
{
    if (NOT DoHasDirtyLocalSettleTrigger(InPlan, InRegistry))
    { return; }

    SCOPE_CYCLE_COUNTER(STAT_Scheduler_LocalSettle);

    while (_LastFramePumpCount < _MaxPumpIterations)
    {
        if (NOT DoHasDirtyLocalSettleTrigger(InPlan, InRegistry))
        { return; }

        const auto SettlePassIndex = _LastFramePumpCount;

        for (const auto NodeIndex : InPlan._ParticipantNodeIndices)
        {
            auto& Node = _Partition._Nodes[NodeIndex];
            if (Node._HasDirtyMarker and NOT Node._IsDirtyChecker(InRegistry))
            { continue; }

#if !UE_BUILD_SHIPPING
            const auto DebugTimingEnabled = CVar_SchedulerDebugTiming.GetValueOnGameThread();
            auto ProcessorStartTime = 0.0;
            if (DebugTimingEnabled)
            {
                SCOPE_CYCLE_COUNTER(STAT_Scheduler_DebugRecord);
                ProcessorStartTime = FPlatformTime::Seconds();
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

#if !UE_BUILD_SHIPPING
            if (DebugTimingEnabled)
            {
                SCOPE_CYCLE_COUNTER(STAT_Scheduler_DebugRecord);
                const auto ProcessorElapsedMs = (FPlatformTime::Seconds() - ProcessorStartTime) * 1000.0;
                DoDebugRecordProcessorPump(NodeIndex, SettlePassIndex, ProcessorElapsedMs,
                    VisitedCount >= 0 ? VisitedCount : ck::GDebug_LastProcessedEntityCount);
            }
#endif
        }

        ++_LastFramePumpCount;
    }

    if (NOT DoHasDirtyLocalSettleTrigger(InPlan, InRegistry))
    { return; }

    auto StillDirtyNames = TArray<FName>{};
    for (const auto TriggerNodeIndex : InPlan._TriggerNodeIndices)
    {
        const auto& TriggerNode = _Partition._Nodes[TriggerNodeIndex];
        if (TriggerNode._IsDirtyChecker(InRegistry))
        { StillDirtyNames.Add(TriggerNode._ProcessorName); }
    }

    auto StillDirtyBreakdown = FString{};
    for (const auto& StillDirtyName : StillDirtyNames)
    {
        if (NOT StillDirtyBreakdown.IsEmpty())
        { StillDirtyBreakdown += TEXT(", "); }
        StillDirtyBreakdown += StillDirtyName.ToString();
    }

    ck::ecs::Warning(TEXT("Local settle after group [{}] reached the [{}]-pass limit. Still dirty: [{}]. "
                          "Work remains queued for the next frame."),
        InPlan._AfterGroupName, _MaxPumpIterations, StillDirtyBreakdown);
}

auto
    ck::FProcessorScheduler::
    DoHasDirtyLocalSettleTrigger(
        const FProcessorLocalSettlePlan& InPlan,
        const FCk_Registry& InRegistry) const
    -> bool
{
    for (const auto TriggerNodeIndex : InPlan._TriggerNodeIndices)
    {
        const auto& TriggerNode = _Partition._Nodes[TriggerNodeIndex];
        if (TriggerNode._IsDirtyChecker(InRegistry))
        { return true; }
    }

    return false;
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
    const auto DebugTimingEnabled = Get_IsDebugTimingWanted();
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
            ck::GDebug_LastProcessedEntityCount = 0;

            auto PumpStartTime = 0.0;
            if (DebugTimingEnabled)
            {
                SCOPE_CYCLE_COUNTER(STAT_Scheduler_DebugRecord);
                PumpStartTime = FPlatformTime::Seconds();
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

            // A pump that PROVABLY visited zero entities cannot have produced new work. -1 is not that proof:
            // it is the sentinel a custom DoTick body leaves when it never reported a count, and it is treated
            // as work because such a body may have done registry-wide work no view count describes. Eleven
            // processors sit in that family today and each of them keeps this loop awake for a pass — the
            // census, and what inverting the default would cost, are in CkEcs/CLAUDE.md § "The -1 visited-count
            // contract" (TProcessorBase::_LastVisitedCount carries the author-facing half).
            if (VisitedCount != 0)
            {
                AnyProcessorTicked = true;
            }

#if !UE_BUILD_SHIPPING
            const auto PumpElapsedMs = DebugTimingEnabled
                ? (FPlatformTime::Seconds() - PumpStartTime) * 1000.0
                : 0.0;

            DoDebugRecordProcessorPump(NodeIndex, InPumpIndex, PumpElapsedMs, ck::GDebug_LastProcessedEntityCount);
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


    if (StillDirtyNames.IsEmpty())
    {
        _LastWarnedStillDirtyNames.Reset();
        return;
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
    Get_DebugFrameHistory() const
    -> const TArray<FSchedulerDebug_FrameSnapshot>&
{
    _LastDebugHistoryReadFrame = GFrameCounter;

    return _DebugFrameHistory;
}

auto
    ck::FProcessorScheduler::
    Get_IsDebugTimingWanted() const
    -> bool
{
    if (CVar_SchedulerDebugTiming.GetValueOnGameThread())
    { return true; }

    // Never read leaves the stamp at 0, so the delta is GFrameCounter itself and this reports
    // "nobody is looking" once the process is past the keep-alive window.
    return (GFrameCounter - _LastDebugHistoryReadFrame) <= ck_processor_scheduler::DebugTimingKeepAliveFrames;
}

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
