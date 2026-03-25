#include "CkInsightsAnalyzer/Report/CkMultiFrameReport.h"
#include "CkInsightsAnalyzer/Core/CkTraceSession.h"
#include "CkInsightsAnalyzer_Log.h"

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

// --------------------------------------------------------------------------------------------------------------------
// Analysis
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_MultiFrameReport::
    DoAnalyzeFrameRange(const FCk_TraceSession& Session,
                        uint64 StartFrame, uint64 EndFrame)
    -> bool
{
    _Stats = FCk_MultiFrameStats{};

    if (!Session.IsOpen())
    {
        UE_LOG(CkInsightsAnalyzer, Error, TEXT("MultiFrameReport: Session not open"));
        return false;
    }

    const uint64 TotalFrames = Session.GetFrameCount();
    if (TotalFrames == 0)
    {
        UE_LOG(CkInsightsAnalyzer, Warning, TEXT("MultiFrameReport: No frames in trace"));
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

    // Build timer name map once
    TMap<uint32, FString> TimerNames;
    Session.ReadTimers(
        [&TimerNames](const TraceServices::ITimingProfilerTimerReader& Reader)
        {
            const uint32 Count = Reader.GetTimerCount();
            for (uint32 i = 0; i < Count; ++i)
            {
                if (const TraceServices::FTimingProfilerTimer* Timer = Reader.GetTimer(i))
                {
                    if (Timer->Name)
                    {
                        TimerNames.Add(Timer->Id, FString(Timer->Name));
                    }
                }
            }
        });

    // Per-category, per-frame exclusive time accumulation
    // CategoryName → array of per-frame exclusive ms
    TMap<FString, TArray<double>> CategoryPerFrame;

    // Analyze each frame
    const uint64 FrameCount = EndFrame - StartFrame;
    _Stats.FrameCount = FrameCount;
    _Stats.FrameDurationsMs.Reserve(FrameCount);

    UE_LOG(CkInsightsAnalyzer, Log,
        TEXT("MultiFrameReport: Analyzing frames %llu-%llu (%llu frames)"),
        StartFrame, EndFrame - 1, FrameCount);

    // Track all frame summaries for worst-frame identification
    TArray<FCk_FrameSummary> AllSummaries;
    AllSummaries.Reserve(FrameCount);

    for (uint64 FrameIdx = StartFrame; FrameIdx < EndFrame; ++FrameIdx)
    {
        FCk_FrameAnalysisResult Result = FCk_FrameAnalyzer::AnalyzeFrame(Session, FrameIdx);
        if (!Result.IsValid()) continue;

        const double DurationMs = Result.FrameDurationMs;
        _Stats.FrameDurationsMs.Add(DurationMs);

        // Identify dominant cost
        auto [DomCost, DomMs] = IdentifyDominantCost(Result, TimerNames);

        FCk_FrameSummary Summary;
        Summary.FrameIndex = FrameIdx;
        Summary.DurationMs = DurationMs;
        Summary.DominantCost = MoveTemp(DomCost);
        Summary.DominantCostMs = DomMs;
        AllSummaries.Add(MoveTemp(Summary));

        // Accumulate per-category exclusive time for this frame
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
    }

    if (_Stats.FrameDurationsMs.Num() == 0)
    {
        return false;
    }

    // Sort durations for percentile calculations
    _Stats.FrameDurationsMs.Sort();

    _Stats.MinFrameMs = _Stats.FrameDurationsMs[0];
    _Stats.MaxFrameMs = _Stats.FrameDurationsMs.Last();
    _Stats.P95FrameMs = Percentile(_Stats.FrameDurationsMs, 95.0);
    _Stats.P99FrameMs = Percentile(_Stats.FrameDurationsMs, 99.0);

    double Sum = 0.0;
    for (double D : _Stats.FrameDurationsMs) Sum += D;
    _Stats.AvgFrameMs = Sum / _Stats.FrameDurationsMs.Num();

    // Worst frames
    AllSummaries.Sort([](const FCk_FrameSummary& A, const FCk_FrameSummary& B)
    {
        return A.DurationMs > B.DurationMs;
    });

    const int32 WorstCount = FMath::Min(AllSummaries.Num(), _Config.WorstFrameCount);
    for (int32 i = 0; i < WorstCount; ++i)
    {
        _Stats.WorstFrames.Add(AllSummaries[i]);
    }
    if (AllSummaries.Num() > 0)
    {
        _Stats.WorstFrameIndex = AllSummaries[0].FrameIndex;
    }

    // Category averages and P95
    for (auto& [CatName, PerFrame] : CategoryPerFrame)
    {
        if (PerFrame.Num() == 0) continue;

        // Pad with zeros for frames that didn't have this category
        while (PerFrame.Num() < static_cast<int32>(_Stats.FrameCount))
        {
            PerFrame.Add(0.0);
        }

        PerFrame.Sort();

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

    // Sort by average exclusive time descending
    _Stats.CategoryAverages.Sort(
        [](const FCk_MultiFrameStats::FCategoryStats& A,
           const FCk_MultiFrameStats::FCategoryStats& B)
        {
            return A.AvgExclMs > B.AvgExclMs;
        });

    return true;
}

auto
    FCk_MultiFrameReport::
    IdentifyDominantCost(const FCk_FrameAnalysisResult& Result,
                         const TMap<uint32, FString>& TimerNames) const
    -> TPair<FString, double>
{
    // Find the timer with the highest exclusive time
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
    if (!DoAnalyzeFrameRange(Session, StartFrame, EndFrame))
    {
        return TEXT("(No frame data to analyze)");
    }
    return GenerateReport();
}

auto
    FCk_MultiFrameReport::
    AnalyzeWorstFrames(const FCk_TraceSession& Session, int32 Count)
    -> FString
{
    _Config.WorstFrameCount = Count;

    // Analyze all frames to find the worst ones
    if (!DoAnalyzeFrameRange(Session, 0, 0))
    {
        return TEXT("(No frame data to analyze)");
    }
    return GenerateReport();
}

// --------------------------------------------------------------------------------------------------------------------
// Report Generation
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_MultiFrameReport::
    GenerateReport() const
    -> FString
{
    TArray<FString> Lines;
    Lines.Reserve(64);

    const double OverBudget = _Stats.AvgFrameMs / _Config.TargetFrameMs;
    const FString Icon = FCk_TimerCategorizer::SeverityIcon(_Stats.AvgFrameMs);

    // Header
    Lines.Add(FString::Printf(
        TEXT("%s *Trace Analysis: %llu frames, avg %.1fms (%.1fx budget)*"),
        *Icon, _Stats.FrameCount, _Stats.AvgFrameMs, OverBudget));

    Lines.Add(FString::Printf(
        TEXT("   P95: %s | P99: %s | Worst: %s (frame #%llu)"),
        *FCk_TimerCategorizer::FormatMs(_Stats.P95FrameMs),
        *FCk_TimerCategorizer::FormatMs(_Stats.P99FrameMs),
        *FCk_TimerCategorizer::FormatMs(_Stats.MaxFrameMs),
        _Stats.WorstFrameIndex));

    // Top Worst Frames
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

    // Category Averages
    if (_Config.bShowCategoryAverages && _Stats.CategoryAverages.Num() > 0)
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

    return FString::Join(Lines, TEXT("\n"));
}

// --------------------------------------------------------------------------------------------------------------------
