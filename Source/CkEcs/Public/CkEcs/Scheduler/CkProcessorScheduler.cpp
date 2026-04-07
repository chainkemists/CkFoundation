#include "CkProcessorScheduler.h"

#include "CkEcs/CkEcsLog.h"

#include "CkProfile/Stats/CkStats.h"

// --------------------------------------------------------------------------------------------------------------------

ck::FProcessorScheduler::
    FProcessorScheduler(
        FProcessorGraphPartition&& InPartition)
    : _Partition(MoveTemp(InPartition))
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

    for (const auto NodeIndex : _Partition._ExecutionOrder)
    {
        auto& Node = _Partition._Nodes[NodeIndex];
        if (Node._Instance.IsSet() and not Node._IsGhost)
        {
            (*Node._Instance)->Tick(InDeltaTime);
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

        if (NOT Node._IsDirtyChecker(InRegistry))
        { continue; }

        if (Node._Instance.IsSet() and not Node._IsGhost)
        {
            (*Node._Instance)->Pump();
            AnyProcessorTicked = true;
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
