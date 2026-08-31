#include "CkInsightsAnalyzer/Report/CkMultiFrameReport.h"
#include "CkInsightsAnalyzer/Core/CkTraceSession.h"
#include "CkInsightsAnalyzer_Log.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Macros/CkMacros.h"

#include "Algo/Accumulate.h"
#include "Containers/ArrayView.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_multi_frame_report
{
    // The scope the engine opens on the game thread while it prepares a trace screenshot. Matching
    // the prepare scope rather than the readback stall is deliberate: the stall shows up as generic
    // idle time, which a genuinely GPU-bound frame produces too.
    const FString ScreenshotScope = TEXT("ScreenshotTracing");

    // The human report gets a readable slice; the JSON carries the full TimerAverageCount rows.
    constexpr int32 MarkdownTimerRows = 25;

    // Identity fields of an averaged frame (root timer, thread) are constant across the frames of
    // any real selection; the vote exists so a malformed frame in the middle cannot redefine them.
    auto Get_DominantVote(const TMap<uint32, int32>& InVotes, uint32 InFallback) -> uint32
    {
        auto Dominant = InFallback;
        auto DominantVotes = 0;

        for (const auto& [Value, Votes] : InVotes)
        {
            const auto Wins = Votes > DominantVotes || (Votes == DominantVotes && Value < Dominant);

            if (Wins)
            {
                Dominant = Value;
                DominantVotes = Votes;
            }
        }

        return Dominant;
    }

    // A node absent from a frame's tree cannot record 0.0: that is also a legitimate magnitude for a
    // node that ran for no measurable time, and the strip has to draw the two differently.
    constexpr auto AbsentInclusiveMs = -1.0f;

    // Identity of a hot-path row within one parent. The raw name alone is not enough: the same timer
    // legitimately appears under different collapsed wrapper chains, and merging those would
    // attribute cost to a call path that never ran.
    struct FHotPathNodeKey
    {
        FString RawName;
        TArray<FString> Breadcrumbs;

        auto operator==(const FHotPathNodeKey& InOther) const -> bool
        {
            return RawName == InOther.RawName && Breadcrumbs == InOther.Breadcrumbs;
        }
    };

    auto GetTypeHash(const FHotPathNodeKey& InKey) -> uint32
    {
        auto Hash = GetTypeHash(InKey.RawName);

        ck::algo::ForEach(InKey.Breadcrumbs, [&Hash](const FString& InBreadcrumb)
        {
            Hash = HashCombine(Hash, GetTypeHash(InBreadcrumb));
        });

        return Hash;
    }

    // Flat pool entry, in the same shape as FCk_JsonReport's call-tree pool: children are indices, so
    // the recursion never holds a reference across an insertion.
    struct FHotPathMergeEntry
    {
        FCk_MergedHotPathNode Node;
        double InclusiveSumMs = 0.0;
        double ExclusiveSumMs = 0.0;
        double CountSum = 0.0;
        TMap<FHotPathNodeKey, int32> ChildIndexByKey;
        TArray<int32> ChildIndices;
    };

    auto Ingest_HotPathNodes(
        const TArray<TSharedPtr<FCk_HotPathNode>>& InNodes,
        int32 InFrameOrdinal,
        int32 InTotalFrames,
        int32 InParentEntryIndex,
        TMap<FHotPathNodeKey, int32>& InOutRootIndexByKey,
        TArray<int32>& InOutRootIndices,
        TArray<FHotPathMergeEntry>& InOutPool)
        -> void
    {
        const auto IsRootLevel = InParentEntryIndex == INDEX_NONE;

        for (const auto& Source : InNodes)
        {
            if (NOT Source.IsValid())
            { continue; }

            const auto Key = FHotPathNodeKey{Source->RawName, Source->Breadcrumbs};

            const auto* Found = IsRootLevel
                ? InOutRootIndexByKey.Find(Key)
                : InOutPool[InParentEntryIndex].ChildIndexByKey.Find(Key);

            auto EntryIndex = Found != nullptr ? *Found : INDEX_NONE;

            if (EntryIndex == INDEX_NONE)
            {
                auto Entry = FHotPathMergeEntry{};
                Entry.Node.RawName = Source->RawName;
                Entry.Node.DisplayName = Source->DisplayName;
                Entry.Node.Breadcrumbs = Source->Breadcrumbs;
                Entry.Node.bIsAggregate = Source->bIsAggregate;
                Entry.Node.PerFrameInclusiveMs.Init(AbsentInclusiveMs, InTotalFrames);

                EntryIndex = InOutPool.Add(MoveTemp(Entry));

                auto& IndexByKey = IsRootLevel
                    ? InOutRootIndexByKey
                    : InOutPool[InParentEntryIndex].ChildIndexByKey;
                IndexByKey.Add(Key, EntryIndex);

                auto& Order = IsRootLevel ? InOutRootIndices : InOutPool[InParentEntryIndex].ChildIndices;
                Order.Add(EntryIndex);
            }

            {
                auto& Entry = InOutPool[EntryIndex];
                auto& PerFrame = Entry.Node.PerFrameInclusiveMs[InFrameOrdinal];

                PerFrame = (PerFrame < 0.0f ? 0.0f : PerFrame) + static_cast<float>(Source->InclusiveMs);

                Entry.InclusiveSumMs += Source->InclusiveMs;
                Entry.ExclusiveSumMs += Source->ExclusiveMs;
                Entry.CountSum += static_cast<double>(Source->Count);
            }

            Ingest_HotPathNodes(Source->Children, InFrameOrdinal, InTotalFrames, EntryIndex,
                InOutRootIndexByKey, InOutRootIndices, InOutPool);
        }
    }

    auto Emit_MergedHotPathNode(int32 InEntryIndex, const TArray<FHotPathMergeEntry>& InPool)
        -> TSharedPtr<FCk_MergedHotPathNode>
    {
        const auto& Entry = InPool[InEntryIndex];

        auto Node = MakeShared<FCk_MergedHotPathNode>(Entry.Node);

        Node->Children = ck::algo::Transform<TArray<TSharedPtr<FCk_MergedHotPathNode>>>(
            Entry.ChildIndices,
            [&InPool](int32 InChildIndex) -> TSharedPtr<FCk_MergedHotPathNode>
            {
                return Emit_MergedHotPathNode(InChildIndex, InPool);
            });

        ck::algo::Sort(Node->Children,
            [](const TSharedPtr<FCk_MergedHotPathNode>& InLhs,
               const TSharedPtr<FCk_MergedHotPathNode>& InRhs)
            {
                return InLhs->AvgInclusiveMs > InRhs->AvgInclusiveMs;
            });

        return Node;
    }
}

