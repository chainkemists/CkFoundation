#pragma once

#include "CoreMinimal.h"
#include "CkInsightsAnalyzer/Core/CkFrameAnalyzer.h"
#include "CkInsightsAnalyzer/Core/CkTimerCategorizer.h"
#include "CkInsightsAnalyzer/Report/CkFrameReport.h"

// --------------------------------------------------------------------------------------------------------------------

class FCk_TraceSession;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Configuration for multi-frame report generation.
 */
struct CKINSIGHTSANALYZER_API FCk_MultiFrameReportConfig
{
    /** Report detail level. Call ApplyDepth() to propagate to individual fields. */
    ECkReportDepth Depth = ECkReportDepth::Standard;

    /** Target frame budget in ms (default 60fps = 16.67ms). */
    double TargetFrameMs = 16.67;

    /** Number of worst frames to show with individual breakdowns. */
    int32 WorstFrameCount = 5;

    /** Minimum category exclusive time (ms) to appear in averages. */
    double MinCategoryMs = 0.3;

    /** Whether to include category averages section. */
    bool bShowCategoryAverages = true;

    /** Set all individual fields from Depth. */
    auto ApplyDepth() -> void
    {
        switch (Depth)
        {
        case ECkReportDepth::Full:
            WorstFrameCount = 10;
            MinCategoryMs = 0.2;
            bShowCategoryAverages = true;
            break;
        case ECkReportDepth::Standard:
            WorstFrameCount = 5;
            MinCategoryMs = 0.3;
            bShowCategoryAverages = true;
            break;
        case ECkReportDepth::Concise:
            WorstFrameCount = 3;
            MinCategoryMs = 0.5;
            bShowCategoryAverages = true;
            break;
        case ECkReportDepth::HotPathsOnly:
            WorstFrameCount = 5;
            MinCategoryMs = 0.5;
            bShowCategoryAverages = false;
            break;
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Per-frame summary for the worst-frames list.
 */
struct FCk_FrameSummary
{
    uint64 FrameIndex = 0;
    double DurationMs = 0.0;

    /** The dominant cost in this frame (category + timer name). */
    FString DominantCost;
    double DominantCostMs = 0.0;
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Aggregate statistics across multiple frames.
 */
struct CKINSIGHTSANALYZER_API FCk_MultiFrameStats
{
    uint64 FrameCount = 0;

    double AvgFrameMs = 0.0;
    double MinFrameMs = 0.0;
    double MaxFrameMs = 0.0;
    double P95FrameMs = 0.0;
    double P99FrameMs = 0.0;

    /** Frame index of the worst frame. */
    uint64 WorstFrameIndex = 0;

    /** Per-frame durations (sorted ascending for percentile calc). */
    TArray<double> FrameDurationsMs;

    /** Top worst frames with summaries. */
    TArray<FCk_FrameSummary> WorstFrames;

    /** Per-category average exclusive time across all frames. */
    struct FCategoryStats
    {
        FString Name;
        double AvgExclMs = 0.0;
        double P95ExclMs = 0.0;
        double TotalPct = 0.0;
    };
    TArray<FCategoryStats> CategoryAverages;
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Generates multi-frame analysis reports from a trace session.
 *
 * Analyzes a range of frames (or the worst N frames) and produces:
 * - Aggregate statistics (avg/min/max/p95/p99 frame time)
 * - Top worst frames with dominant cost identification
 * - Per-category average exclusive time with P95
 *
 * Usage:
 *   FCk_MultiFrameReport Report;
 *   FString Markdown = Report.AnalyzeAndGenerate(Session, 0, 500);
 *   // or:
 *   FString Markdown = Report.AnalyzeWorstFrames(Session, 10);
 */
class CKINSIGHTSANALYZER_API FCk_MultiFrameReport
{
public:
    FCk_MultiFrameReport();
    explicit FCk_MultiFrameReport(const FCk_MultiFrameReportConfig& Config);

    /**
     * Analyze a range of frames and generate a report.
     *
     * @param Session      Open trace session
     * @param StartFrame   First frame index (inclusive)
     * @param EndFrame     Last frame index (exclusive), or 0 for all frames
     * @return Slack markdown string
     */
    auto AnalyzeAndGenerate(const FCk_TraceSession& Session,
                            uint64 StartFrame = 0, uint64 EndFrame = 0) -> FString;

    /**
     * Find and analyze the worst N frames in the trace.
     *
     * @param Session   Open trace session
     * @param Count     Number of worst frames to analyze
     * @return Slack markdown string
     */
    auto AnalyzeWorstFrames(const FCk_TraceSession& Session, int32 Count = 10) -> FString;

    /** Get the stats from the last analysis (available after Generate/AnalyzeWorstFrames). */
    auto GetStats() const -> const FCk_MultiFrameStats& { return _Stats; }

    /** Get/set the report configuration. */
    auto GetConfig() const -> const FCk_MultiFrameReportConfig& { return _Config; }
    auto SetConfig(const FCk_MultiFrameReportConfig& Config) -> void { _Config = Config; }

private:

    /** Analyze frames and populate _Stats. Returns false if no frames. */
    auto DoAnalyzeFrameRange(const FCk_TraceSession& Session,
                             uint64 StartFrame, uint64 EndFrame) -> bool;

    /** Identify dominant cost for a single frame result. */
    auto IdentifyDominantCost(const FCk_FrameAnalysisResult& Result,
                              const TMap<uint32, FString>& TimerNames) const
        -> TPair<FString, double>;

    /** Generate the markdown report from populated _Stats. */
    auto GenerateReport() const -> FString;

    /** Compute percentile from a sorted array. */
    static auto Percentile(const TArray<double>& SortedValues, double P) -> double;

    FCk_MultiFrameReportConfig _Config;
    FCk_MultiFrameStats _Stats;
    FCk_TimerCategorizer _Categorizer;
};

// --------------------------------------------------------------------------------------------------------------------
