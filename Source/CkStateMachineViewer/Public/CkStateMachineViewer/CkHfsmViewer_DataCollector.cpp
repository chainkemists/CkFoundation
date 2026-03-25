#include "CkHfsmViewer_DataCollector.h"

#include "CkStateMachineViewer/CkHfsmViewer_Log.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "HAL/PlatformTime.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"

#include "CkStateMachine/CkStateMachine_Debug_Fragment.h"
#include "CkStateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/CkStateMachine_Utils.h"
#include "CkStateMachine/EntityScripts/CkSmState_EntityScript.h"

// --------------------------------------------------------------------------------------------------------------------

static auto
    GetCleanClassName(
        const UClass* InClass)
    -> FString
{
    if (NOT IsValid(InClass))
    { return TEXT("(unknown)"); }

    auto Name = InClass->GetName();
    Name.RemoveFromStart(TEXT("BP_"));
    Name.RemoveFromEnd(TEXT("_C"));
    return Name;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_DataCollector::
    Collect(
        UWorld* InWorld)
    -> void
{
    _StateMachines.Reset();

    // Track PIE debug pause state for logical time computation
    {
        auto IsPausedNow = UCk_Utils_EditorOnly_UE::Get_IsDebugPauseExecution();

        if (IsPausedNow && NOT _WasPausedLastTick)
        {
            _PauseStartTime = FPlatformTime::Seconds();
        }
        else if (NOT IsPausedNow && _WasPausedLastTick)
        {
            _CompletedPauseIntervals.Add({_PauseStartTime, FPlatformTime::Seconds()});
        }

        _WasPausedLastTick = IsPausedNow;
        _IsPieDebugPaused = IsPausedNow;
    }

    if (NOT IsValid(InWorld))
    { return; }

    auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(InWorld);

    if (NOT ck::IsValid(TransientEntity))
    { return; }

    TransientEntity.View<ck::FFragment_Sm_Current, ck::FFragment_Sm_Params>().ForEach(
        [this, &TransientEntity](FCk_Entity InEntity, const ck::FFragment_Sm_Current&, const ck::FFragment_Sm_Params&)
        {
            auto Handle = ck::MakeHandle(InEntity, TransientEntity);
            _StateMachines.Add(CollectStateMachine(Handle));
        });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_DataCollector::
    Get_AllStateMachines() const
    -> const TArray<FCkHfsmViewer_SmInfo>&
{
    return _StateMachines;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_DataCollector::
    CollectStateMachine(
        const FCk_Handle& InSmHandle)
    -> FCkHfsmViewer_SmInfo
{
    auto SmInfo = FCkHfsmViewer_SmInfo{};
    SmInfo.Handle = ck::StaticCast<FCk_Handle_StateMachine>(InSmHandle);

    const auto& Current = InSmHandle.Get<ck::FFragment_Sm_Current>();
    SmInfo.RunStatus = Current.Get_RunStatus();
    SmInfo.CurrentStateClass = Current.Get_CurrentStateClass();
    SmInfo.IsTransitionQueued = InSmHandle.Has<ck::FTag_Sm_TransitionQueued>();

    if (InSmHandle.Has<ck::FFragment_Sm_Params>())
    {
        SmInfo.InitialStateClass = InSmHandle.Get<ck::FFragment_Sm_Params>().Get_InitialStateClass();
    }

    if (InSmHandle.Has<ck::FFragment_Sm_Context>())
    {
        SmInfo.GameEntity = InSmHandle.Get<ck::FFragment_Sm_Context>().Get_GameEntityHandle();
    }

    if (ck::IsValid(SmInfo.GameEntity))
    {
        SmInfo.DebugName = UCk_Utils_Handle_UE::Get_DebugName(SmInfo.GameEntity).ToString();
    }

    if (SmInfo.DebugName.IsEmpty())
    {
        SmInfo.DebugName = UCk_Utils_Handle_UE::Get_DebugName(InSmHandle).ToString();
    }

    if (SmInfo.DebugName.IsEmpty())
    {
        SmInfo.DebugName = TEXT("(unnamed)");
    }

    // Build the graph from the debug fragment's cached data
    if (NOT InSmHandle.Has<ck::FFragment_Sm_Debug>())
    {
        return SmInfo;
    }

    const auto& Debug = InSmHandle.Get<ck::FFragment_Sm_Debug>();

    // Reset accumulated pause duration when a new SM run starts
    auto CurrentRunCounter = Debug.Get_RunCounter();

    if (CurrentRunCounter != _LastObservedRunCounter)
    {
        _CompletedPauseIntervals.Reset();
        _BreakpointHitWallTimes.Reset();
        _LastObservedRunCounter = CurrentRunCounter;
    }

    const auto& CachedStates = Debug.Get_CachedStates();

    auto StateClassToIndex = TMap<TSubclassOf<UCk_SmState_EntityScript>, int32>{};

    // Build state nodes from cache
    for (const auto& [StateClass, CachedState] : CachedStates)
    {
        auto StateInfo = FCkHfsmViewer_StateInfo{};
        StateInfo.StateClass = StateClass;
        StateInfo.StateName = CachedState.StateName;
        StateInfo.IsCurrentState = (StateClass == SmInfo.CurrentStateClass);

        // Copy cached tasks
        for (const auto& CachedTask : CachedState.Tasks)
        {
            auto TaskInfo = FCkHfsmViewer_TaskInfo{};
            TaskInfo.ClassName = CachedTask.ClassName;
            TaskInfo.Mode = CachedTask.Mode;
            StateInfo.Tasks.Add(MoveTemp(TaskInfo));
        }

        auto Index = SmInfo.States.Num();
        StateClassToIndex.Add(StateClass, Index);
        SmInfo.States.Add(MoveTemp(StateInfo));

        if (StateClass == SmInfo.CurrentStateClass)
        {
            SmInfo.CurrentStateIndex = Index;
        }
    }

    // Build transitions from cache
    for (const auto& [StateClass, CachedState] : CachedStates)
    {
        auto* SourceIndex = StateClassToIndex.Find(StateClass);

        if (NOT SourceIndex)
        { continue; }

        for (const auto& CachedTransition : CachedState.Transitions)
        {
            auto TransInfo = FCkHfsmViewer_TransitionInfo{};
            TransInfo.SourceStateIndex = *SourceIndex;
            TransInfo.Order = CachedTransition.Order;
            TransInfo.TargetStateClass = CachedTransition.TargetStateClass;

            if (IsValid(CachedTransition.TargetStateClass))
            {
                TransInfo.TargetStateName = GetCleanClassName(CachedTransition.TargetStateClass);
            }

            auto* TargetIndex = StateClassToIndex.Find(CachedTransition.TargetStateClass);
            TransInfo.TargetStateIndex = TargetIndex ? *TargetIndex : -1;

            // Build conditions from cache (no live satisfaction data yet)
            for (const auto& CachedCondition : CachedTransition.Conditions)
            {
                auto CondInfo = FCkHfsmViewer_ConditionInfo{};
                CondInfo.ClassName = CachedCondition.ClassName;
                CondInfo.Mode = CachedCondition.Mode;
                CondInfo.ResetBehavior = CachedCondition.ResetBehavior;
                CondInfo.IsSatisfied = false;
                TransInfo.Conditions.Add(MoveTemp(CondInfo));
            }

            TransInfo.TotalCount = TransInfo.Conditions.Num();
            TransInfo.SatisfiedCount = 0;
            TransInfo.AreAllConditionsSatisfied = false;

            SmInfo.Transitions.Add(MoveTemp(TransInfo));
        }
    }

    // Overlay live condition satisfaction for the current state's transitions
    auto CurrentStateHandle = Current.Get_CurrentStateHandle();

    if (ck::IsValid(CurrentStateHandle) && IsValid(SmInfo.CurrentStateClass))
    {
        OverlayLiveData(
            CurrentStateHandle,
            SmInfo.CurrentStateIndex,
            StateClassToIndex,
            SmInfo);
    }

    // Copy history
    for (const auto& HistoryEntry : Debug.Get_History())
    {
        auto ViewerEntry = FCkHfsmViewer_HistoryEntry{};
        ViewerEntry.FromStateName = HistoryEntry.FromStateName;
        ViewerEntry.ToStateName = HistoryEntry.ToStateName;
        ViewerEntry.FrameNumber = HistoryEntry.FrameNumber;
        ViewerEntry.TransitionOrder = HistoryEntry.TransitionOrder;
        ViewerEntry.ConditionNames = HistoryEntry.TransitionConditionNames;
        ViewerEntry.RealTimeSeconds = ComputeLogicalTime(HistoryEntry.RealTimeSeconds);
        SmInfo.History.Add(MoveTemp(ViewerEntry));
    }

    // Compute dwell times using logical time
    auto Now = ComputeLogicalTime(FPlatformTime::Seconds());

    // Current state: live dwell from debug fragment timestamp
    if (SmInfo.CurrentStateIndex >= 0)
    {
        auto EnteredAt = ComputeLogicalTime(Debug.Get_CurrentStateEnteredAtRealTime());

        if (EnteredAt > 0.0)
        {
            SmInfo.States[SmInfo.CurrentStateIndex].DwellTimeSeconds = Now - EnteredAt;
            SmInfo.States[SmInfo.CurrentStateIndex].IsCurrentDwellLive = true;
            SmInfo.States[SmInfo.CurrentStateIndex].HasBeenVisited = true;
        }
    }

    // Past states: walk history backwards to find last dwell per state
    auto VisitedStateClasses = TSet<TSubclassOf<UCk_SmState_EntityScript>>{};

    for (auto HistIdx = SmInfo.History.Num() - 1; HistIdx >= 1; --HistIdx)
    {
        const auto& Entry = Debug.Get_History()[HistIdx];
        auto FromClass = Entry.FromStateClass;

        if (VisitedStateClasses.Contains(FromClass))
        { continue; }

        VisitedStateClasses.Add(FromClass);

        auto* FromIndex = StateClassToIndex.Find(FromClass);

        if (NOT FromIndex)
        { continue; }

        auto& StateInfo = SmInfo.States[*FromIndex];

        if (StateInfo.IsCurrentDwellLive)
        { continue; }

        // Find when this state was entered (the previous history entry's timestamp)
        auto EnteredAt = 0.0;

        if (HistIdx > 0)
        {
            EnteredAt = ComputeLogicalTime(Debug.Get_History()[HistIdx - 1].RealTimeSeconds);
        }

        auto ExitedAt = ComputeLogicalTime(Entry.RealTimeSeconds);

        if (EnteredAt > 0.0 && ExitedAt > EnteredAt)
        {
            StateInfo.DwellTimeSeconds = ExitedAt - EnteredAt;
        }

        StateInfo.HasBeenVisited = true;
    }

    // First history entry's ToState was entered at SM start, mark as visited
    if (NOT Debug.Get_History().IsEmpty())
    {
        auto FirstToClass = Debug.Get_History()[0].ToStateClass;
        auto* FirstToIndex = StateClassToIndex.Find(FirstToClass);

        if (FirstToIndex && NOT SmInfo.States[*FirstToIndex].HasBeenVisited)
        {
            SmInfo.States[*FirstToIndex].HasBeenVisited = true;
        }
    }

    // Build CurrentRun from live history
    {
        auto RunStartTime = 0.0;
        auto InitialStateName = FString{};

        if (NOT SmInfo.History.IsEmpty())
        {
            RunStartTime = SmInfo.History[0].RealTimeSeconds;
            InitialStateName = SmInfo.History[0].FromStateName;
        }
        else if (SmInfo.CurrentStateIndex >= 0)
        {
            RunStartTime = ComputeLogicalTime(Debug.Get_CurrentStateEnteredAtRealTime());
            InitialStateName = SmInfo.States[SmInfo.CurrentStateIndex].StateName;
        }

        SmInfo.CurrentRun.RunIndex = Debug.Get_RunCounter();
        SmInfo.CurrentRun.StartTime = RunStartTime;
        SmInfo.CurrentRun.EndTime = 0.0;
        SmInfo.CurrentRun.Duration = Now - RunStartTime;
        SmInfo.CurrentRun.History = SmInfo.History;
        SmInfo.CurrentRun.Segments = BuildTimelineSegments(SmInfo.History, RunStartTime, Now, InitialStateName);
        SmInfo.CurrentRun.BusyFrames = DetectBusyFrames(SmInfo.History, RunStartTime);
        SmInfo.CurrentRun.FrameSegments = BuildFrameSegments(SmInfo.History, RunStartTime, Now);

        // Populate pause markers from breakpoint hit wall times
        for (const auto& HitWallTime : _BreakpointHitWallTimes)
        {
            auto LogicalTime = ComputeLogicalTime(HitWallTime);
            auto RelativeTime = LogicalTime - RunStartTime;

            auto Marker = FCkHfsmViewer_TimelinePauseMarker{};
            Marker.Time = RelativeTime;
            Marker.IsBreakpoint = true;
            SmInfo.CurrentRun.PauseMarkers.Add(MoveTemp(Marker));
        }

        // Populate pause markers from non-breakpoint pauses
        for (const auto& [PauseStart, PauseEnd] : _CompletedPauseIntervals)
        {
            auto LogicalTime = ComputeLogicalTime(PauseStart);
            auto RelativeTime = LogicalTime - RunStartTime;

            auto IsBreakpointPause = _BreakpointHitWallTimes.ContainsByPredicate(
                [&](double InHitTime) { return FMath::Abs(InHitTime - PauseStart) < 0.5; });

            if (NOT IsBreakpointPause)
            {
                auto Marker = FCkHfsmViewer_TimelinePauseMarker{};
                Marker.Time = RelativeTime;
                Marker.IsBreakpoint = false;
                SmInfo.CurrentRun.PauseMarkers.Add(MoveTemp(Marker));
            }
        }
    }

    // Copy completed runs from debug fragment
#if !UE_BUILD_SHIPPING
    {
        for (const auto& BackendRun : Debug.Get_CompletedRuns())
        {
            auto ViewerRun = FCkHfsmViewer_RunInfo{};
            ViewerRun.RunIndex = BackendRun.RunIndex;
            ViewerRun.StartTime = ComputeLogicalTime(BackendRun.StartRealTimeSeconds);
            ViewerRun.EndTime = ComputeLogicalTime(BackendRun.EndRealTimeSeconds);
            ViewerRun.Duration = ViewerRun.EndTime - ViewerRun.StartTime;

            for (const auto& HistEntry : BackendRun.History)
            {
                auto ViewerEntry = FCkHfsmViewer_HistoryEntry{};
                ViewerEntry.FromStateName = HistEntry.FromStateName;
                ViewerEntry.ToStateName = HistEntry.ToStateName;
                ViewerEntry.FrameNumber = HistEntry.FrameNumber;
                ViewerEntry.TransitionOrder = HistEntry.TransitionOrder;
                ViewerEntry.ConditionNames = HistEntry.TransitionConditionNames;
                ViewerEntry.RealTimeSeconds = ComputeLogicalTime(HistEntry.RealTimeSeconds);
                ViewerRun.History.Add(MoveTemp(ViewerEntry));
            }

            auto RunInitialState = ViewerRun.History.IsEmpty() ? FString{} : ViewerRun.History[0].FromStateName;
            ViewerRun.Segments = BuildTimelineSegments(ViewerRun.History, ViewerRun.StartTime, ViewerRun.EndTime, RunInitialState);
            ViewerRun.BusyFrames = DetectBusyFrames(ViewerRun.History, ViewerRun.StartTime);
            ViewerRun.FrameSegments = BuildFrameSegments(ViewerRun.History, ViewerRun.StartTime, ViewerRun.EndTime);

            SmInfo.CompletedRuns.Add(MoveTemp(ViewerRun));
        }
    }

    // Copy breakpoint flags from ECS onto viewer state/transition structs
    if (InSmHandle.Has<ck::FFragment_Sm_Breakpoints>())
    {
        const auto& Breakpoints = InSmHandle.Get<ck::FFragment_Sm_Breakpoints>();

        for (auto& StateInfo : SmInfo.States)
        {
            StateInfo.HasEntryBreakpoint = Breakpoints.Get_EntryBreakpoints().Contains(StateInfo.StateClass);
            StateInfo.HasExitBreakpoint = Breakpoints.Get_ExitBreakpoints().Contains(StateInfo.StateClass);
        }

        for (auto& TransInfo : SmInfo.Transitions)
        {
            if (TransInfo.SourceStateIndex >= 0 && TransInfo.TargetStateClass)
            {
                auto SourceClass = SmInfo.States[TransInfo.SourceStateIndex].StateClass;
                auto Key = ck::FFragment_Sm_Breakpoints::FTransitionKey{SourceClass, TransInfo.TargetStateClass};
                TransInfo.HasBreakpoint = Breakpoints.Get_TransitionBreakpoints().Contains(Key);
            }
        }
    }

    // Read breakpoint hit info (transient fragment added at breakpoint site)
    if (InSmHandle.Has<ck::FFragment_Sm_Debug_BreakpointHit>())
    {
        if (_IsPieDebugPaused)
        {
            const auto& HitFrag = InSmHandle.Get<ck::FFragment_Sm_Debug_BreakpointHit>();
            SmInfo.HasBreakpointHit = true;
            SmInfo.BreakpointHitDescription = HitFrag.Description;

            if (NOT _BreakpointHitWallTimes.Contains(HitFrag.RealTimeSeconds))
            {
                _BreakpointHitWallTimes.Add(HitFrag.RealTimeSeconds);
            }

            if (SmInfo.CurrentStateIndex >= 0)
            {
                SmInfo.States[SmInfo.CurrentStateIndex].IsBreakpointHit = true;
            }
        }
        else
        {
            auto MutableHandle = static_cast<FCk_Handle>(InSmHandle);
            MutableHandle.Remove<ck::FFragment_Sm_Debug_BreakpointHit>();
        }
    }
#endif

    SmInfo.IsPieDebugPaused = _IsPieDebugPaused;

    return SmInfo;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_DataCollector::
    OverlayLiveData(
        const FCk_Handle& InStateHandle,
        int32 InCurrentStateIndex,
        const TMap<TSubclassOf<UCk_SmState_EntityScript>, int32>& InStateClassToIndex,
        FCkHfsmViewer_SmInfo& InOutSmInfo)
    -> void
{
    auto StateChildren = UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(InStateHandle);

    auto TaskIndex = 0;

    for (const auto& ChildHandle : StateChildren)
    {
        // Overlay live condition data on transitions
        if (ChildHandle.Has<ck::FFragment_SmTransition_Params>())
        {
            const auto& TransParams = ChildHandle.Get<ck::FFragment_SmTransition_Params>();
            auto TargetClass = TransParams.Get_TargetStateClass();
            auto Order = TransParams.Get_Order();

            auto* MatchingTransition = static_cast<FCkHfsmViewer_TransitionInfo*>(nullptr);

            for (auto& Trans : InOutSmInfo.Transitions)
            {
                if (Trans.SourceStateIndex == InCurrentStateIndex
                    && Trans.TargetStateClass == TargetClass
                    && Trans.Order == Order)
                {
                    MatchingTransition = &Trans;
                    break;
                }
            }

            if (NOT MatchingTransition)
            { continue; }

            MatchingTransition->Handle = ChildHandle;

            auto TransChildren = UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(ChildHandle);
            auto SatisfiedCount = 0;
            auto TotalCount = 0;
            auto ConditionIndex = 0;

            for (const auto& CondHandle : TransChildren)
            {
                if (NOT CondHandle.Has<ck::FFragment_SmCondition_Current>())
                { continue; }

                const auto& CondCurrent = CondHandle.Get<ck::FFragment_SmCondition_Current>();
                auto IsSatisfied = CondCurrent.Get_IsSatisfied();

                ++TotalCount;

                if (IsSatisfied)
                {
                    ++SatisfiedCount;
                }

                if (ConditionIndex < MatchingTransition->Conditions.Num())
                {
                    MatchingTransition->Conditions[ConditionIndex].Handle = CondHandle;
                    MatchingTransition->Conditions[ConditionIndex].IsSatisfied = IsSatisfied;
                }

                ++ConditionIndex;
            }

            MatchingTransition->TotalCount = TotalCount;
            MatchingTransition->SatisfiedCount = SatisfiedCount;
            MatchingTransition->AreAllConditionsSatisfied = (TotalCount > 0 && SatisfiedCount == TotalCount);
        }

        // Overlay live task results
        if (ChildHandle.Has<ck::FFragment_SmTask_Current>())
        {
            auto& CurrentState = InOutSmInfo.States[InCurrentStateIndex];

            if (TaskIndex < CurrentState.Tasks.Num())
            {
                CurrentState.Tasks[TaskIndex].Handle = ChildHandle;
                CurrentState.Tasks[TaskIndex].LastResult = ChildHandle.Get<ck::FFragment_SmTask_Current>().Get_LastResult();
            }

            ++TaskIndex;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_DataCollector::
    BuildTimelineSegments(
        const TArray<FCkHfsmViewer_HistoryEntry>& InHistory,
        double InRunStartTime,
        double InNow,
        const FString& InInitialStateName)
    -> TArray<FCkHfsmViewer_TimelineSegment>
{
    auto Segments = TArray<FCkHfsmViewer_TimelineSegment>{};

    if (InHistory.IsEmpty())
    {
        if (NOT InInitialStateName.IsEmpty() && InNow > InRunStartTime)
        {
            auto Segment = FCkHfsmViewer_TimelineSegment{};
            Segment.StateName = InInitialStateName;
            Segment.StartTime = 0.0;
            Segment.EndTime = 0.0;
            Segment.Color = ComputeStateColor(InInitialStateName);
            Segments.Add(MoveTemp(Segment));
        }

        return Segments;
    }

    // First segment: initial state (FromState of first entry) from time 0 to first transition
    {
        auto Segment = FCkHfsmViewer_TimelineSegment{};
        Segment.StateName = InHistory[0].FromStateName;
        Segment.StartTime = 0.0;
        Segment.EndTime = InHistory[0].RealTimeSeconds - InRunStartTime;
        Segment.Color = ComputeStateColor(Segment.StateName);
        Segments.Add(MoveTemp(Segment));
    }

    // Middle segments: each history entry's ToState occupies from this entry's time to the next entry's time
    for (auto Index = 0; Index < InHistory.Num(); ++Index)
    {
        auto Segment = FCkHfsmViewer_TimelineSegment{};
        Segment.StateName = InHistory[Index].ToStateName;
        Segment.StartTime = InHistory[Index].RealTimeSeconds - InRunStartTime;

        if (Index + 1 < InHistory.Num())
        {
            Segment.EndTime = InHistory[Index + 1].RealTimeSeconds - InRunStartTime;
        }
        else
        {
            // Last entry: open-ended (still active or run ended)
            Segment.EndTime = 0.0;
        }

        Segment.Color = ComputeStateColor(Segment.StateName);
        Segments.Add(MoveTemp(Segment));
    }

    return Segments;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_DataCollector::
    DetectBusyFrames(
        const TArray<FCkHfsmViewer_HistoryEntry>& InHistory,
        double InRunStartTime)
    -> TArray<FCkHfsmViewer_TimelineBusyFrame>
{
    auto BusyFrames = TArray<FCkHfsmViewer_TimelineBusyFrame>{};

    if (InHistory.Num() < 2)
    { return BusyFrames; }

    auto CurrentFrame = InHistory[0].FrameNumber;
    auto Count = 1;
    auto FirstEntryTime = InHistory[0].RealTimeSeconds;

    for (auto Index = 1; Index < InHistory.Num(); ++Index)
    {
        if (InHistory[Index].FrameNumber == CurrentFrame)
        {
            ++Count;
        }
        else
        {
            if (Count >= 2)
            {
                auto BusyFrame = FCkHfsmViewer_TimelineBusyFrame{};
                BusyFrame.Time = FirstEntryTime - InRunStartTime;
                BusyFrame.FrameNumber = CurrentFrame;
                BusyFrame.TransitionCount = Count;
                BusyFrames.Add(MoveTemp(BusyFrame));
            }

            CurrentFrame = InHistory[Index].FrameNumber;
            Count = 1;
            FirstEntryTime = InHistory[Index].RealTimeSeconds;
        }
    }

    // Check the last group
    if (Count >= 2)
    {
        auto BusyFrame = FCkHfsmViewer_TimelineBusyFrame{};
        BusyFrame.Time = FirstEntryTime - InRunStartTime;
        BusyFrame.FrameNumber = CurrentFrame;
        BusyFrame.TransitionCount = Count;
        BusyFrames.Add(MoveTemp(BusyFrame));
    }

    return BusyFrames;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_DataCollector::
    BuildFrameSegments(
        const TArray<FCkHfsmViewer_HistoryEntry>& InHistory,
        double InRunStartTime,
        double InNow)
    -> TArray<FCkHfsmViewer_FrameSegment>
{
    auto Segments = TArray<FCkHfsmViewer_FrameSegment>{};

    if (InHistory.IsEmpty())
    { return Segments; }

    for (auto Index = 0; Index < InHistory.Num(); ++Index)
    {
        auto Segment = FCkHfsmViewer_FrameSegment{};
        Segment.StartTime = InHistory[Index].RealTimeSeconds - InRunStartTime;
        Segment.StartFrame = InHistory[Index].FrameNumber;

        if (Index + 1 < InHistory.Num())
        {
            Segment.EndTime = InHistory[Index + 1].RealTimeSeconds - InRunStartTime;
            Segment.EndFrame = InHistory[Index + 1].FrameNumber;
        }
        else
        {
            Segment.EndTime = InNow > InRunStartTime ? InNow - InRunStartTime : 0.0;
            Segment.EndFrame = Segment.StartFrame;
        }

        Segments.Add(MoveTemp(Segment));
    }

    return Segments;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_DataCollector::
    ComputeLogicalTime(
        double InWallClockTime) const
    -> double
{
    auto TotalPauseBefore = 0.0;

    for (const auto& [PauseStart, PauseEnd] : _CompletedPauseIntervals)
    {
        if (InWallClockTime > PauseStart)
        {
            TotalPauseBefore += FMath::Min(InWallClockTime, PauseEnd) - PauseStart;
        }
    }

    if (_WasPausedLastTick && InWallClockTime > _PauseStartTime)
    {
        TotalPauseBefore += FMath::Min(InWallClockTime, FPlatformTime::Seconds()) - _PauseStartTime;
    }

    return InWallClockTime - TotalPauseBefore;
}

// --------------------------------------------------------------------------------------------------------------------