// --------------------------------------------------------------------------------------------------------------------

FCk_MultiFrameReport::FCk_MultiFrameReport() = default;

FCk_MultiFrameReport::FCk_MultiFrameReport(const FCk_MultiFrameReportConfig& Config)
    : _Config(Config)
{
}

// --------------------------------------------------------------------------------------------------------------------
// Percentile
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_MultiFrameReport::
    Percentile(const TArray<double>& SortedValues, double P)
    -> double
{
    if (SortedValues.Num() == 0) return 0.0;
    if (SortedValues.Num() == 1) return SortedValues[0];

    const double Index = (P / 100.0) * (SortedValues.Num() - 1);
    const int32 Lower = FMath::FloorToInt32(Index);
    const int32 Upper = FMath::CeilToInt32(Index);

    if (Lower == Upper || Upper >= SortedValues.Num())
    {
        return SortedValues[FMath::Min(Lower, SortedValues.Num() - 1)];
    }

    const double Frac = Index - Lower;
    return SortedValues[Lower] * (1.0 - Frac) + SortedValues[Upper] * Frac;
}

auto
    FCk_MultiFrameReport::
    PercentileWithLeadingZeros(
        const TArray<double>& InSortedPresentValues,
        int32 InLeadingZeroCount,
        double InPercentile)
    -> double
{
    const auto LeadingZeros = FMath::Max(0, InLeadingZeroCount);
    const auto TotalNum = LeadingZeros + InSortedPresentValues.Num();

    if (TotalNum == 0)
    { return 0.0; }

    const auto ValueAt = [&InSortedPresentValues, LeadingZeros](int32 InValueIndex) -> double
    {
        if (InValueIndex < LeadingZeros || InSortedPresentValues.IsEmpty())
        { return 0.0; }

        const auto PresentIndex =
            FMath::Clamp(InValueIndex - LeadingZeros, 0, InSortedPresentValues.Num() - 1);

        return InSortedPresentValues[PresentIndex];
    };

    if (TotalNum == 1)
    { return ValueAt(0); }

    const auto Index = (InPercentile / 100.0) * (TotalNum - 1);
    const auto Lower = FMath::FloorToInt32(Index);
    const auto Upper = FMath::CeilToInt32(Index);

    if (Lower == Upper || Upper >= TotalNum)
    { return ValueAt(FMath::Min(Lower, TotalNum - 1)); }

    const auto Frac = Index - Lower;

    return ValueAt(Lower) * (1.0 - Frac) + ValueAt(Upper) * Frac;
}

// --------------------------------------------------------------------------------------------------------------------
// Frame runs
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_MultiFrameReport::
    DoIs_ValidRunSelection(
        const TArray<FCk_FrameRun>& InRuns,
        uint64 InTotalFrames)
    -> bool
{
    if (InRuns.IsEmpty() || InTotalFrames == 0)
    { return false; }

    auto PreviousLastFrame = uint64{0};

    for (auto RunIndex = 0; RunIndex < InRuns.Num(); ++RunIndex)
    {
        const auto& Run = InRuns[RunIndex];

        if (Run.FirstFrame > Run.LastFrame)
        { return false; }

        if (Run.LastFrame >= InTotalFrames)
        { return false; }

        if (RunIndex > 0 && Run.FirstFrame <= PreviousLastFrame)
        { return false; }

        PreviousLastFrame = Run.LastFrame;
    }

    return true;
}

auto
    FCk_MultiFrameReport::
    DoGet_SelectedFrameCount(const TArray<FCk_FrameRun>& InRuns)
    -> uint64
{
    return Algo::TransformAccumulate(InRuns,
        [](const FCk_FrameRun& InRun) -> uint64
        {
            return InRun.FirstFrame > InRun.LastFrame
                ? 0
                : (InRun.LastFrame - InRun.FirstFrame) + 1;
        },
        static_cast<uint64>(0));
}

auto
    FCk_MultiFrameReport::
    DoGet_FrameIndices(const TArray<FCk_FrameRun>& InRuns)
    -> TArray<uint64>
{
    auto Indices = TArray<uint64>{};
    Indices.Reserve(static_cast<int32>(
        FMath::Min<uint64>(DoGet_SelectedFrameCount(InRuns), MAX_int32)));

    ck::algo::ForEach(InRuns, [&Indices](const FCk_FrameRun& InRun)
    {
        if (InRun.FirstFrame > InRun.LastFrame)
        { return; }

        for (auto FrameIndex = InRun.FirstFrame; FrameIndex <= InRun.LastFrame; ++FrameIndex)
        {
            Indices.Add(FrameIndex);
        }
    });

    return Indices;
}

auto
    FCk_MultiFrameReport::
    DoGet_FrameRunsLabel(const TArray<FCk_FrameRun>& InRuns)
    -> FString
{
    const auto RunLabels = ck::algo::Transform<TArray<FString>>(InRuns,
        [](const FCk_FrameRun& InRun) -> FString
        {
            return InRun.FirstFrame == InRun.LastFrame
                ? FString::Printf(TEXT("%llu"), InRun.FirstFrame)
                : FString::Printf(TEXT("%llu-%llu"), InRun.FirstFrame, InRun.LastFrame);
        });

    return FString::Join(RunLabels, TEXT(", "));
}

