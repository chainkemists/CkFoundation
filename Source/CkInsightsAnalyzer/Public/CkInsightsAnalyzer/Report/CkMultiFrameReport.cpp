#include "CkInsightsAnalyzer/Report/CkMultiFrameReport.h"
#include "CkInsightsAnalyzer/Core/CkTraceSession.h"
#include "CkInsightsAnalyzer_Log.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
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

    if (EndFrame == 0 || EndFrame > TotalFrames)
    {
        EndFrame = TotalFrames;
    }
    if (StartFrame >= EndFrame)
    {
        return false;
    }

    TraceServices::FAnalysisSessionReadScope ReadScope = Session.CreateReadScope();

    const auto TimerNames = FCk_FrameReport::BuildTimerNameMap(Session);

    TMap<FString, TArray<double>> CategoryPerFrame;

    // Per-timer accumulation across the whole analysed range. Exclusive is kept per frame so a
    // percentile is available; inclusive and count only need running totals.
    auto TimerExclusivePerFrame = TMap<uint32, TArray<double>>{};
    auto TimerInclusiveSum = TMap<uint32, double>{};
    auto TimerCallSum = TMap<uint32, uint64>{};

    const auto FrameCount = EndFrame - StartFrame;
    _Stats.FrameCount = FrameCount;
    _Stats.FrameDurationsMs.Reserve(FrameCount);

    ck::insights_analyzer::Log(
        TEXT("MultiFrameReport: Analyzing frames {}-{} ({} frames)"),
        StartFrame, EndFrame - 1, FrameCount);

    TArray<FCk_FrameSummary> AllSummaries;
    AllSummaries.Reserve(FrameCount);

    for (uint64 FrameIdx = StartFrame; FrameIdx < EndFrame; ++FrameIdx)
    {
        FCk_FrameAnalysisResult Result = FCk_FrameAnalyzer::AnalyzeFrame(Session, FrameIdx);
        if (NOT Result.IsValid()) continue;

        if (_Config.ExcludeScreenshotFrames && DoIs_ScreenshotFrame(Result, TimerNames))
        {
            ++_Stats.ExcludedScreenshotFrameCount;
            continue;
        }

        const double DurationMs = Result.FrameDurationMs;
        _Stats.FrameDurationsMs.Add(DurationMs);

        auto [DomCost, DomMs] = IdentifyDominantCost(Result, TimerNames);

        FCk_FrameSummary Summary;
        Summary.FrameIndex = FrameIdx;
        Summary.DurationMs = DurationMs;
        Summary.DominantCost = MoveTemp(DomCost);
        Summary.DominantCostMs = DomMs;
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

    const int32 WorstCount = FMath::Min(AllSummaries.Num(), _Config.WorstFrameCount);
    for (int32 i = 0; i < WorstCount; ++i)
    {
        _Stats.WorstFrames.Add(AllSummaries[i]);

        FCk_FrameAnalysisResult Analysis = FCk_FrameAnalyzer::AnalyzeFrame(Session, AllSummaries[i].FrameIndex);
        if (Analysis.IsValid())
        {
            _Stats.HotFrames.Add(FCk_HotFrameDetails{AllSummaries[i], MoveTemp(Analysis)});
        }
    }
    if (AllSummaries.Num() > 0)
    {
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
