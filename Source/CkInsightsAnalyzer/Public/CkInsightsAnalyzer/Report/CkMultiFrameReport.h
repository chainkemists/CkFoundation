#pragma once

#include "CoreMinimal.h"
#include "CkInsightsAnalyzer/Core/CkFrameAnalyzer.h"
#include "CkInsightsAnalyzer/Core/CkTimerCategorizer.h"
#include "CkInsightsAnalyzer/Report/CkFrameReport.h"

// --------------------------------------------------------------------------------------------------------------------

class FCk_TraceSession;

// --------------------------------------------------------------------------------------------------------------------

/**
 * A contiguous run of game-frame indices, INCLUSIVE on both ends.
 *
 * A multi-frame selection is an array of these, ascending and non-overlapping; a plain contiguous
 * range is exactly one run. Inclusive rather than half-open because the runs are what the selection
 * UI and the report header both render, and an exclusive end reads as an off-by-one to a human.
 */
struct CKINSIGHTSANALYZER_API FCk_FrameRun
{
    uint64 FirstFrame = 0;
    uint64 LastFrame = 0;
};

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

    /**
     * Also aggregate per-thread wait/stall time across every analysed frame into
     * FCk_MultiFrameStats::WaitAverages.
     *
     * Off by default because it is the one panel that cannot be derived from the averaged frame:
     * FCk_FrameReport::ComputeWaitSummaries re-traverses every non-game thread over the frame's
     * REAL time window, which a synthetic averaged frame has no way to name. Turning this on
     * therefore pays a full per-thread timeline traversal per analysed frame — fine for a UI
     * selection of tens of frames, not for a 10k-frame whole-trace run.
     */
    bool ComputeWaitAverages = false;

    /**
     * Also build every analysed frame's own hot-path tree and merge them into
     * FCk_MultiFrameStats::MergedHotPaths.
     *
     * Off by default because it costs a full tree build per analysed frame — fine for a UI selection
     * of tens of frames, not for a 10k-frame whole-trace run. The tree cannot be derived from the
     * averaged frame instead: its parent-child edges are means over every analysed frame, so an edge
     * present in a minority of them falls under the tree's absolute thresholds and disappears.
     */
    bool BuildMergedHotPaths = false;

    /**
     * Forwarded into each per-frame hot-path tree build (FCk_FrameReportConfig::ShowAllChildren),
     * so the merged tree honors the same toggle the single-frame tree does. Orthogonal to Depth.
     */
    bool ShowAllChildren = false;

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

    /**
     * The runs that were selected for analysis, ascending and non-overlapping. A contiguous
     * analysis produces exactly one run.
     *
     * This is the SELECTION, not the outcome: frames that failed to analyse, and screenshot frames
     * dropped by ExcludeScreenshotFrames, are still inside a run here but are not in FrameCount.
     */
    TArray<FCk_FrameRun> SelectedRuns;

    /**
     * The frame indices that were actually analysed, ascending — screenshot-excluded frames and
     * frames that failed to analyse are absent, which is what separates this from SelectedRuns.
     *
     * This is the ORDINAL SPACE the per-frame series hang off: MergedHotPaths' PerFrameInclusiveMs
     * is indexed by position in this array. Always populated.
     */
    TArray<uint64> AnalysedFrameIndices;

    /**
     * The hot-path tree of every analysed frame, merged node by node — the multi-frame answer to
     * "which call paths cost what", carrying per-node presence so a path that dominates a handful of
     * frames is distinguishable from one that costs a little on all of them.
     *
     * Built PER FRAME and merged rather than read off AveragedFrame: an averaged parent-child edge is
     * divided by every analysed frame, so a path present in a minority of them lands under the tree's
     * absolute thresholds and vanishes, and the union of per-frame edges is not a tree in the first
     * place.
     *
     * Only populated when FCk_MultiFrameReportConfig::BuildMergedHotPaths is set.
     */
    TArray<TSharedPtr<FCk_MergedHotPathNode>> MergedHotPaths;

    /**
     * A synthetic frame whose every accumulable quantity is the arithmetic mean over the analysed
     * frames — "what a typical frame in this selection looks like", in the exact shape a
     * single-frame panel already consumes.
     *
     * Merge rules, per field:
     * - TimerInclusive / TimerExclusive — summed across frames, divided by FrameCount.
     *   A timer absent from a frame contributes zero, so these read as cost added to a typical
     *   frame and stay consistent with FCk_MultiFrameStats::TimerAverages.
     * - TimerCount — the same mean, ROUNDED to the uint32 the field already is. A timer that fires
     *   less than every other frame therefore rounds to 0; TimerAverages[].AvgCount carries the
     *   fractional truth and is the field to display when the distinction matters.
     * - FrameDurationMs — equals AvgFrameMs by construction.
     * - FrameRootTimerIndex / ThreadId — identity, not quantity: the DOMINANT value across the
     *   analysed frames (they are constant in practice; a tie takes the lowest index).
     * - FrameIndex — left 0. There is no frame this result describes; the selection is SelectedRuns.
     * - FrameStartTime / FrameEndTime / Events — SYNTHETIC. Events holds exactly one depth-0 event
     *   over [0, FrameDurationMs] under FrameRootTimerIndex, purely so IsValid() holds for consumers
     *   that gate on it. These are NOT session times, so IsSynthesizedAverage is set and the helpers
     *   that re-read the session over that window (ComputeWorkerThreadSummaries, ComputeWaitSummaries)
     *   reject it rather than reporting whatever happened at the start of the trace — use
     *   WaitAverages for the wait panel.
     * - ChildrenOf — NOT populated. A mean parent-child edge is diluted by every frame the edge is
     *   absent from, which put the whole hot-path tree under its own thresholds; the tree is built
     *   per frame and merged into MergedHotPaths instead, and FCk_FrameReport::BuildHotPathTree
     *   rejects a synthesized result.
     *
     * Valid inputs to FCk_FrameReport::ComputeCategorySummary and ComputeTopTimers, which read only
     * the per-timer maps above plus the session's timer-name table.
     */
    TOptional<FCk_FrameAnalysisResult> AveragedFrame;

    /**
     * Per-thread wait/stall time averaged over the analysed frames, game thread first.
     *
     * Only populated when FCk_MultiFrameReportConfig::ComputeWaitAverages is set — see the note
     * there for why this panel is aggregated here rather than read off AveragedFrame.
     */
    TArray<FCk_WaitThreadSummary> WaitAverages;

    /** True only when ComputeWaitAverages paid the per-thread scans and populated WaitAverages. */
    bool WaitAveragesComputed = false;

    /** GT-only accounting for every frame that survived analysis/exclusion, in AnalysedFrameIndices order. */
    TArray<FCk_FrameAccounting> FrameAccounting;

    /** Arithmetic means of numeric accounting fields only; start/end timestamps are deliberately absent. */
    TOptional<FCk_FrameAccounting> AverageAccounting;

    /** Timer-table threshold/cap reconciliation against all GT exclusive time, averaged per analysed frame (ms). */
    double TotalExclusiveMs = 0.0;
    double ReportedTimerExclusiveMs = 0.0;
    double OmittedTimerExclusiveMs = 0.0;

    /** Category-table threshold reconciliation against all GT exclusive time, averaged per analysed frame (ms). */
    double ReportedCategoryExclusiveMs = 0.0;
    double OmittedCategoryExclusiveMs = 0.0;

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

        /** Outer-scope union per timer; unlike AvgInclMs this avoids same-name recursive double count. */
        double AvgOuterInclMs = 0.0;
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
 *   // or, for a disjoint selection:
 *   FString Markdown = Report.AnalyzeFrameSet(Session, {{120, 140}, {200, 200}, {250, 260}});
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
     * Analyze the union of an arbitrary set of frame runs and generate a report.
     *
     * The runs must be non-empty, each with FirstFrame <= LastFrame, ascending, non-overlapping,
     * and within the trace's frame count — a violation ensures and rejects the WHOLE selection
     * rather than silently analysing the salvageable part of it.
     *
     * @param InSession  Open trace session
     * @param InRuns     Ascending, non-overlapping, inclusive frame runs
     * @return Slack markdown string
     */
    auto AnalyzeFrameSet(const FCk_TraceSession& InSession,
                         const TArray<FCk_FrameRun>& InRuns) -> FString;

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

    /**
     * Clamp a half-open [StartFrame, EndFrame) range against the trace and hand it to
     * DoAnalyzeFrameSet as a single run. Returns false if no frames.
     */
    auto DoAnalyzeFrameRange(const FCk_TraceSession& Session,
                             uint64 StartFrame, uint64 EndFrame) -> bool;

    /**
     * The one worker behind every entry point: analyses the frames the runs flatten to, in
     * ascending order, and populates _Stats. Returns false if nothing analysable remained.
     */
    auto DoAnalyzeFrameSet(const FCk_TraceSession& InSession,
                           const TArray<FCk_FrameRun>& InRuns) -> bool;

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

    /**
     * Whether a run selection is analysable against a trace of InTotalFrames frames: non-empty,
     * every run FirstFrame <= LastFrame, strictly ascending, non-overlapping, in bounds. Adjacent
     * runs (LastFrame + 1 == next FirstFrame) are legal — the caller chose not to merge them.
     *
     * Public like DoIs_ScreenshotFrame: a pure predicate the spec pins directly.
     */
    static auto DoIs_ValidRunSelection(
        const TArray<FCk_FrameRun>& InRuns,
        uint64 InTotalFrames)
        -> bool;

    /** Flatten runs to the ascending frame-index list the worker iterates. */
    static auto DoGet_FrameIndices(const TArray<FCk_FrameRun>& InRuns) -> TArray<uint64>;

    /** How many frames the runs select, without materialising the index list. */
    static auto DoGet_SelectedFrameCount(const TArray<FCk_FrameRun>& InRuns) -> uint64;

    /** Human-readable selection label, e.g. "120-140, 200, 250-260". Single-frame runs print bare. */
    static auto DoGet_FrameRunsLabel(const TArray<FCk_FrameRun>& InRuns) -> FString;

    /**
     * Merge per-frame hot-path trees into one tree carrying per-node presence and per-frame
     * magnitudes — see FCk_MergedHotPathNode for the statistics contract.
     *
     * InPerFrameTrees is ORDINAL-indexed: one entry per analysed frame, in the order of
     * FCk_MultiFrameStats::AnalysedFrameIndices, and an entry may be empty for a frame whose tree
     * was. The averaging denominator is therefore InPerFrameTrees.Num(), not the number of entries
     * that contributed nodes.
     *
     * Public like DoIs_ValidRunSelection: a pure reduction the spec pins directly.
     */
    static auto DoMerge_HotPathTrees(
        const TArray<TArray<TSharedPtr<FCk_HotPathNode>>>& InPerFrameTrees)
        -> TArray<TSharedPtr<FCk_MergedHotPathNode>>;

