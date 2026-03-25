#include "CkInsightsAnalyzer/Core/CkFrameAnalyzer.h"
#include "CkInsightsAnalyzer/Core/CkTraceSession.h"
#include "CkInsightsAnalyzer_Log.h"

// --------------------------------------------------------------------------------------------------------------------
// FCk_FrameAnalysisResult
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameAnalysisResult::
    GetChildren(uint32 ParentTimerIndex, double MinInclusiveMs) const
    -> TArray<TPair<uint32, double>>
{
    TArray<TPair<uint32, double>> Result;

    const TMap<uint32, double>* Children = ChildrenOf.Find(ParentTimerIndex);
    if (!Children) return Result;

    for (const auto& [ChildIndex, InclSeconds] : *Children)
    {
        const double InclMs = InclSeconds * 1000.0;
        if (InclMs >= MinInclusiveMs)
        {
            Result.Emplace(ChildIndex, InclMs);
        }
    }

    // Sort by inclusive time descending
    Result.Sort([](const TPair<uint32, double>& A, const TPair<uint32, double>& B)
    {
        return A.Value > B.Value;
    });

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------
// FCk_FrameAnalyzer
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameAnalyzer::
    AnalyzeFrame(const FCk_TraceSession& Session, uint64 FrameIndex)
    -> FCk_FrameAnalysisResult
{
    FCk_FrameAnalysisResult Result;

    if (!Session.IsOpen())
    {
        UE_LOG(CkInsightsAnalyzer, Error, TEXT("AnalyzeFrame: Session not open"));
        return Result;
    }

    TraceServices::FAnalysisSessionReadScope ReadScope = Session.CreateReadScope();

    const TraceServices::IFrameProvider* FrameProvider = Session.GetFrameProvider();
    if (!FrameProvider)
    {
        UE_LOG(CkInsightsAnalyzer, Error, TEXT("AnalyzeFrame: No frame provider"));
        return Result;
    }

    const TraceServices::FFrame* Frame =
        FrameProvider->GetFrame(ETraceFrameType::TraceFrameType_Game, FrameIndex);

    if (!Frame)
    {
        UE_LOG(CkInsightsAnalyzer, Error,
            TEXT("AnalyzeFrame: Frame %llu not found"), FrameIndex);
        return Result;
    }

    const uint32 GameThreadId = Session.GetGameThreadId();
    if (GameThreadId == static_cast<uint32>(INDEX_NONE))
    {
        UE_LOG(CkInsightsAnalyzer, Error, TEXT("AnalyzeFrame: Could not identify game thread"));
        return Result;
    }

    Result = AnalyzeTimeRange(Session, GameThreadId,
                              Frame->StartTime, Frame->EndTime, FrameIndex);
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameAnalyzer::
    AnalyzeTimeRange(const FCk_TraceSession& Session,
                     uint32 ThreadId,
                     double StartTime, double EndTime,
                     uint64 FrameIndex)
    -> FCk_FrameAnalysisResult
{
    FCk_FrameAnalysisResult Result;
    Result.FrameIndex = FrameIndex;
    Result.FrameStartTime = StartTime;
    Result.FrameEndTime = EndTime;
    Result.FrameDurationMs = (EndTime - StartTime) * 1000.0;
    Result.ThreadId = ThreadId;

    if (!Session.IsOpen())
    {
        UE_LOG(CkInsightsAnalyzer, Error, TEXT("AnalyzeTimeRange: Session not open"));
        return Result;
    }

    const uint32 TimelineIndex = Session.GetTimelineIndex(ThreadId);
    if (TimelineIndex == static_cast<uint32>(INDEX_NONE))
    {
        UE_LOG(CkInsightsAnalyzer, Warning,
            TEXT("AnalyzeTimeRange: No timeline for thread %u"), ThreadId);
        return Result;
    }

    // Extract events
    Result.Events = ExtractEvents(Session, TimelineIndex, StartTime, EndTime);

    if (Result.Events.Num() == 0)
    {
        UE_LOG(CkInsightsAnalyzer, Warning,
            TEXT("AnalyzeTimeRange: No events in [%.6f, %.6f] on thread %u"),
            StartTime, EndTime, ThreadId);
        return Result;
    }

    // Find root timer (minimum depth event)
    uint32 MinDepth = MAX_uint32;
    for (const FCk_TimingEvent& Evt : Result.Events)
    {
        if (Evt.Depth < MinDepth)
        {
            MinDepth = Evt.Depth;
            Result.FrameRootTimerIndex = Evt.TimerIndex;
        }
    }

    // Compute exclusive times
    ComputeExclusiveTimes(Result.Events, Result);

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameAnalyzer::
    AnalyzeThread(const FCk_TraceSession& Session, uint32 ThreadId)
    -> FCk_FrameAnalysisResult
{
    if (!Session.IsOpen())
    {
        return FCk_FrameAnalysisResult{};
    }

    const uint32 TimelineIndex = Session.GetTimelineIndex(ThreadId);
    if (TimelineIndex == static_cast<uint32>(INDEX_NONE))
    {
        return FCk_FrameAnalysisResult{};
    }

    // Use full timeline range
    double StartTime = 0.0;
    double EndTime = Session.GetDurationSeconds() + 1.0;

    return AnalyzeTimeRange(Session, ThreadId, StartTime, EndTime, 0);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameAnalyzer::
    ExtractEvents(const FCk_TraceSession& Session,
                  uint32 TimelineIndex,
                  double StartTime, double EndTime)
    -> TArray<FCk_TimingEvent>
{
    TArray<FCk_TimingEvent> Events;

    Session.ReadTimeline(TimelineIndex,
        [&Events, StartTime, EndTime](const TraceServices::ITimingProfilerProvider::Timeline& Timeline)
        {
            Timeline.EnumerateEvents(StartTime, EndTime,
                [&Events](double EvtStartTime, double EvtEndTime, uint32 EvtDepth,
                          const TraceServices::FTimingProfilerEvent& Event)
                    -> TraceServices::EEventEnumerate
                {
                    Events.Add(FCk_TimingEvent{
                        Event.TimerIndex,
                        EvtStartTime,
                        EvtEndTime,
                        EvtDepth
                    });
                    return TraceServices::EEventEnumerate::Continue;
                });
        });

    return Events;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameAnalyzer::
    ComputeExclusiveTimes(const TArray<FCk_TimingEvent>& Events,
                          FCk_FrameAnalysisResult& Result)
    -> void
{
    if (Events.Num() == 0) return;

    // CRITICAL: Sort by (start, depth) so parents are processed before children
    // when they share the same start time. This ensures the parent is on the
    // stack when we process the child, so we can subtract the child's time.
    TArray<int32> SortedIndices;
    SortedIndices.SetNum(Events.Num());
    for (int32 i = 0; i < Events.Num(); ++i)
    {
        SortedIndices[i] = i;
    }

    SortedIndices.Sort([&Events](int32 A, int32 B)
    {
        const FCk_TimingEvent& EA = Events[A];
        const FCk_TimingEvent& EB = Events[B];
        if (EA.StartTime != EB.StartTime)
            return EA.StartTime < EB.StartTime;
        return EA.Depth < EB.Depth;
    });

    // Per-event remaining exclusive time (starts at inclusive, children subtracted)
    TArray<double> EventExcl;
    EventExcl.SetNum(SortedIndices.Num());

    for (int32 i = 0; i < SortedIndices.Num(); ++i)
    {
        const FCk_TimingEvent& Evt = Events[SortedIndices[i]];
        EventExcl[i] = Evt.EndTime - Evt.StartTime;
    }

    // Stack: (Depth, EndTime, SortedIndex, TimerIndex)
    struct FStackEntry
    {
        uint32 Depth;
        double EndTime;
        int32  SortedIndex;
        uint32 TimerIndex;
    };
    TArray<FStackEntry> Stack;
    Stack.Reserve(64);

    // Process each event
    for (int32 i = 0; i < SortedIndices.Num(); ++i)
    {
        const FCk_TimingEvent& Evt = Events[SortedIndices[i]];
        const double Inclusive = Evt.EndTime - Evt.StartTime;

        // Accumulate inclusive time and count
        double& InclAccum = Result.TimerInclusive.FindOrAdd(Evt.TimerIndex, 0.0);
        InclAccum += Inclusive;

        uint32& CountAccum = Result.TimerCount.FindOrAdd(Evt.TimerIndex, 0);
        CountAccum += 1;

        // Pop events that have ended
        while (Stack.Num() > 0 && Stack.Last().EndTime <= Evt.StartTime)
        {
            Stack.Pop(EAllowShrinking::No);
        }

        // Find direct parent: nearest ancestor on stack whose depth < ours
        // and whose time range contains us
        for (int32 j = Stack.Num() - 1; j >= 0; --j)
        {
            const FStackEntry& Parent = Stack[j];
            if (Parent.Depth < Evt.Depth && Parent.EndTime >= Evt.EndTime)
            {
                // This is our direct parent — subtract child inclusive from parent exclusive
                EventExcl[Parent.SortedIndex] -= Inclusive;

                // Record parent→child relationship
                TMap<uint32, double>& ChildMap =
                    Result.ChildrenOf.FindOrAdd(Parent.TimerIndex);
                double& ChildIncl = ChildMap.FindOrAdd(Evt.TimerIndex, 0.0);
                ChildIncl += Inclusive;
                break;
            }
        }

        Stack.Push(FStackEntry{ Evt.Depth, Evt.EndTime, i, Evt.TimerIndex });
    }

    // Aggregate exclusive times per timer (clamp negatives to zero)
    for (int32 i = 0; i < SortedIndices.Num(); ++i)
    {
        const FCk_TimingEvent& Evt = Events[SortedIndices[i]];
        const double Excl = FMath::Max(0.0, EventExcl[i]);

        double& ExclAccum = Result.TimerExclusive.FindOrAdd(Evt.TimerIndex, 0.0);
        ExclAccum += Excl;
    }
}

// --------------------------------------------------------------------------------------------------------------------
