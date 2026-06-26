#include "CkProcessorScheduler.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/Settings/CkEcs_Settings.h"

#include "CkProfile/Stats/CkStats.h"

#include "HAL/PlatformTime.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"
#endif

// --------------------------------------------------------------------------------------------------------------------

#if !UE_BUILD_SHIPPING
int32 ck::GDebug_LastProcessedEntityCount = 0;

// Gate for the Scheduler Debugger's per-processor wall-clock timing. That timing costs TWO
// FPlatformTime::Seconds() (QueryPerformanceCounter) calls per processor per frame and is REDUNDANT
// with the stat system (STAT_Tick already times every processor). On machines with slow QPC it is the
// dominant unattributed cost inside Scheduler::Dispatch (the "gap" between processor scopes). Default on
// to preserve the debugger; set `ck.Scheduler.DebugTiming 0` to eliminate it while profiling.
static TAutoConsoleVariable<bool> CVar_SchedulerDebugTiming(
    TEXT("ck.Scheduler.DebugTiming"),
    true,
    TEXT("Collect per-processor wall-clock timings for the Scheduler Debugger (default on). Set 0 to skip ")
    TEXT("the 2x QueryPerformanceCounter-per-processor cost that otherwise shows as Scheduler::Dispatch self-time."),
    ECVF_Default);
#endif

// --------------------------------------------------------------------------------------------------------------------

// Scheduler orchestration stats. The per-processor STAT_Tick / STAT_ForEachEntity scopes
// (STATGROUP_CkProcessors) nest INSIDE these, so the self-time of each scope below is exactly the
// "gap" work the scheduler does between/around processors: the dispatch loop, the dev-build per-processor
// timing capture, the pump passes, and the per-pump dirty-marker rescans.
DECLARE_STATS_GROUP(TEXT("CkScheduler"), STATGROUP_CkScheduler, STATCAT_Advanced);

DECLARE_CYCLE_STAT(TEXT("Scheduler::ResetPumpVersions"), STAT_Scheduler_ResetPumpVersions, STATGROUP_CkScheduler);
DECLARE_CYCLE_STAT(TEXT("Scheduler::MainPass"),          STAT_Scheduler_MainPass,          STATGROUP_CkScheduler);
DECLARE_CYCLE_STAT(TEXT("Scheduler::Dispatch"),          STAT_Scheduler_Dispatch,          STATGROUP_CkScheduler);
DECLARE_CYCLE_STAT(TEXT("Scheduler::Pump"),              STAT_Scheduler_Pump,              STATGROUP_CkScheduler);
DECLARE_CYCLE_STAT(TEXT("Scheduler::PumpDispatch"),      STAT_Scheduler_PumpDispatch,      STATGROUP_CkScheduler);
DECLARE_CYCLE_STAT(TEXT("Scheduler::PumpDirtyCheck"),    STAT_Scheduler_PumpDirtyCheck,    STATGROUP_CkScheduler);

// Dev-only: the Scheduler Debugger's per-processor QueryPerformanceCounter timing. Carved out of
// Scheduler::Dispatch self-time so you can SEE how much of the inter-processor "gap" is this redundant
// dev measurement (eliminable via `ck.Scheduler.DebugTiming 0`) vs. the inherent dispatch-loop cost
// (node fetch + entt::poly call) that remains as Dispatch self-time. Zero cost when DebugTiming is off.
DECLARE_CYCLE_STAT(TEXT("Scheduler::DebugRecord"),       STAT_Scheduler_DebugRecord,       STATGROUP_CkScheduler);

// --------------------------------------------------------------------------------------------------------------------

// Wall-clock throttle for the pump-pressure warnings. A persistently over-budget frame used to log
// EVERY frame (one line + one per still-dirty processor) — that spam is now surfaced live in CkWatermark.
// These logs are retained for dedicated/headless servers where no watermark renders; they fire at most
// once per this interval, with a still-dirty-SET change bypassing the throttle so a newly-stuck processor
// surfaces promptly. Unique name avoids unity-build collisions across translation units.
static constexpr double GCk_Scheduler_PumpWarningThrottleSeconds = 5.0;