private:

    /** Reduce the per-frame timer accumulators into _Stats.TimerAverages. Sorts the exclusive samples in place. */
    auto DoBuild_TimerAverages(
        const TMap<uint32, FString>& InTimerNames,
        TMap<uint32, TArray<double>>& InTimerExclusivePerFrame,
        const TMap<uint32, double>& InTimerInclusiveSum,
        const TMap<uint32, double>& InTimerOuterInclusiveSum,
        const TMap<uint32, uint64>& InTimerCallSum)
        -> void;

    /** Per-frame accumulators for FCk_MultiFrameStats::AveragedFrame. Times stay in SECONDS. */
    struct FAveragedFrameAccumulator
    {
        TMap<uint32, double> InclusiveSum;
        TMap<uint32, double> ExclusiveSum;
        TMap<uint32, double> CountSum;
        TMap<uint32, int32> RootTimerVotes;
        TMap<uint32, int32> ThreadIdVotes;

        auto Accumulate(const FCk_FrameAnalysisResult& InResult) -> void;
    };

    /** Divide the accumulator by the analysed frame count into _Stats.AveragedFrame. */
    auto DoBuild_AveragedFrame(const FAveragedFrameAccumulator& InAccumulator) -> void;

    /**
     * Per-thread wait accumulation for FCk_MultiFrameStats::WaitAverages. Scopes are keyed by their
     * already-simplified display name, which is the only stable identity a wait scope has across
     * threads and frames.
     */
    struct FWaitAverageAccumulator
    {
        FString ThreadName;
        bool IsGameThread = false;
        double WaitMsSum = 0.0;
        double WallMsSum = 0.0;
        TMap<FString, double> ScopeExclusiveMsSum;
        TMap<FString, double> ScopeCountSum;
    };

    /** Divide the per-thread wait accumulation by the analysed frame count into _Stats.WaitAverages. */
    auto DoBuild_WaitAverages(const TMap<uint32, FWaitAverageAccumulator>& InPerThread) -> void;

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