// --------------------------------------------------------------------------------------------------------------------
// Hot-path merge
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_MultiFrameReport::
    DoMerge_HotPathTrees(
        const TArray<TArray<TSharedPtr<FCk_HotPathNode>>>& InPerFrameTrees)
    -> TArray<TSharedPtr<FCk_MergedHotPathNode>>
{
    using namespace ck_multi_frame_report;

    const auto TotalFrames = InPerFrameTrees.Num();

    if (TotalFrames == 0)
    { return {}; }

    auto Pool = TArray<FHotPathMergeEntry>{};
    auto RootIndexByKey = TMap<FHotPathNodeKey, int32>{};
    auto RootIndices = TArray<int32>{};

    constexpr auto NoParent = INDEX_NONE;

    for (auto FrameOrdinal = 0; FrameOrdinal < TotalFrames; ++FrameOrdinal)
    {
        Ingest_HotPathNodes(InPerFrameTrees[FrameOrdinal], FrameOrdinal, TotalFrames, NoParent,
            RootIndexByKey, RootIndices, Pool);
    }

    const auto TotalFramesAsDouble = static_cast<double>(TotalFrames);

    for (auto& Entry : Pool)
    {
        auto PresentSamples = TArray<double>{};
        PresentSamples.Reserve(Entry.Node.PerFrameInclusiveMs.Num());

        ck::algo::ForEach(Entry.Node.PerFrameInclusiveMs, [&PresentSamples](float InInclusiveMs)
        {
            if (InInclusiveMs >= 0.0f)
            { PresentSamples.Add(static_cast<double>(InInclusiveMs)); }
        });

        ck::algo::Sort(PresentSamples);

        Entry.Node.FramesPresent = static_cast<uint64>(PresentSamples.Num());
        Entry.Node.AvgInclusiveMs = Entry.InclusiveSumMs / TotalFramesAsDouble;
        Entry.Node.AvgExclusiveMs = Entry.ExclusiveSumMs / TotalFramesAsDouble;
        Entry.Node.AvgCount = Entry.CountSum / TotalFramesAsDouble;
        Entry.Node.HitAvgInclusiveMs = PresentSamples.IsEmpty()
            ? 0.0
            : Entry.InclusiveSumMs / static_cast<double>(PresentSamples.Num());
        Entry.Node.P95InclusiveMs = Percentile(PresentSamples, 95.0);
        Entry.Node.MaxInclusiveMs = PresentSamples.IsEmpty() ? 0.0 : PresentSamples.Last();
    }

    auto Roots = ck::algo::Transform<TArray<TSharedPtr<FCk_MergedHotPathNode>>>(RootIndices,
        [&Pool](int32 InRootIndex) -> TSharedPtr<FCk_MergedHotPathNode>
        {
            return Emit_MergedHotPathNode(InRootIndex, Pool);
        });

    ck::algo::Sort(Roots,
        [](const TSharedPtr<FCk_MergedHotPathNode>& InLhs, const TSharedPtr<FCk_MergedHotPathNode>& InRhs)
        {
            return InLhs->AvgInclusiveMs > InRhs->AvgInclusiveMs;
        });

    return Roots;
}

