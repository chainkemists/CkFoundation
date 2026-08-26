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
    bool ShowCategoryAverages = true;

    /**
     * How many per-timer rows to report in TimerAverages (0 disables the section).
     *
     * Unlike the hot-frame timer lists, these are averaged over EVERY analysed frame, which is
     * what makes "what is the Other category actually made of, on a typical frame" answerable.
     */
    int32 TimerAverageCount = 100;

    /** Minimum per-timer average exclusive time (ms) to appear in TimerAverages. */
    double MinTimerAverageMs = 0.005;

    /**
     * Skip frames that contain a trace-screenshot scope.
     *
     * A screenshot frame stalls the game thread for 20+ ms in FlushRenderingCommands waiting out
     * the readback, which is capture-tooling cost, not game cost. Left in, those frames own the
     * worst-frame ranking and quietly inflate max/p99.
     */
    bool ExcludeScreenshotFrames = false;

    /** Set all individual fields from Depth. */
    auto ApplyDepth() -> void
    {
        switch (Depth)
        {
        case ECkReportDepth::Full:
            WorstFrameCount = 10;
            MinCategoryMs = 0.2;
            ShowCategoryAverages = true;
            TimerAverageCount = 250;
            break;
        case ECkReportDepth::Standard:
            WorstFrameCount = 5;
            MinCategoryMs = 0.3;
            ShowCategoryAverages = true;
            TimerAverageCount = 100;
            break;
        case ECkReportDepth::Concise:
            WorstFrameCount = 3;
            MinCategoryMs = 0.5;
            ShowCategoryAverages = true;
            TimerAverageCount = 50;
            break;
        case ECkReportDepth::HotPathsOnly:
            WorstFrameCount = 5;
            MinCategoryMs = 0.5;
            ShowCategoryAverages = false;
            TimerAverageCount = 0;
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

    /** Capture-tooling frame (contains a trace-screenshot scope). Never ranked as a worst frame. */
    bool IsScreenshotFrame = false;
};

// --------------------------------------------------------------------------------------------------------------------

/** Complete analysis data for one selected hot frame. */
struct CKINSIGHTSANALYZER_API FCk_HotFrameDetails
{
    FCk_FrameSummary Summary;
    FCk_FrameAnalysisResult Analysis;
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

    /** Full per-frame analysis for the selected worst frames, in the same order as WorstFrames. */
    TArray<FCk_HotFrameDetails> HotFrames;

    /** How many frames were skipped because they contained a trace-screenshot scope. */
    uint64 ExcludedScreenshotFrameCount = 0;

    /**
     * Screenshot frames that stayed in the averages (ExcludeScreenshotFrames off). Reported so
     * their exclusion from the worst-frame ranking hides nothing: readback stalls made them own
     * that list on every capture with screenshots enabled, drowning the real spikes.
     */
    TArray<uint64> ScreenshotFrameIndices;

    /** Per-category average exclusive time across all frames. */
    struct FCategoryStats
    {
        FString Name;
        double AvgExclMs = 0.0;
        double P95ExclMs = 0.0;
        double TotalPct = 0.0;
    };
    TArray<FCategoryStats> CategoryAverages;

    /**
     * Per-timer averages across every analysed frame, sorted by AvgExclMs descending.
     *
     * Averages are over the full analysed frame count — a timer absent from a frame contributes
     * zero for that frame — so AvgExclMs reads as "cost this timer adds to a typical frame" and
     * the rows sum toward the frame average. That is the difference from the hot-frame timer
     * lists, which describe one outlier frame each.
     */
    struct FTimerStats
    {
        FString Name;
        FString Category;
        double AvgExclMs = 0.0;
        double P95ExclMs = 0.0;
        double MaxExclMs = 0.0;
        double AvgInclMs = 0.0;

        /** Average calls per analysed frame. Fractional when the timer is not hit every frame. */
        double AvgCount = 0.0;

        /** How many analysed frames contained this timer at all. */
        uint64 FramesPresent = 0;
    };
    TArray<FTimerStats> TimerAverages;
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

public:
    /** Whether this frame contains a trace-screenshot scope, i.e. is capture-tooling polluted.
     *  Public like Percentile: a pure predicate the spec pins directly. */
    static auto DoIs_ScreenshotFrame(
        const FCk_FrameAnalysisResult& InResult,
        const TMap<uint32, FString>& InTimerNames)
        -> bool;

private:

    /** Reduce the per-frame timer accumulators into _Stats.TimerAverages. Sorts the exclusive samples in place. */
    auto DoBuild_TimerAverages(
        const TMap<uint32, FString>& InTimerNames,
        TMap<uint32, TArray<double>>& InTimerExclusivePerFrame,
        const TMap<uint32, double>& InTimerInclusiveSum,
        const TMap<uint32, uint64>& InTimerCallSum)
        -> void;

    /** Generate the markdown report from populated _Stats. */
    auto GenerateReport(const FCk_TraceSession& Session) const -> FString;

    /** Compute percentile from a sorted array. */
    static auto Percentile(const TArray<double>& SortedValues, double P) -> double;

    /**
     * Percentile over a sorted array conceptually preceded by InLeadingZeroCount zeros, without
     * materialising them.
     *
     * Per-timer samples are only kept for the frames a timer actually appeared in. Padding each
     * one out to the full frame count would cost (distinct timers x frames) doubles, which is
     * driven by how many RARE timers the trace contains — the opposite of the data that matters.
     * Identical arithmetic to Percentile on the padded array; the spec pins that equivalence.
     */
    static auto PercentileWithLeadingZeros(
        const TArray<double>& InSortedPresentValues,
        int32 InLeadingZeroCount,
        double InPercentile)
        -> double;

#if WITH_DEV_AUTOMATION_TESTS
    friend class FCkTest_MultiFrameReport_PercentileWithLeadingZerosMatchesPadded;
#endif

    FCk_MultiFrameReportConfig _Config;
    FCk_MultiFrameStats _Stats;
    FCk_TimerCategorizer _Categorizer;
};

// --------------------------------------------------------------------------------------------------------------------