// --------------------------------------------------------------------------------------------------------------------

ck::FProcessorScheduler::
    FProcessorScheduler(
        FProcessorGraphPartition&& InPartition)
    : _Partition(MoveTemp(InPartition))
    , _UseDirtyMarkerVersionShortCircuit(UCk_Utils_Ecs_Settings_UE::Get_EnableDirtyMarkerPumpShortCircuit())
{
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorScheduler::
    Tick(
        FCk_Time InDeltaTime,
        const FCk_Registry& InRegistry)
    -> void
{
    _IsTickInProgress = true;

#if !UE_BUILD_SHIPPING
    const auto DebugTimingEnabled = CVar_SchedulerDebugTiming.GetValueOnGameThread();
    DoDebugBeginFrame();
    const auto FrameStartTime = FPlatformTime::Seconds();
#endif

    // Reset per-node pump version cache for this frame. Only meaningful when the short-circuit is
    // enabled (see _UseDirtyMarkerVersionShortCircuit, cached at construction), but the reset is
    // cheap enough that there's no point branching on the flag here.
    {
        SCOPE_CYCLE_COUNTER(STAT_Scheduler_ResetPumpVersions);
        for (auto& Node : _Partition._Nodes)
        {
            if (Node._HasDirtyMarker)
            {
                Node._LastSeenDirtyVersion = 0;
            }
        }
    }

    // Main pass — every processor ticks once. Each processor's own STAT_Tick scope nests inside;
    // STAT_Scheduler_MainPass self-time is the inter-processor dispatch + dev-build timing capture
    // (the gaps between processor scopes on the scheduler track).
    {
        SCOPE_CYCLE_COUNTER(STAT_Scheduler_MainPass);
        for (const auto NodeIndex : _Partition._ExecutionOrder)
        {
            // Per-node dispatch — covers EVERY node (incl. ghost/skipped ones that emit no processor box),
            // so the inter-processor "gaps" on the scheduler track become labeled Dispatch self-time.
            // In a dev build that self-time is dominated by the scheduler-debugger's own per-processor
            // FPlatformTime timing below (redundant with STAT_Tick; compiled out in Test/Shipping).
            SCOPE_CYCLE_COUNTER(STAT_Scheduler_Dispatch);

            auto& Node = _Partition._Nodes[NodeIndex];
            if (Node._Instance.IsSet() and not Node._IsGhost)
            {
#if !UE_BUILD_SHIPPING
                auto ProcessorStartTime = 0.0;
                if (DebugTimingEnabled)
                {
                    SCOPE_CYCLE_COUNTER(STAT_Scheduler_DebugRecord);
                    ProcessorStartTime = FPlatformTime::Seconds();
                    ck::GDebug_LastProcessedEntityCount = 0;
                }
#endif

                (*Node._Instance)->Tick(InDeltaTime);

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
    }

    // Pump passes — drain cascading reactive work. Each processor's Pump scope nests inside;
    // STAT_Scheduler_Pump self-time (minus STAT_Scheduler_PumpDirtyCheck) is the pump-loop overhead.
    {
        SCOPE_CYCLE_COUNTER(STAT_Scheduler_Pump);
        _LastFramePumpCount = 0;
        for (auto PumpIndex = 0; PumpIndex < _MaxPumpIterations; ++PumpIndex)
        {
            const auto AnotherPumpNeeded = DoPump(InRegistry, PumpIndex);
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
        // Throttled — see GCk_Scheduler_PumpWarningThrottleSeconds. Clear the cached still-dirty set so the
        // limit-reached path re-logs immediately if the frame escalates from "high" to "at limit".
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
        int32 InPumpIndex)
    -> bool
{
    auto AnyProcessorTicked = false;

#if !UE_BUILD_SHIPPING
    const auto DebugTimingEnabled = CVar_SchedulerDebugTiming.GetValueOnGameThread();
#endif

    for (const auto NodeIndex : _Partition._ExecutionOrder)
    {
        auto& Node = _Partition._Nodes[NodeIndex];

        if (NOT Node._HasDirtyMarker)
        { continue; }

        // Explicit opt-out: time-stepping consumers (apply-offset, etc.) keep MarkedDirtyBy for skip-when-empty
        // diagnostics + scheduler edges, but must not run with DeltaT=0 because they re-apply cached state.
        // See ECk_ProcessorPumpPolicy doc in CkProcessorDescriptor.h.
        if (Node._PumpPolicy == ECk_ProcessorPumpPolicy::SkipPump)
        { continue; }

        // Short-circuit path (opt-in, cached at scheduler construction): if the registry's dirty-marker
        // version hasn't changed since our last observation, neither the count nor the contents of
        // the marker fragment could have moved. Skip the O(fragment-storage) Has_AnyEntityWith scan.
        auto VersionBeforePump = uint64{0};
        if (_UseDirtyMarkerVersionShortCircuit)
        {
            // Sum across all marker hashes (multi-marker composites). Each per-hash version is
            // monotonic, so the sum is too — an unchanged sum means no marker moved.
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
                // Nothing dirty right now — remember the version so we don't scan again until
                // another mutation happens.
                Node._LastSeenDirtyVersion = VersionBeforePump;
                continue;
            }
        }
        else
        {
            // Legacy path — rescan every pump pass. Behavior is identical to pre-short-circuit.
            auto IsDirty = false;
            {
                SCOPE_CYCLE_COUNTER(STAT_Scheduler_PumpDirtyCheck);
                IsDirty = Node._IsDirtyChecker(InRegistry);
            }
            if (NOT IsDirty)
            { continue; }
        }

        if (Node._Instance.IsSet() and not Node._IsGhost)
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

            (*Node._Instance)->Pump();
            AnyProcessorTicked = true;

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
                // Store the PRE-pump version. If this pump (or any callee it invoked synchronously,
                // e.g. EntityScript Construct → DefineState → AddTask queueing a new SpawnEntity
                // request) bumped the marker version, the next pump pass will observe CurrentVersion
                // > LastSeen and re-fire to drain the freshly-added work. Storing the POST-pump
                // version would absorb the processor's own recursively-added work into LastSeen and
                // defer it until next frame's main Tick — that's exactly the cascade bug.
                Node._LastSeenDirtyVersion = VersionBeforePump;
            }
        }
        else if (_UseDirtyMarkerVersionShortCircuit)
        {
            Node._LastSeenDirtyVersion = VersionBeforePump;
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
    auto StillDirtyNames = TArray<FName>{};
    for (const auto NodeIndex : _Partition._ExecutionOrder)
    {
        const auto& Node = _Partition._Nodes[NodeIndex];
        if (Node._HasDirtyMarker and Node._IsDirtyChecker(InRegistry))
        {
            StillDirtyNames.Add(Node._ProcessorName);
        }
    }

    // Re-log only when the throttle interval has elapsed OR the set of still-dirty processors changed
    // (StillDirtyNames is gathered in stable _ExecutionOrder, so element-wise inequality == a real change).
    const auto SetChanged = StillDirtyNames != _LastWarnedStillDirtyNames;
    const auto ThrottleElapsed = InNow - _LastPumpWarningTime >= GCk_Scheduler_PumpWarningThrottleSeconds;
    if (NOT SetChanged and NOT ThrottleElapsed)
    { return; }

    _LastPumpWarningTime = InNow;
    _LastWarnedStillDirtyNames = StillDirtyNames;

    ck::ecs::Warning(TEXT("Pump limit [{}] reached. Still dirty: [{}]"),
        _MaxPumpIterations, StillDirtyNames.Num());

    for (const auto& Name : StillDirtyNames)
    {
        ck::ecs::Warning(TEXT("  - [{}]"), Name);
    }
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