// --------------------------------------------------------------------------------------------------------------------
// Analysis
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_MultiFrameReport::
    DoIs_ScreenshotFrame(
        const FCk_FrameAnalysisResult& InResult,
        const TMap<uint32, FString>& InTimerNames)
    -> bool
{
    return ck::algo::AnyOf(InResult.TimerExclusive, [&InTimerNames](const auto& InTimerEntry)
    {
        const auto* TimerName = InTimerNames.Find(InTimerEntry.Key);

        return TimerName != nullptr &&
               TimerName->Contains(ck_multi_frame_report::ScreenshotScope, ESearchCase::IgnoreCase);
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_MultiFrameReport::
    DoBuild_TimerAverages(
        const TMap<uint32, FString>& InTimerNames,
        TMap<uint32, TArray<double>>& InTimerExclusivePerFrame,
        const TMap<uint32, double>& InTimerInclusiveSum,
        const TMap<uint32, uint64>& InTimerCallSum)
    -> void
{
    if (_Config.TimerAverageCount <= 0 || _Stats.FrameCount == 0)
    { return; }

    const auto AnalysedFrames = static_cast<int32>(_Stats.FrameCount);
    const auto AnalysedFramesAsDouble = static_cast<double>(_Stats.FrameCount);

    for (auto& [TimerIndex, PerFrame] : InTimerExclusivePerFrame)
    {
        if (PerFrame.IsEmpty())
        { continue; }

        const auto FramesPresent = static_cast<uint64>(PerFrame.Num());
        const auto AbsentFrames = FMath::Max(0, AnalysedFrames - PerFrame.Num());

        ck::algo::Sort(PerFrame);

        // Divide by the ANALYSED frame count, not by the frames this timer appeared in: the absent
        // frames contribute zero, which is what makes the row read as "cost added to a typical
        // frame" and lets the rows sum toward the frame average.
        const auto AvgExclMs = Algo::Accumulate(PerFrame, 0.0) / AnalysedFramesAsDouble;

        if (AvgExclMs < _Config.MinTimerAverageMs)
        { continue; }

        const auto* FoundName = InTimerNames.Find(TimerIndex);
        const auto TimerName = FoundName != nullptr
            ? *FoundName
            : FString::Printf(TEXT("UNKNOWN_%u"), TimerIndex);

        const auto* FoundInclusiveSum = InTimerInclusiveSum.Find(TimerIndex);
        const auto* FoundCallSum = InTimerCallSum.Find(TimerIndex);

        _Stats.TimerAverages.Add(FCk_MultiFrameStats::FTimerStats
        {
            TimerName,
            _Categorizer.Categorize(TimerName),
            AvgExclMs,
            PercentileWithLeadingZeros(PerFrame, AbsentFrames, 95.0),
            PerFrame.Last(),
            FoundInclusiveSum != nullptr ? (*FoundInclusiveSum / AnalysedFramesAsDouble) : 0.0,
            FoundCallSum != nullptr ? (static_cast<double>(*FoundCallSum) / AnalysedFramesAsDouble) : 0.0,
            FramesPresent
        });
    }

    ck::algo::Sort(_Stats.TimerAverages,
        [](const FCk_MultiFrameStats::FTimerStats& InLhs, const FCk_MultiFrameStats::FTimerStats& InRhs)
        {
            return InLhs.AvgExclMs > InRhs.AvgExclMs;
        });

    if (_Stats.TimerAverages.Num() > _Config.TimerAverageCount)
    { _Stats.TimerAverages.SetNum(_Config.TimerAverageCount); }
}

auto
    FCk_MultiFrameReport::FAveragedFrameAccumulator::
    Accumulate(const FCk_FrameAnalysisResult& InResult)
    -> void
{
    ck::algo::ForEach(InResult.TimerInclusive, [this](const auto& InEntry)
    {
        InclusiveSum.FindOrAdd(InEntry.Key, 0.0) += InEntry.Value;
    });

    ck::algo::ForEach(InResult.TimerExclusive, [this](const auto& InEntry)
    {
        ExclusiveSum.FindOrAdd(InEntry.Key, 0.0) += InEntry.Value;
    });

    ck::algo::ForEach(InResult.TimerCount, [this](const auto& InEntry)
    {
        CountSum.FindOrAdd(InEntry.Key, 0.0) += static_cast<double>(InEntry.Value);
    });

    ++RootTimerVotes.FindOrAdd(InResult.FrameRootTimerIndex, 0);
    ++ThreadIdVotes.FindOrAdd(InResult.ThreadId, 0);
}

auto
    FCk_MultiFrameReport::
    DoBuild_AveragedFrame(const FAveragedFrameAccumulator& InAccumulator)
    -> void
{
    if (_Stats.FrameCount == 0)
    { return; }

    const auto AnalysedFrames = static_cast<double>(_Stats.FrameCount);

    auto Averaged = FCk_FrameAnalysisResult{};
    Averaged.IsSynthesizedAverage = true;
    Averaged.FrameDurationMs = _Stats.AvgFrameMs;
    Averaged.FrameStartTime = 0.0;
    Averaged.FrameEndTime = _Stats.AvgFrameMs / 1000.0;
    Averaged.ThreadId = ck_multi_frame_report::Get_DominantVote(InAccumulator.ThreadIdVotes, 0);
    Averaged.FrameRootTimerIndex = ck_multi_frame_report::Get_DominantVote(
        InAccumulator.RootTimerVotes, static_cast<uint32>(INDEX_NONE));

    Averaged.TimerInclusive.Reserve(InAccumulator.InclusiveSum.Num());
    ck::algo::ForEach(InAccumulator.InclusiveSum, [&Averaged, AnalysedFrames](const auto& InEntry)
    {
        Averaged.TimerInclusive.Add(InEntry.Key, InEntry.Value / AnalysedFrames);
    });

    Averaged.TimerExclusive.Reserve(InAccumulator.ExclusiveSum.Num());
    ck::algo::ForEach(InAccumulator.ExclusiveSum, [&Averaged, AnalysedFrames](const auto& InEntry)
    {
        Averaged.TimerExclusive.Add(InEntry.Key, InEntry.Value / AnalysedFrames);
    });

    Averaged.TimerCount.Reserve(InAccumulator.CountSum.Num());
    ck::algo::ForEach(InAccumulator.CountSum, [&Averaged, AnalysedFrames](const auto& InEntry)
    {
        Averaged.TimerCount.Add(InEntry.Key,
            static_cast<uint32>(FMath::RoundToInt64(InEntry.Value / AnalysedFrames)));
    });

    // The one synthetic event. Consumers of an averaged frame gate on Result.IsValid(), which is
    // Events.Num() > 0 — an averaged frame with no events reads as no data at all.
    constexpr auto RootDepth = uint32{0};
    Averaged.Events.Add(FCk_TimingEvent{
        Averaged.FrameRootTimerIndex, Averaged.FrameStartTime, Averaged.FrameEndTime, RootDepth});

    _Stats.AveragedFrame = MoveTemp(Averaged);
}

auto
    FCk_MultiFrameReport::
    DoBuild_WaitAverages(const TMap<uint32, FWaitAverageAccumulator>& InPerThread)
    -> void
{
    if (_Stats.FrameCount == 0)
    { return; }

    const auto AnalysedFrames = static_cast<double>(_Stats.FrameCount);

    for (const auto& [ThreadId, Accumulated] : InPerThread)
    {
        auto Summary = FCk_WaitThreadSummary{};
        Summary.ThreadId = ThreadId;
        Summary.ThreadName = Accumulated.ThreadName;
        Summary.bIsGameThread = Accumulated.IsGameThread;
        Summary.WaitMs = Accumulated.WaitMsSum / AnalysedFrames;
        Summary.WallMs = Accumulated.WallMsSum / AnalysedFrames;

        auto Scopes = TArray<TPair<FString, double>>{};
        Scopes.Reserve(Accumulated.ScopeExclusiveMsSum.Num());

        ck::algo::ForEach(Accumulated.ScopeExclusiveMsSum, [&Scopes](const auto& InEntry)
        {
            Scopes.Emplace(InEntry.Key, InEntry.Value);
        });

        ck::algo::Sort(Scopes,
            [](const TPair<FString, double>& InLhs, const TPair<FString, double>& InRhs)
            {
                return InLhs.Value > InRhs.Value;
            });

        constexpr auto TopWaitScopes = 3;
        const auto ShownScopes = FMath::Min(Scopes.Num(), TopWaitScopes);

        for (auto ScopeIndex = 0; ScopeIndex < ShownScopes; ++ScopeIndex)
        {
            const auto* FoundCountSum = Accumulated.ScopeCountSum.Find(Scopes[ScopeIndex].Key);

            Summary.TopWaits.Add(FCk_WaitThreadSummary::FWaitScope{
                Scopes[ScopeIndex].Key,
                Scopes[ScopeIndex].Value / AnalysedFrames,
                FoundCountSum != nullptr
                    ? static_cast<uint32>(FMath::RoundToInt64(*FoundCountSum / AnalysedFrames))
                    : 0});
        }

        _Stats.WaitAverages.Add(MoveTemp(Summary));
    }

    ck::algo::Sort(_Stats.WaitAverages,
        [](const FCk_WaitThreadSummary& InLhs, const FCk_WaitThreadSummary& InRhs)
        {
            if (InLhs.bIsGameThread != InRhs.bIsGameThread)
            { return InLhs.bIsGameThread; }

            return InLhs.WaitMs > InRhs.WaitMs;
        });
}

auto
    FCk_MultiFrameReport::
    DoAnalyzeFrameRange(const FCk_TraceSession& Session,
                        uint64 StartFrame, uint64 EndFrame)
    -> bool
{
    _Stats = FCk_MultiFrameStats{};

    if (NOT Session.IsOpen())
    {
        ck::insights_analyzer::Error(TEXT("MultiFrameReport: Session not open"));
        return false;
    }

    const auto TotalFrames = Session.GetFrameCount();
    if (TotalFrames == 0)
    {
        ck::insights_analyzer::Warning(TEXT("MultiFrameReport: No frames in trace"));
        return false;
    }

    const auto EndFrameExclusive = (EndFrame == 0 || EndFrame > TotalFrames) ? TotalFrames : EndFrame;
    if (StartFrame >= EndFrameExclusive)
    {
        return false;
    }

    return DoAnalyzeFrameSet(Session, {FCk_FrameRun{StartFrame, EndFrameExclusive - 1}});
}

auto
    FCk_MultiFrameReport::
    DoAnalyzeFrameSet(const FCk_TraceSession& InSession,
                      const TArray<FCk_FrameRun>& InRuns)
    -> bool
{
    _Stats = FCk_MultiFrameStats{};

    if (NOT InSession.IsOpen())
    {
        ck::insights_analyzer::Error(TEXT("MultiFrameReport: Session not open"));
        return false;
    }

    const auto TotalFrames = InSession.GetFrameCount();
    if (TotalFrames == 0)
    {
        ck::insights_analyzer::Warning(TEXT("MultiFrameReport: No frames in trace"));
        return false;
    }

    // A selection is admitted whole or not at all: analysing the salvageable subset of a malformed
    // one would report averages over frames the caller never asked for, under its own label.
    const auto RunsAreValid = DoIs_ValidRunSelection(InRuns, TotalFrames);
    CK_ENSURE_IF_NOT(RunsAreValid,
        TEXT("MultiFrameReport: rejected frame-run selection [{}] against a trace of [{}] frames - runs must be ")
        TEXT("non-empty, each FirstFrame <= LastFrame, ascending, non-overlapping, and in bounds"),
        DoGet_FrameRunsLabel(InRuns), TotalFrames)
    { return false; }

    const auto SelectedFrames = DoGet_FrameIndices(InRuns);
    if (SelectedFrames.IsEmpty())
    {
        return false;
    }

    _Stats.SelectedRuns = InRuns;

    TraceServices::FAnalysisSessionReadScope ReadScope = InSession.CreateReadScope();

    const auto TimerNames = FCk_FrameReport::BuildTimerNameMap(InSession);

    TMap<FString, TArray<double>> CategoryPerFrame;

    // Per-timer accumulation across the whole analysed range. Exclusive is kept per frame so a
    // percentile is available; inclusive and count only need running totals.
    auto TimerExclusivePerFrame = TMap<uint32, TArray<double>>{};
    auto TimerInclusiveSum = TMap<uint32, double>{};
    auto TimerCallSum = TMap<uint32, uint64>{};

    auto AveragedFrameAccumulator = FAveragedFrameAccumulator{};
    auto WaitPerThread = TMap<uint32, FWaitAverageAccumulator>{};

    auto PerFrameHotPathConfig = FCk_FrameReportConfig{};
    PerFrameHotPathConfig.Depth = _Config.Depth;
    PerFrameHotPathConfig.TargetFrameMs = _Config.TargetFrameMs;
    PerFrameHotPathConfig.ApplyDepth();
    PerFrameHotPathConfig.ShowAllChildren = _Config.ShowAllChildren;

    const auto PerFrameHotPathReport = FCk_FrameReport{PerFrameHotPathConfig};
    auto PerFrameHotPaths = TArray<TArray<TSharedPtr<FCk_HotPathNode>>>{};

    const auto FrameCount = static_cast<uint64>(SelectedFrames.Num());
    _Stats.FrameCount = FrameCount;
    _Stats.FrameDurationsMs.Reserve(FrameCount);
    _Stats.AnalysedFrameIndices.Reserve(FrameCount);

    ck::insights_analyzer::Log(
        TEXT("MultiFrameReport: Analyzing frames {} ({} frames)"),
        DoGet_FrameRunsLabel(InRuns), FrameCount);

    TArray<FCk_FrameSummary> AllSummaries;
    AllSummaries.Reserve(FrameCount);

    for (const uint64 FrameIdx : SelectedFrames)
    {
        FCk_FrameAnalysisResult Result = FCk_FrameAnalyzer::AnalyzeFrame(InSession, FrameIdx);
        if (NOT Result.IsValid()) continue;

        const bool IsScreenshotFrame = DoIs_ScreenshotFrame(Result, TimerNames);
        if (_Config.ExcludeScreenshotFrames && IsScreenshotFrame)
        {
            ++_Stats.ExcludedScreenshotFrameCount;
            continue;
        }
        if (IsScreenshotFrame)
        {
            _Stats.ScreenshotFrameIndices.Add(FrameIdx);
        }

        const double DurationMs = Result.FrameDurationMs;
        _Stats.FrameDurationsMs.Add(DurationMs);

        // The ordinal space every per-frame series hangs off, appended here so a frame that failed to
        // analyse or was excluded above never takes an ordinal.
        _Stats.AnalysedFrameIndices.Add(FrameIdx);

        if (_Config.BuildMergedHotPaths)
        {
            PerFrameHotPaths.Add(
                PerFrameHotPathReport.BuildHotPathTree(InSession, Result, TimerNames));
        }

        auto [DomCost, DomMs] = IdentifyDominantCost(Result, TimerNames);

        FCk_FrameSummary Summary;
        Summary.FrameIndex = FrameIdx;
        Summary.DurationMs = DurationMs;
        Summary.DominantCost = MoveTemp(DomCost);
        Summary.DominantCostMs = DomMs;
        Summary.IsScreenshotFrame = IsScreenshotFrame;
        AllSummaries.Add(MoveTemp(Summary));

        TMap<FString, double> FrameCatExcl;
        for (const auto& [TimerIndex, ExclSec] : Result.TimerExclusive)
        {
            const double ExclMs = ExclSec * 1000.0;
            if (ExclMs < 0.01) continue;

            const FString* NamePtr = TimerNames.Find(TimerIndex);
            const FString TimerName = NamePtr ? *NamePtr : FString::Printf(TEXT("UNKNOWN_%u"), TimerIndex);
            const FString Category = _Categorizer.Categorize(TimerName);

            double& CatTotal = FrameCatExcl.FindOrAdd(Category, 0.0);
            CatTotal += ExclMs;
        }

        for (const auto& [Cat, ExclMs] : FrameCatExcl)
        {
            CategoryPerFrame.FindOrAdd(Cat).Add(ExclMs);
        }

        if (_Config.TimerAverageCount > 0)
        {
            // No 0.01 ms floor here, unlike the category pass above: a timer can be individually
            // tiny and still be one of the biggest rows once it fires a few hundred times a frame.
            // The floor that matters is MinTimerAverageMs, applied to the average at emit time.
            ck::algo::ForEach(Result.TimerExclusive, [&](const auto& InEntry)
            {
                TimerExclusivePerFrame.FindOrAdd(InEntry.Key).Add(InEntry.Value * 1000.0);
            });

            ck::algo::ForEach(Result.TimerInclusive, [&](const auto& InEntry)
            {
                TimerInclusiveSum.FindOrAdd(InEntry.Key, 0.0) += InEntry.Value * 1000.0;
            });

            ck::algo::ForEach(Result.TimerCount, [&](const auto& InEntry)
            {
                TimerCallSum.FindOrAdd(InEntry.Key, 0) += InEntry.Value;
            });
        }

        AveragedFrameAccumulator.Accumulate(Result);

        if (_Config.ComputeWaitAverages)
        {
            // No floor: a thread whose wait is negligible on any single frame can still matter once
            // averaged, and filtering is the consumer's call.
            constexpr auto NoWaitFloor = 0.0;
            const auto FrameWaits =
                FCk_FrameReport::ComputeWaitSummaries(InSession, Result, NoWaitFloor, TimerNames);

            ck::algo::ForEach(FrameWaits, [&WaitPerThread](const FCk_WaitThreadSummary& InWait)
            {
                auto& Accumulated = WaitPerThread.FindOrAdd(InWait.ThreadId);
                Accumulated.ThreadName = InWait.ThreadName;
                Accumulated.IsGameThread = InWait.bIsGameThread;
                Accumulated.WaitMsSum += InWait.WaitMs;
                Accumulated.WallMsSum += InWait.WallMs;

                // TopWaits is already truncated to the frame's three biggest scopes, so the per-scope
                // averages are a top-3 sample while the thread's WaitMs stays exact.
                ck::algo::ForEach(InWait.TopWaits,
                    [&Accumulated](const FCk_WaitThreadSummary::FWaitScope& InScope)
                    {
                        Accumulated.ScopeExclusiveMsSum.FindOrAdd(InScope.Name, 0.0) += InScope.ExclusiveMs;
                        Accumulated.ScopeCountSum.FindOrAdd(InScope.Name, 0.0) +=
                            static_cast<double>(InScope.Count);
                    });
            });
        }
    }

    if (_Stats.FrameDurationsMs.Num() == 0)
    {
        return false;
    }

    // Frames that failed to analyse, and frames excluded as capture artifacts, must not stay in
    // the divisor — otherwise every average below reads low by exactly the share that was skipped.
    _Stats.FrameCount = static_cast<uint64>(_Stats.FrameDurationsMs.Num());

    ck::algo::Sort(_Stats.FrameDurationsMs);

    _Stats.MinFrameMs = _Stats.FrameDurationsMs[0];
    _Stats.MaxFrameMs = _Stats.FrameDurationsMs.Last();
    _Stats.P95FrameMs = Percentile(_Stats.FrameDurationsMs, 95.0);
    _Stats.P99FrameMs = Percentile(_Stats.FrameDurationsMs, 99.0);

    double Sum = 0.0;
    for (double D : _Stats.FrameDurationsMs) Sum += D;
    _Stats.AvgFrameMs = Sum / _Stats.FrameDurationsMs.Num();

    ck::algo::Sort(AllSummaries, [](const FCk_FrameSummary& A, const FCk_FrameSummary& B)
    {
        return A.DurationMs > B.DurationMs;
    });

    // Screenshot frames are capture cost, not game cost: the readback stall + PNG compress made
    // them own this list on every screenshot-enabled capture (2026-08-26: 6 of the 10 worst-frame
    // slots across both reports), drowning the real spikes. They stay in the averages when not
    // excluded — ScreenshotFrameIndices reports them so the ranking skip hides nothing.
    for (const FCk_FrameSummary& Candidate : AllSummaries)
    {
        if (_Stats.WorstFrames.Num() >= _Config.WorstFrameCount)
        { break; }
        if (Candidate.IsScreenshotFrame)
        { continue; }

        _Stats.WorstFrames.Add(Candidate);

        FCk_FrameAnalysisResult Analysis = FCk_FrameAnalyzer::AnalyzeFrame(InSession, Candidate.FrameIndex);
        if (Analysis.IsValid())
        {
            _Stats.HotFrames.Add(FCk_HotFrameDetails{Candidate, MoveTemp(Analysis)});
        }
    }
    if (_Stats.WorstFrames.Num() > 0)
    {
        _Stats.WorstFrameIndex = _Stats.WorstFrames[0].FrameIndex;
    }
    else if (AllSummaries.Num() > 0)
    {
        // Every analysed frame was a screenshot frame — degenerate, but stay truthful.
        _Stats.WorstFrameIndex = AllSummaries[0].FrameIndex;
    }

    for (auto& [CatName, PerFrame] : CategoryPerFrame)
    {
        if (PerFrame.Num() == 0) continue;

        // Pad with zeros for frames that didn't have this category
        while (PerFrame.Num() < static_cast<int32>(_Stats.FrameCount))
        {
            PerFrame.Add(0.0);
        }

        ck::algo::Sort(PerFrame);

        double CatSum = 0.0;
        for (double V : PerFrame) CatSum += V;
        const double CatAvg = CatSum / PerFrame.Num();
        const double CatP95 = Percentile(PerFrame, 95.0);

        if (CatAvg < _Config.MinCategoryMs) continue;

        const double CatPct = (_Stats.AvgFrameMs > 0.0) ? (CatAvg / _Stats.AvgFrameMs) * 100.0 : 0.0;

        _Stats.CategoryAverages.Add(FCk_MultiFrameStats::FCategoryStats{
            CatName, CatAvg, CatP95, CatPct
        });
    }

    ck::algo::Sort(_Stats.CategoryAverages,
        [](const FCk_MultiFrameStats::FCategoryStats& A,
           const FCk_MultiFrameStats::FCategoryStats& B)
        {
            return A.AvgExclMs > B.AvgExclMs;
        });

    DoBuild_TimerAverages(TimerNames, TimerExclusivePerFrame, TimerInclusiveSum, TimerCallSum);
    DoBuild_AveragedFrame(AveragedFrameAccumulator);

    if (_Config.BuildMergedHotPaths)
    {
        _Stats.MergedHotPaths = DoMerge_HotPathTrees(PerFrameHotPaths);
    }

    if (_Config.ComputeWaitAverages)
    {
        DoBuild_WaitAverages(WaitPerThread);
    }

    return true;
}

auto
    FCk_MultiFrameReport::
    IdentifyDominantCost(const FCk_FrameAnalysisResult& Result,
                         const TMap<uint32, FString>& TimerNames) const
    -> TPair<FString, double>
{
    uint32 MaxTimerIndex = 0;
    double MaxExclMs = 0.0;

    for (const auto& [TimerIndex, ExclSec] : Result.TimerExclusive)
    {
        const double ExclMs = ExclSec * 1000.0;
        if (ExclMs > MaxExclMs)
        {
            MaxExclMs = ExclMs;
            MaxTimerIndex = TimerIndex;
        }
    }

    if (MaxExclMs < 0.01)
    {
        return { TEXT("(idle)"), 0.0 };
    }

    const FString* NamePtr = TimerNames.Find(MaxTimerIndex);
    const FString TimerName = NamePtr ? *NamePtr : TEXT("Unknown");
    const FString SimpleName = FCk_TimerCategorizer::SimplifyName(TimerName);
    const FString Category = _Categorizer.Categorize(TimerName);

    return { FString::Printf(TEXT("%s (%s)"), *Category, *SimpleName), MaxExclMs };
}

// --------------------------------------------------------------------------------------------------------------------
// Public API
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_MultiFrameReport::
    AnalyzeAndGenerate(const FCk_TraceSession& Session,
                       uint64 StartFrame, uint64 EndFrame)
    -> FString
{
    if (NOT DoAnalyzeFrameRange(Session, StartFrame, EndFrame))
    {
        return TEXT("(No frame data to analyze)");
    }
    return GenerateReport(Session);
}

auto
    FCk_MultiFrameReport::
    AnalyzeFrameSet(const FCk_TraceSession& InSession,
                    const TArray<FCk_FrameRun>& InRuns)
    -> FString
{
    if (NOT DoAnalyzeFrameSet(InSession, InRuns))
    {
        return TEXT("(No frame data to analyze)");
    }
    return GenerateReport(InSession);
}

auto
    FCk_MultiFrameReport::
    AnalyzeWorstFrames(const FCk_TraceSession& Session, int32 Count)
    -> FString
{
    _Config.WorstFrameCount = Count;

    constexpr uint64 FromFirstFrame = 0;
    constexpr uint64 ToEndOfTrace = 0;
    if (NOT DoAnalyzeFrameRange(Session, FromFirstFrame, ToEndOfTrace))
    {
        return TEXT("(No frame data to analyze)");
    }
    return GenerateReport(Session);
}

// --------------------------------------------------------------------------------------------------------------------
// Report Generation
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_MultiFrameReport::
    GenerateReport(const FCk_TraceSession& Session) const
    -> FString
{
    TArray<FString> Lines;
    Lines.Reserve(64);

    FCk_FrameReport::GenerateTraceOverview(Session, Lines);

    const double OverBudget = _Stats.AvgFrameMs / _Config.TargetFrameMs;
    const FString Icon = FCk_TimerCategorizer::SeverityIcon(_Stats.AvgFrameMs);

    Lines.Add(FString::Printf(
        TEXT("%s *Trace Analysis: %llu frames, avg %.1fms (%.1fx budget)*"),
        *Icon, _Stats.FrameCount, _Stats.AvgFrameMs, OverBudget));

    Lines.Add(FString::Printf(
        TEXT("   P95: %s | P99: %s | Worst: %s (frame #%llu)"),
        *FCk_TimerCategorizer::FormatMs(_Stats.P95FrameMs),
        *FCk_TimerCategorizer::FormatMs(_Stats.P99FrameMs),
        *FCk_TimerCategorizer::FormatMs(_Stats.MaxFrameMs),
        _Stats.WorstFrameIndex));

    // Only for a disjoint selection. A contiguous run is already fully described by the frame count
    // above, and the module's report-ordering rule wants contiguous reports byte-comparable with
    // every report generated before multi-run selection existed.
    if (_Stats.SelectedRuns.Num() > 1)
    {
        Lines.Add(FString::Printf(
            TEXT("   Frames %s (%llu frames)"),
            *DoGet_FrameRunsLabel(_Stats.SelectedRuns),
            DoGet_SelectedFrameCount(_Stats.SelectedRuns)));
    }

    if (_Stats.WorstFrames.Num() > 0)
    {
        Lines.Add(TEXT(""));
        Lines.Add(FString::Printf(TEXT("*Top %d Worst Frames*"), _Stats.WorstFrames.Num()));

        for (int32 i = 0; i < _Stats.WorstFrames.Num(); ++i)
        {
            const FCk_FrameSummary& F = _Stats.WorstFrames[i];
            Lines.Add(FString::Printf(
                TEXT("  %d. Frame #%llu: %s \u2014 %s %s"),
                i + 1, F.FrameIndex,
                *FCk_TimerCategorizer::FormatMs(F.DurationMs),
                *F.DominantCost,
                *FCk_TimerCategorizer::FormatMs(F.DominantCostMs)));
        }
    }

    if (_Stats.HotFrames.Num() > 0)
    {
        FCk_FrameReportConfig HotFrameConfig;
        HotFrameConfig.Depth = _Config.Depth;
        HotFrameConfig.TargetFrameMs = _Config.TargetFrameMs;
        HotFrameConfig.ApplyDepth();
        HotFrameConfig.ShowRawTimerList = true;

        FCk_FrameReport HotFrameReport(HotFrameConfig);
        for (int32 i = 0; i < _Stats.HotFrames.Num(); ++i)
        {
            const FCk_HotFrameDetails& HotFrame = _Stats.HotFrames[i];
            Lines.Add(TEXT(""));
            Lines.Add(FString::Printf(
                TEXT("*Hot Frame %d Detail: #%llu (%s)*"),
                i + 1,
                HotFrame.Summary.FrameIndex,
                *FCk_TimerCategorizer::FormatMs(HotFrame.Summary.DurationMs)));
            Lines.Add(HotFrameReport.Generate(Session, HotFrame.Analysis));
        }
    }

    if (_Config.ShowCategoryAverages && _Stats.CategoryAverages.Num() > 0)
    {
        Lines.Add(TEXT(""));
        Lines.Add(FString::Printf(
            TEXT("*Category Averages (across %llu frames)*"), _Stats.FrameCount));

        for (const auto& Cat : _Stats.CategoryAverages)
        {
            const FString CatIcon = FCk_TimerCategorizer::SeverityIcon(Cat.AvgExclMs);
            Lines.Add(FString::Printf(
                TEXT("%s *%8s*  %4.0f%%  %s    [P95: %s]"),
                *CatIcon,
                *FCk_TimerCategorizer::FormatMs(Cat.AvgExclMs),
                Cat.TotalPct,
                *Cat.Name,
                *FCk_TimerCategorizer::FormatMs(Cat.P95ExclMs)));
        }
    }

    if (_Stats.TimerAverages.Num() > 0)
    {
        const auto ShownRows =
            FMath::Min(_Stats.TimerAverages.Num(), ck_multi_frame_report::MarkdownTimerRows);

        Lines.Add(TEXT(""));
        Lines.Add(FString::Printf(
            TEXT("*Top %d Timers by Average Cost (across %llu frames)*"),
            ShownRows, _Stats.FrameCount));

        auto ShownTimers =
            TArrayView<const FCk_MultiFrameStats::FTimerStats>{_Stats.TimerAverages.GetData(), ShownRows};

        ck::algo::ForEach(ShownTimers,
            [&Lines](const FCk_MultiFrameStats::FTimerStats& InTimer)
            {
                Lines.Add(FString::Printf(
                    TEXT("%s *%8s*  %6.1fx  %s  [%s]"),
                    *FCk_TimerCategorizer::SeverityIcon(InTimer.AvgExclMs),
                    *FCk_TimerCategorizer::FormatMs(InTimer.AvgExclMs),
                    InTimer.AvgCount,
                    *FCk_TimerCategorizer::SimplifyName(InTimer.Name),
                    *InTimer.Category));
            });
    }

    if (_Stats.ExcludedScreenshotFrameCount > 0)
    {
        Lines.Add(TEXT(""));
        Lines.Add(FString::Printf(
            TEXT("   (%llu screenshot-polluted frames excluded)"),
            _Stats.ExcludedScreenshotFrameCount));
    }

    return FString::Join(Lines, TEXT("\n"));
}

// --------------------------------------------------------------------------------------------------------------------
