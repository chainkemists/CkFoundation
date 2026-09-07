#include "CkInsightsAnalyzer/Core/CkFrameAnalyzer.h"
#include "CkInsightsAnalyzer/Core/CkTraceSession.h"
#include "CkInsightsAnalyzer_Log.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include <limits>

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    auto IsValidAnalysisWindow(double InStartTime, double InEndTime) -> bool
    {
        if (NOT FMath::IsFinite(InStartTime) ||
            NOT FMath::IsFinite(InEndTime) ||
            InStartTime >= InEndTime)
        { return false; }

        const double DurationSeconds = InEndTime - InStartTime;
        return FMath::IsFinite(DurationSeconds) &&
            FMath::IsFinite(DurationSeconds * 1000.0);
    }

    auto IsPositiveInfinite(double InValue) -> bool
    {
        return InValue == std::numeric_limits<double>::infinity();
    }

    auto SortEventsParentBeforeChildren(TArray<FCk_TimingEvent>& InOutEvents) -> void
    {
        InOutEvents.Sort([](const FCk_TimingEvent& A, const FCk_TimingEvent& B)
        {
            if (A.StartTime != B.StartTime)
            { return A.StartTime < B.StartTime; }

            if (A.Depth != B.Depth)
            { return A.Depth < B.Depth; }

            if (A.EndTime != B.EndTime)
            { return A.EndTime > B.EndTime; }

            return A.TimerIndex < B.TimerIndex;
        });
    }

    auto ComputeUnionTotals(FCk_FrameAnalysisResult& InOutResult) -> void
    {
        auto InstrumentedEnd = TNumericLimits<double>::Lowest();
        auto TimerEnds = TMap<uint32, double>{};

        for (const FCk_TimingEvent& Event : InOutResult.Events)
        {
            if (Event.StartTime > InstrumentedEnd)
            {
                InOutResult.InstrumentedMs += (Event.EndTime - Event.StartTime) * 1000.0;
                InstrumentedEnd = Event.EndTime;
            }
            else if (Event.EndTime > InstrumentedEnd)
            {
                InOutResult.InstrumentedMs += (Event.EndTime - InstrumentedEnd) * 1000.0;
                InstrumentedEnd = Event.EndTime;
            }

            double* TimerEnd = TimerEnds.Find(Event.TimerIndex);
            if (TimerEnd == nullptr || Event.StartTime > *TimerEnd)
            {
                InOutResult.TimerOuterInclusive.FindOrAdd(Event.TimerIndex) +=
                    Event.EndTime - Event.StartTime;
                TimerEnds.Add(Event.TimerIndex, Event.EndTime);
            }
            else if (Event.EndTime > *TimerEnd)
            {
                InOutResult.TimerOuterInclusive.FindOrAdd(Event.TimerIndex) +=
                    Event.EndTime - *TimerEnd;
                *TimerEnd = Event.EndTime;
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameAnalysisResult::
    GetChildren(uint32 ParentTimerIndex, double MinInclusiveMs) const
    -> TArray<TPair<uint32, double>>
{
    TArray<TPair<uint32, double>> Result;

    const auto Children = ChildrenOf.Find(ParentTimerIndex);
    if (NOT Children) return Result;

    for (const auto& [ChildIndex, InclSeconds] : *Children)
    {
        const double InclMs = InclSeconds * 1000.0;
        if (InclMs >= MinInclusiveMs)
        {
            Result.Emplace(ChildIndex, InclMs);
        }
    }

    Result.Sort([](const TPair<uint32, double>& A, const TPair<uint32, double>& B)
    {
        return A.Value > B.Value;
    });

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameAnalyzer::
    AnalyzeFrame(const FCk_TraceSession& Session, uint64 FrameIndex)
    -> FCk_FrameAnalysisResult
{
    FCk_FrameAnalysisResult Result;

    if (NOT Session.IsOpen())
    {
        ck::insights_analyzer::Error(TEXT("AnalyzeFrame: Session not open"));
        return Result;
    }

    TraceServices::FAnalysisSessionReadScope ReadScope = Session.CreateReadScope();

    const auto FrameProvider = Session.GetFrameProvider();
    if (ck::Is_NOT_Valid(FrameProvider, ck::IsValid_Policy_NullptrOnly{}))
    {
        ck::insights_analyzer::Error(TEXT("AnalyzeFrame: No frame provider"));
        return Result;
    }

    const auto Frame = FrameProvider->GetFrame(ETraceFrameType::TraceFrameType_Game, FrameIndex);

    if (ck::Is_NOT_Valid(Frame, ck::IsValid_Policy_NullptrOnly{}))
    {
        ck::insights_analyzer::Error(
            TEXT("AnalyzeFrame: Frame {} not found"), FrameIndex);
        return Result;
    }

    const auto GameThreadId = Session.GetGameThreadId();
    if (GameThreadId == static_cast<uint32>(INDEX_NONE))
    {
        ck::insights_analyzer::Error(TEXT("AnalyzeFrame: Could not identify game thread"));
        return Result;
    }

    // An unterminated tail frame (capture stopped mid-frame) reports EndTime = +inf;
    // clamp to the capture end so the partial frame analyzes with finite times.
    auto FrameEndTime = Frame->EndTime;
    if (NOT FMath::IsFinite(FrameEndTime))
    {
        FrameEndTime = Session.GetDurationSeconds();
    }

    Result = AnalyzeTimeRange(Session, GameThreadId,
                              Frame->StartTime, FrameEndTime, FrameIndex);
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
    if (NOT Session.IsOpen())
    {
        ck::insights_analyzer::Error(TEXT("AnalyzeTimeRange: Session not open"));
        return {};
    }

    if (NOT IsValidAnalysisWindow(StartTime, EndTime))
    {
        ck::insights_analyzer::Warning(
            TEXT("AnalyzeTimeRange: Invalid time range [{:.6f}, {:.6f}]"),
            StartTime, EndTime);
        return {};
    }

    const auto TimelineIndex = Session.GetTimelineIndex(ThreadId);
    if (TimelineIndex == static_cast<uint32>(INDEX_NONE))
    {
        ck::insights_analyzer::Warning(
            TEXT("AnalyzeTimeRange: No timeline for thread {}"), ThreadId);
        return {};
    }

    return AnalyzeEvents(
        ExtractEvents(Session, TimelineIndex, StartTime, EndTime),
        StartTime, EndTime, ThreadId, FrameIndex);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameAnalyzer::
    AnalyzeEvents(
        const TArray<FCk_TimingEvent>& InEvents,
        double InStartTime, double InEndTime,
        uint32 InThreadId, uint64 InFrameIndex)
    -> FCk_FrameAnalysisResult
{
    if (NOT IsValidAnalysisWindow(InStartTime, InEndTime))
    { return {}; }

    auto Result = FCk_FrameAnalysisResult{};
    Result.FrameIndex = InFrameIndex;
    Result.FrameStartTime = InStartTime;
    Result.FrameEndTime = InEndTime;
    Result.FrameDurationMs = (InEndTime - InStartTime) * 1000.0;
    Result.ThreadId = InThreadId;
    Result.HasValidTimeRange = true;

    Result.Events.Reserve(InEvents.Num());
    for (const FCk_TimingEvent& Event : InEvents)
    {
        if (NOT FMath::IsFinite(Event.StartTime) ||
            (NOT FMath::IsFinite(Event.EndTime) && NOT IsPositiveInfinite(Event.EndTime)))
        { continue; }

        const double ClippedStart = FMath::Max(Event.StartTime, InStartTime);
        const double ClippedEnd = FMath::Min(Event.EndTime, InEndTime);
        if (ClippedEnd <= ClippedStart)
        { continue; }

        Result.Events.Add(FCk_TimingEvent{
            Event.TimerIndex,
            ClippedStart,
            ClippedEnd,
            Event.Depth});
    }

    SortEventsParentBeforeChildren(Result.Events);
    ComputeUnionTotals(Result);

    if (Result.Events.IsEmpty())
    { return Result; }

    uint32 MinDepth = MAX_uint32;
    for (const FCk_TimingEvent& Event : Result.Events)
    {
        if (Event.Depth < MinDepth)
        {
            MinDepth = Event.Depth;
            Result.FrameRootTimerIndex = Event.TimerIndex;
        }
    }

    ComputeExclusiveTimes(Result.Events, Result);
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameAnalyzer::
    AnalyzeThread(const FCk_TraceSession& Session, uint32 ThreadId)
    -> FCk_FrameAnalysisResult
{
    if (NOT Session.IsOpen())
    {
        return FCk_FrameAnalysisResult{};
    }

    const uint32 TimelineIndex = Session.GetTimelineIndex(ThreadId);
    if (TimelineIndex == static_cast<uint32>(INDEX_NONE))
    {
        return FCk_FrameAnalysisResult{};
    }

    double StartTime = 0.0;
    double EndTime = Session.GetDurationSeconds() + 1.0;

    return AnalyzeTimeRange(Session, ThreadId, StartTime, EndTime, 0);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameAnalyzer::
    TryCaptureFrameSnapshot(const FCk_TraceSession& Session,
                            uint64 FrameIndex,
                            FCk_FrameSnapshot& OutSnapshot,
                            const TAtomic<bool>* Cancelled)
    -> bool
{
    OutSnapshot = {};
    if (Cancelled != nullptr && Cancelled->Load())
    {
        OutSnapshot.UnavailableReason = TEXT("Frame capture was cancelled.");
        return false;
    }

    if (Session.GetSession() == nullptr)
    {
        OutSnapshot.UnavailableReason = TEXT("Trace session is not available.");
        return false;
    }

    const bool IsAnalysisComplete = Session.IsAnalysisComplete();
    OutSnapshot.IsProvisional = NOT IsAnalysisComplete;

    auto Events = TArray<FCk_TimingEvent>{};
    auto TimerNames = TMap<uint32, FString>{};
    auto Result = FCk_FrameAnalysisResult{};

    {
        TraceServices::FAnalysisSessionReadScope ReadScope = Session.CreateReadScope();
        const auto FrameProvider = Session.GetFrameProvider();
        const auto TimingProvider = Session.GetTimingProvider();
        const auto ThreadProvider = Session.GetThreadProvider();
        if (ck::Is_NOT_Valid(FrameProvider, ck::IsValid_Policy_NullptrOnly{}) ||
            ck::Is_NOT_Valid(TimingProvider, ck::IsValid_Policy_NullptrOnly{}) ||
            ck::Is_NOT_Valid(ThreadProvider, ck::IsValid_Policy_NullptrOnly{}))
        {
            OutSnapshot.UnavailableReason = TEXT("Trace providers are not available yet.");
            return false;
        }

        const auto Frame = FrameProvider->GetFrame(ETraceFrameType::TraceFrameType_Game, FrameIndex);
        if (ck::Is_NOT_Valid(Frame, ck::IsValid_Policy_NullptrOnly{}))
        {
            OutSnapshot.UnavailableReason = TEXT("Requested game frame is not available yet.");
            return false;
        }

        if (NOT FMath::IsFinite(Frame->StartTime) || Frame->StartTime < 0.0 ||
            Frame->EndTime < Frame->StartTime)
        {
            OutSnapshot.UnavailableReason = TEXT("Requested game frame does not have closed finite boundaries.");
            return false;
        }

        double FrameEndTime = Frame->EndTime;
        if (NOT FMath::IsFinite(FrameEndTime))
        {
            if (FrameEndTime != FrameEndTime || FrameEndTime < 0.0 || NOT IsAnalysisComplete)
            {
                OutSnapshot.UnavailableReason = TEXT("Requested game frame does not have closed finite boundaries.");
                return false;
            }

            FrameEndTime = Session.GetDurationSeconds();
            if (NOT FMath::IsFinite(FrameEndTime) || FrameEndTime < Frame->StartTime)
            {
                OutSnapshot.UnavailableReason = TEXT("Requested game frame cannot be clamped to trace duration.");
                return false;
            }
        }

        uint32 GameThreadId = static_cast<uint32>(INDEX_NONE);
        ThreadProvider->EnumerateThreads(
            [&GameThreadId](const TraceServices::FThreadInfo& Info)
            {
                if (Info.Name != nullptr && FCString::Strcmp(Info.Name, TEXT("GameThread")) == 0)
                {
                    GameThreadId = Info.Id;
                }
            });

        // Keep the legacy completed-trace fallback, but do it inside this scope instead of touching
        // FCk_TraceSession's mutable cache from a worker thread.
        if (GameThreadId == static_cast<uint32>(INDEX_NONE) && IsAnalysisComplete)
        {
            uint64 MaxEvents = 0;
            ThreadProvider->EnumerateThreads(
                [&](const TraceServices::FThreadInfo& Info)
                {
                    uint32 TimelineIndex = 0;
                    if (TimingProvider->GetCpuThreadTimelineIndex(Info.Id, TimelineIndex))
                    {
                        TimingProvider->ReadTimeline(TimelineIndex,
                            [&](const TraceServices::ITimingProfilerProvider::Timeline& Timeline)
                            {
                                const uint64 EventCount = Timeline.GetEventCount();
                                if (EventCount > MaxEvents)
                                {
                                    MaxEvents = EventCount;
                                    GameThreadId = Info.Id;
                                }
                            });
                    }
                });
        }

        if (GameThreadId == static_cast<uint32>(INDEX_NONE))
        {
            OutSnapshot.UnavailableReason = IsAnalysisComplete
                ? TEXT("Could not identify the game thread.")
                : TEXT("GameThread identity is not available yet.");
            return false;
        }

        uint32 TimelineIndex = 0;
        if (NOT TimingProvider->GetCpuThreadTimelineIndex(GameThreadId, TimelineIndex))
        {
            OutSnapshot.UnavailableReason = TEXT("GameThread timeline is not available yet.");
            return false;
        }

        bool WasCancelled = false;
        const bool ReadTimeline = TimingProvider->ReadTimeline(TimelineIndex,
            [&](const TraceServices::ITimingProfilerProvider::Timeline& Timeline)
            {
                Timeline.EnumerateEvents(Frame->StartTime, FrameEndTime,
                    [&](double EventStartTime, double EventEndTime, uint32 EventDepth,
                        const TraceServices::FTimingProfilerEvent& Event)
                        -> TraceServices::EEventEnumerate
                    {
                        if (Cancelled != nullptr && Cancelled->Load())
                        {
                            WasCancelled = true;
                            return TraceServices::EEventEnumerate::Stop;
                        }

                        Events.Add(FCk_TimingEvent{
                            Event.TimerIndex,
                            EventStartTime,
                            FMath::IsFinite(EventEndTime) ? EventEndTime : FrameEndTime,
                            EventDepth});
                        return TraceServices::EEventEnumerate::Continue;
                    });
            });

        if (WasCancelled)
        {
            OutSnapshot.UnavailableReason = TEXT("Frame capture was cancelled.");
            return false;
        }
        if (NOT ReadTimeline)
        {
            OutSnapshot.UnavailableReason = TEXT("GameThread timeline is not available yet.");
            return false;
        }
        if (Events.IsEmpty())
        {
            OutSnapshot.UnavailableReason = TEXT("Requested game frame has no timing events yet.");
            return false;
        }

        TSet<uint32> UsedTimerIndices;
        for (const FCk_TimingEvent& Event : Events)
        {
            UsedTimerIndices.Add(Event.TimerIndex);
        }
        TimingProvider->ReadTimers(
            [&TimerNames, &UsedTimerIndices](const TraceServices::ITimingProfilerTimerReader& Reader)
            {
                const uint32 Count = Reader.GetTimerCount();
                for (uint32 Index = 0; Index < Count; ++Index)
                {
                    const TraceServices::FTimingProfilerTimer* Timer = Reader.GetTimer(Index);
                    if (Timer != nullptr && Timer->Name != nullptr && UsedTimerIndices.Contains(Timer->Id))
                    {
                        TimerNames.Add(Timer->Id, FString(Timer->Name));
                    }
                }
            });

        Result.FrameIndex = FrameIndex;
        Result.FrameStartTime = Frame->StartTime;
        Result.FrameEndTime = FrameEndTime;
        Result.FrameDurationMs = (FrameEndTime - Frame->StartTime) * 1000.0;
        Result.ThreadId = GameThreadId;
        Result.Events = MoveTemp(Events);
    }

    // Use the same clipping and accounting as completed-frame analysis after releasing the provider lock.
    Result = AnalyzeEvents(Result.Events, Result.FrameStartTime, Result.FrameEndTime,
        Result.ThreadId, Result.FrameIndex);
    if (Cancelled != nullptr && Cancelled->Load())
    {
        OutSnapshot.UnavailableReason = TEXT("Frame capture was cancelled.");
        return false;
    }

    if (Result.Events.IsEmpty())
    {
        OutSnapshot.UnavailableReason = TEXT("Requested game frame has no timing events yet.");
        return false;
    }

    // Clipping can discard boundary-only events; retain names only for events in the published snapshot.
    auto ReferencedTimers = TSet<uint32>{};
    for (const FCk_TimingEvent& Event : Result.Events)
    { ReferencedTimers.Add(Event.TimerIndex); }
    for (auto It = TimerNames.CreateIterator(); It; ++It)
    {
        if (NOT ReferencedTimers.Contains(It.Key()))
        { It.RemoveCurrent(); }
    }

    OutSnapshot.Result = MoveTemp(Result);
    OutSnapshot.TimerNames = MoveTemp(TimerNames);
    return true;
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
                    // AnalyzeEvents owns boundary handling: it accepts a +inf capture tail,
                    // but rejects all other malformed non-finite event bounds.
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

    // (start, depth) order puts a parent on the stack before a child sharing its start time.
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
        { return EA.StartTime < EB.StartTime; }
        if (EA.Depth != EB.Depth)
        { return EA.Depth < EB.Depth; }
        if (EA.EndTime != EB.EndTime)
        { return EA.EndTime > EB.EndTime; }
        return EA.TimerIndex < EB.TimerIndex;
    });

    TArray<double> RemainingExclusiveSeconds;
    RemainingExclusiveSeconds.SetNum(SortedIndices.Num());

    for (int32 i = 0; i < SortedIndices.Num(); ++i)
    {
        const FCk_TimingEvent& Evt = Events[SortedIndices[i]];
        RemainingExclusiveSeconds[i] = Evt.EndTime - Evt.StartTime;
    }

    struct FStackEntry
    {
        uint32 Depth;
        double EndTime;
        int32  SortedIndex;
        uint32 TimerIndex;
    };
    TArray<FStackEntry> Stack;
    Stack.Reserve(64);

    for (int32 i = 0; i < SortedIndices.Num(); ++i)
    {
        const FCk_TimingEvent& Evt = Events[SortedIndices[i]];
        const double Inclusive = Evt.EndTime - Evt.StartTime;

        double& InclAccum = Result.TimerInclusive.FindOrAdd(Evt.TimerIndex, 0.0);
        InclAccum += Inclusive;

        uint32& CountAccum = Result.TimerCount.FindOrAdd(Evt.TimerIndex, 0);
        CountAccum += 1;

        while (Stack.Num() > 0 && Stack.Last().EndTime <= Evt.StartTime)
        {
            Stack.Pop(EAllowShrinking::No);
        }

        for (int32 j = Stack.Num() - 1; j >= 0; --j)
        {
            const FStackEntry& Parent = Stack[j];
            if (Parent.Depth < Evt.Depth && Parent.EndTime >= Evt.EndTime)
            {
                RemainingExclusiveSeconds[Parent.SortedIndex] -= Inclusive;

                TMap<uint32, double>& ChildMap =
                    Result.ChildrenOf.FindOrAdd(Parent.TimerIndex);
                double& ChildIncl = ChildMap.FindOrAdd(Evt.TimerIndex, 0.0);
                ChildIncl += Inclusive;
                break;
            }
        }

        Stack.Push(FStackEntry{ Evt.Depth, Evt.EndTime, i, Evt.TimerIndex });
    }

    for (int32 i = 0; i < SortedIndices.Num(); ++i)
    {
        const FCk_TimingEvent& Evt = Events[SortedIndices[i]];
        const double Excl = FMath::Max(0.0, RemainingExclusiveSeconds[i]);

        double& ExclAccum = Result.TimerExclusive.FindOrAdd(Evt.TimerIndex, 0.0);
        ExclAccum += Excl;
    }
}

// --------------------------------------------------------------------------------------------------------------------
