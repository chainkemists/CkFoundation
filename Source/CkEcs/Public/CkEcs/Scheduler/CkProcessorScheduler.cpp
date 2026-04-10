#include "CkProcessorScheduler.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/Settings/CkEcs_Settings.h"

#include "CkProfile/Stats/CkStats.h"

#if !UE_BUILD_SHIPPING
#include "HAL/PlatformTime.h"
#endif

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
    DoDebugBeginFrame();
    const auto FrameStartTime = FPlatformTime::Seconds();
#endif

    // Reset per-node pump version cache for this frame. Only meaningful when the short-circuit is
    // enabled (see _UseDirtyMarkerVersionShortCircuit, cached at construction), but the reset is
    // cheap enough that there's no point branching on the flag here.
    for (auto& Node : _Partition._Nodes)
    {
        if (Node._HasDirtyMarker)
        {
            Node._LastSeenDirtyVersion = 0;
        }
    }

    for (const auto NodeIndex : _Partition._ExecutionOrder)
    {
        auto& Node = _Partition._Nodes[NodeIndex];
        if (Node._Instance.IsSet() and not Node._IsGhost)
        {
#if !UE_BUILD_SHIPPING
            const auto ProcessorStartTime = FPlatformTime::Seconds();
#endif

            (*Node._Instance)->Tick(InDeltaTime);

#if !UE_BUILD_SHIPPING
            const auto ProcessorElapsedMs = (FPlatformTime::Seconds() - ProcessorStartTime) * 1000.0;
            DoDebugRecordProcessorTick(NodeIndex, ProcessorElapsedMs);
#endif
        }
    }

    _LastFramePumpCount = 0;
    for (auto PumpIndex = 0; PumpIndex < _MaxPumpIterations; ++PumpIndex)
    {
        const auto AnotherPumpNeeded = DoPump(InRegistry, PumpIndex);
        if (NOT AnotherPumpNeeded)
        { break; }
        ++_LastFramePumpCount;
    }

    constexpr auto WarnThreshold = 8;
    if (_LastFramePumpCount >= _MaxPumpIterations)
    {
        DoLogPumpLimitReached(InRegistry);
    }
    else if (_LastFramePumpCount >= WarnThreshold)
    {
        ck::ecs::Warning(TEXT("High pump count this frame: [{}]"), _LastFramePumpCount);
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

    for (const auto NodeIndex : _Partition._ExecutionOrder)
    {
        auto& Node = _Partition._Nodes[NodeIndex];

        if (NOT Node._HasDirtyMarker)
        { continue; }

        // Short-circuit path (opt-in, cached at scheduler construction): if the registry's dirty-marker
        // version hasn't changed since our last observation, neither the count nor the contents of
        // the marker fragment could have moved. Skip the O(fragment-storage) Has_AnyEntityWith scan.
        if (_UseDirtyMarkerVersionShortCircuit)
        {
            const auto CurrentVersion = InRegistry.Get_DirtyMarkerVersion(Node._DirtyMarkerHash);
            if (CurrentVersion == Node._LastSeenDirtyVersion)
            { continue; }

            if (NOT Node._IsDirtyChecker(InRegistry))
            {
                // Nothing dirty right now — remember the version so we don't scan again until
                // another mutation happens.
                Node._LastSeenDirtyVersion = CurrentVersion;
                continue;
            }
        }
        else
        {
            // Legacy path — rescan every pump pass. Behavior is identical to pre-short-circuit.
            if (NOT Node._IsDirtyChecker(InRegistry))
            { continue; }
        }

        if (Node._Instance.IsSet() and not Node._IsGhost)
        {
#if !UE_BUILD_SHIPPING
            const auto PumpStartTime = FPlatformTime::Seconds();
#endif

            (*Node._Instance)->Pump();
            AnyProcessorTicked = true;

#if !UE_BUILD_SHIPPING
            const auto PumpElapsedMs = (FPlatformTime::Seconds() - PumpStartTime) * 1000.0;
            DoDebugRecordProcessorPump(NodeIndex, InPumpIndex, PumpElapsedMs);
#endif

            if (_UseDirtyMarkerVersionShortCircuit)
            {
                // Re-read the version AFTER Pump() — the processor probably mutated the marker
                // fragment, and the next pump pass should only re-fire if ANOTHER processor
                // mutates it again.
                Node._LastSeenDirtyVersion = InRegistry.Get_DirtyMarkerVersion(Node._DirtyMarkerHash);
            }
        }
        else if (_UseDirtyMarkerVersionShortCircuit)
        {
            Node._LastSeenDirtyVersion = InRegistry.Get_DirtyMarkerVersion(Node._DirtyMarkerHash);
        }
    }

    return AnyProcessorTicked;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorScheduler::
    DoLogPumpLimitReached(
        const FCk_Registry& InRegistry)
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
        double InElapsedMs)
    -> void
{
    if (NOT _DebugCurrentFrame.ProcessorTimings.IsValidIndex(InNodeIndex))
    { return; }

    _DebugCurrentFrame.ProcessorTimings[InNodeIndex].MainPassTimeMs = InElapsedMs;
}

auto
    ck::FProcessorScheduler::
    DoDebugRecordProcessorPump(
        int32 InNodeIndex,
        int32 InPumpPass,
        double InElapsedMs)
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
}

auto
    ck::FProcessorScheduler::
    DoDebugEndFrame()
    -> void
{
    if (_DebugFrameHistory.Num() >= DebugFrameHistoryMax)
    {
        _DebugFrameHistory.RemoveAt(0);
    }
    _DebugFrameHistory.Add(MoveTemp(_DebugCurrentFrame));
}

#endif // !UE_BUILD_SHIPPING

// --------------------------------------------------------------------------------------------------------------------
