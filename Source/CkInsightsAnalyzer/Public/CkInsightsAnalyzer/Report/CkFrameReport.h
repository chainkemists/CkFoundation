#pragma once

#include "CoreMinimal.h"
#include "CkInsightsAnalyzer/Core/CkFrameAnalyzer.h"
#include "CkInsightsAnalyzer/Core/CkTimerCategorizer.h"

// --------------------------------------------------------------------------------------------------------------------

class FCk_TraceSession;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Controls the level of detail in generated reports.
 */
enum class ECkReportDepth : uint8
{
    /** Everything: deep hot path tree, categories, worker threads, raw timer list. */
    Full,

    /** Hot paths, category summary, worker threads. */
    Standard,

    /** Hot paths and category summary only. */
    Concise,

    /** Hot path tree only — no summaries or extras. */
    HotPathsOnly,
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Configuration for report generation.
 */
struct CKINSIGHTSANALYZER_API FCk_FrameReportConfig
{
    /** Report detail level. Call ApplyDepth() to propagate to individual fields. */
    ECkReportDepth Depth = ECkReportDepth::Standard;

    /** Target frame budget in ms (default 60fps = 16.67ms). */
    double TargetFrameMs = 16.67;

    /** Maximum tree depth for hot path display. */
    int32 MaxTreeDepth = 5;

    /** Maximum number of root-level timers to show. */
    int32 MaxRootTimers = 12;

    /** Minimum inclusive time (ms) for a timer to appear in the tree. */
    double MinInclusiveMs = 1.0;

    /** Floor (ms) below which a child folds into the "(+N below threshold)" aggregate row. */
    double MinChildMs = 0.3;

    /** Fraction of the parent's inclusive time below which a child folds into the aggregate row. */
    double MinChildPctOfParent = 0.03;

    /** Maximum visible children per node; the remainder folds into the aggregate row. */
    int32 MaxVisibleChildren = 8;

    /**
     * Bypass the child threshold and cap entirely — every child renders and no aggregate rows are
     * emitted for threshold-folding (dedup-dropped mass can still produce one). Orthogonal to
     * Depth (ApplyDepth never touches it); MaxTreeDepth still bounds recursion.
     */
    bool ShowAllChildren = false;

    /** Minimum exclusive time (ms) for a timer to appear in the category summary. */
    double MinCategoryMs = 0.3;

    /** Minimum wall time (ms) for a worker thread to be shown. */
    double MinWorkerThreadMs = 5.0;

    /** Maximum number of worker threads to show. */
    int32 MaxWorkerThreads = 8;

    /** Whether to include the raw ranked timer list. */
    bool ShowRawTimerList = false;

    /** Number of raw timers to show (if enabled). */
    int32 RawTimerCount = 50;

    /** Whether to include worker thread analysis. */
    bool ShowWorkerThreads = true;

    /** Whether to include category summary. */
    bool ShowCategorySummary = true;

    /** Whether to include the per-thread wait/stall breakdown. */
    bool ShowWaitBreakdown = true;

    /** Minimum aggregated wait time (ms) for a thread to appear in the wait breakdown. */
    double MinWaitMs = 1.0;

    /** Set all individual fields from Depth. */
    auto ApplyDepth() -> void
    {
        switch (Depth)
        {
        case ECkReportDepth::Full:
            MaxTreeDepth = 8;
            MaxRootTimers = 15;
            MinInclusiveMs = 0.5;
            MinCategoryMs = 0.2;
            MaxWorkerThreads = 12;
            MinWorkerThreadMs = 2.0;
            ShowRawTimerList = true;
            RawTimerCount = 50;
            ShowWorkerThreads = true;
            ShowCategorySummary = true;
            ShowWaitBreakdown = true;
            break;
        case ECkReportDepth::Standard:
            MaxTreeDepth = 5;
            MaxRootTimers = 12;
            MinInclusiveMs = 1.0;
            MinCategoryMs = 0.3;
            MaxWorkerThreads = 8;
            MinWorkerThreadMs = 5.0;
            ShowRawTimerList = false;
            ShowWorkerThreads = true;
            ShowCategorySummary = true;
            ShowWaitBreakdown = true;
            break;
        case ECkReportDepth::Concise:
            MaxTreeDepth = 3;
            MaxRootTimers = 8;
            MinInclusiveMs = 2.0;
            MinCategoryMs = 0.5;
            ShowRawTimerList = false;
            ShowWorkerThreads = false;
            ShowCategorySummary = true;
            ShowWaitBreakdown = false;
            break;
        case ECkReportDepth::HotPathsOnly:
            MaxTreeDepth = 4;
            MaxRootTimers = 10;
            MinInclusiveMs = 1.0;
            ShowRawTimerList = false;
            ShowWorkerThreads = false;
            ShowCategorySummary = false;
            ShowWaitBreakdown = false;
            break;
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Worker thread analysis summary for a single thread.
 */
struct FCk_WorkerThreadSummary
{
    uint32 ThreadId = 0;
    FString ThreadName;
    FString ThreadGroup;
    double WallTimeMs = 0.0;
    int32 EventCount = 0;

    /** Top exclusive-time timers: (TimerName, ExclusiveMs, Count). */
    struct FTopTimer
    {
        FString Name;
        double ExclusiveMs = 0.0;
        uint32 Count = 0;
    };
    TArray<FTopTimer> TopTimers;
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * One node of the structured hot-path tree — same wrapper unwrapping/collapsing
 * and per-root dedup logic as the markdown report, exposed as data so Slate
 * views (SCkInsightsAnalyzerTab) can render a real tree instead of text.
 */
struct CKINSIGHTSANALYZER_API FCk_HotPathNode
{
    /** Raw timer name (post wrapper-collapse). */
    FString RawName;

    /** Simplified display name (FCk_TimerCategorizer::SimplifyName). */
    FString DisplayName;

    /** Collapsed wrapper chain leading into this timer (raw names). */
    TArray<FString> Breadcrumbs;

    double InclusiveMs = 0.0;
    double ExclusiveMs = 0.0;
    uint32 Count = 0;

    /**
     * True for the synthetic "(+N below threshold)" reconciliation row appended after the visible
     * children — it carries the inclusive mass of children dropped by the 3%/0.3ms threshold, the
     * child cap, or sibling-subtree dedup, so self + children + this row ≈ the parent's inclusive.
     */
    bool bIsAggregate = false;

    TArray<TSharedPtr<FCk_HotPathNode>> Children;
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * One node of a hot-path tree merged across several analysed frames
 * (FCk_MultiFrameReport::DoMerge_HotPathTrees).
 *
 * Identity is (RawName, Breadcrumbs) WITHIN a parent: the same timer legitimately appears under
 * different collapsed wrapper chains, and merging those into one row attributes cost to a call path
 * that never ran.
 *
 * Statistics contract:
 * - AvgInclusiveMs / AvgExclusiveMs / AvgCount are means over ALL analysed frames — a frame the node
 *   is absent from contributes zero, matching FCk_MultiFrameStats::TimerAverages, so the rows read as
 *   "cost added to a typical frame" and sum toward the frame average.
 * - FramesPresent counts the analysed frames whose OWN hot-path tree contained this node. A node
 *   below that frame's thresholds is absent, so presence means "made that frame's tree", not
 *   "the timer fired".
 * - HitAvgInclusiveMs is the mean over FramesPresent alone — what the node costs on a frame it shows
 *   up in, which for a spiky node is the number that explains the spike.
 * - P95InclusiveMs / MaxInclusiveMs are over the PRESENT samples only.
 * - PerFrameInclusiveMs is indexed by analysed-frame ORDINAL, parallel to
 *   FCk_MultiFrameStats::AnalysedFrameIndices, and holds a negative value for a frame the node was
 *   absent from — zero would be indistinguishable from a node that ran for no measurable time.
 * - Children are sorted by AvgInclusiveMs descending.
 */
struct CKINSIGHTSANALYZER_API FCk_MergedHotPathNode
{
    FString RawName;
    FString DisplayName;
    TArray<FString> Breadcrumbs;

    double AvgInclusiveMs = 0.0;
    double AvgExclusiveMs = 0.0;
    double AvgCount = 0.0;
    uint64 FramesPresent = 0;
    double HitAvgInclusiveMs = 0.0;
    double P95InclusiveMs = 0.0;
    double MaxInclusiveMs = 0.0;

    /** True for the synthetic "(+N below threshold)" row — merged by the same identity as any other. */
    bool bIsAggregate = false;

    TArray<float> PerFrameInclusiveMs;

    TArray<TSharedPtr<FCk_MergedHotPathNode>> Children;
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Per-thread wait/stall summary for a single frame window — how much of the thread's
 * time went to task waits, idle time, and sync completions (FCk_TimerCategorizer::IsWaitTimer).
 */
struct CKINSIGHTSANALYZER_API FCk_WaitThreadSummary
{
    uint32 ThreadId = 0;
    FString ThreadName;
    bool bIsGameThread = false;

    /** Aggregated exclusive time (ms) of wait scopes on this thread within the frame window. */
    double WaitMs = 0.0;

    /** Thread wall time (ms) within the frame window (frame duration for the game thread). */
    double WallMs = 0.0;

    /** Top wait scopes by exclusive time: (SimplifiedName, ExclusiveMs, Count). */
    struct FWaitScope
    {
        FString Name;
        double ExclusiveMs = 0.0;
        uint32 Count = 0;
    };
    TArray<FWaitScope> TopWaits;
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Reconciles one real game-frame window without claiming that all elapsed wall time was CPU busy.
 * NamedWaitMs is only the explicit-name heuristic in FCk_TimerCategorizer::IsWaitTimer.
 */
struct CKINSIGHTSANALYZER_API FCk_FrameAccounting
{
    uint64 FrameIndex = 0;
    uint32 ThreadId = 0;
    double StartTime = 0.0;
    double EndTime = 0.0;
    double FrameMs = 0.0;
    double InstrumentedMs = 0.0;
    double NamedWaitMs = 0.0;
    double OtherInstrumentedMs = 0.0;
    double UninstrumentedMs = 0.0;
    double ExclusiveSumMs = 0.0;
    /** ExclusiveSumMs - InstrumentedMs; a diagnostic for malformed/overlapping scope accounting. */
    double ExclusiveCoverageErrorMs = 0.0;
};

// --------------------------------------------------------------------------------------------------------------------

/** Per-category exclusive-time entry for structured display. */
struct CKINSIGHTSANALYZER_API FCk_CategorySummaryEntry
{
    FString Name;
    double ExclusiveMs = 0.0;
    double PctOfFrame = 0.0;
};

// --------------------------------------------------------------------------------------------------------------------

/** Ranked timer entry (by exclusive time) for structured display. */
struct CKINSIGHTSANALYZER_API FCk_TopTimerEntry
{
    /** Simplified display name. */
    FString Name;

    double ExclusiveMs = 0.0;
    double InclusiveMs = 0.0;
    uint32 Count = 0;

    /** Exclusive time as a percentage of the frame duration. */
    double PctOfFrame = 0.0;
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Generates Slack-friendly markdown reports from frame analysis results.
 *
 * Ports the full report generation pipeline from analyze_frame.py:
 * - Header with frame time and budget comparison
 * - Hierarchical hot path tree with wrapper collapsing
 * - Category summary (exclusive time breakdown)
 * - Worker thread summary
 * - Optional raw timer list
 *
 * Usage:
 *   FCk_FrameReport Report;
 *   FString Markdown = Report.Generate(Session, AnalysisResult);
 */
class CKINSIGHTSANALYZER_API FCk_FrameReport
{
public:
    FCk_FrameReport();
    explicit FCk_FrameReport(const FCk_FrameReportConfig& Config);

    /**
     * Generate a complete Slack-friendly markdown report for a single frame.
     *
     * @param Session   The trace session (for timer names and worker thread analysis)
     * @param Result    Analysis result from FCk_FrameAnalyzer
     * @return Slack markdown string
     */
    auto Generate(const FCk_TraceSession& Session,
                  const FCk_FrameAnalysisResult& Result) const -> FString;

    /**
     * One-line trace overview (file, duration, game/render frame counts, thread count).
     * Shared by the single-frame and multi-frame markdown reports.
     */
    static auto GenerateTraceOverview(const FCk_TraceSession& Session,
                                      TArray<FString>& Lines) -> void;

    /**
     * Analyze all non-game threads within the frame's time window and summarize them.
     * Returns summaries sorted by wall time descending, filtered to >= MinWorkerThreadMs.
     * Shared by the markdown and JSON renderers.
     */
    static auto ComputeWorkerThreadSummaries(const FCk_TraceSession& Session,
                                             const FCk_FrameAnalysisResult& GameThreadResult,
                                             double MinWorkerThreadMs)
        -> TArray<FCk_WorkerThreadSummary>;

    /**
     * Aggregate wait/stall time per thread (game thread first, then workers) within the
     * frame's time window. Sums EXCLUSIVE time of wait scopes so nested waits never double
     * count. Returns threads with >= MinWaitMs of wait, sorted by wait time descending
     * (game thread pinned first). Shared by the markdown, JSON, and Slate renderers.
     */
    static auto ComputeWaitSummaries(const FCk_TraceSession& Session,
                                     const FCk_FrameAnalysisResult& GameThreadResult,
                                     double MinWaitMs)
        -> TArray<FCk_WaitThreadSummary>;

    /** Get/set the report configuration. */
    auto GetConfig() const -> const FCk_FrameReportConfig& { return _Config; }
    auto SetConfig(const FCk_FrameReportConfig& Config) -> void { _Config = Config; }

    // ---- Timer name resolution ----

    /** Resolve a timer index to its name via the session's timer reader. */
    using FTimerNameMap = TMap<uint32, FString>;

    /** Build a map of TimerIndex → Name from the session. Caller must hold a read scope. */
    static auto BuildTimerNameMap(const FCk_TraceSession& Session) -> FTimerNameMap;

    /** Get timer name, with fallback. */
    static auto GetTimerName(const FTimerNameMap& Names, uint32 TimerIndex) -> FString;

    /** Compute GT-only accounting from one real result and a caller-owned timer-name map. */
    static auto ComputeFrameAccounting(const FCk_FrameAnalysisResult& Result,
                                       const FTimerNameMap& TimerNames) -> FCk_FrameAccounting;

    /**
     * ComputeWaitSummaries for callers that already hold the timer-name map — a per-frame loop that
     * rebuilt it on every call would pay #timers work per analysed frame for nothing.
     */
    static auto ComputeWaitSummaries(const FCk_TraceSession& Session,
                                     const FCk_FrameAnalysisResult& GameThreadResult,
                                     double MinWaitMs,
                                     const FTimerNameMap& TimerNames)
        -> TArray<FCk_WaitThreadSummary>;

    // ---- Structured data (for Slate views) ----
    // NOTE: declared after FTimerNameMap — member alias must precede its use in
    // parameter types.

    /**
     * Build the structured hot-path tree: roots sorted by inclusive time
     * (capped at MaxRootTimers), same wrapper collapsing and per-root dedup
     * as the markdown report. Acquires its own read scope.
     */
    auto BuildHotPathTree(const FCk_TraceSession& Session,
                          const FCk_FrameAnalysisResult& Result) const
        -> TArray<TSharedPtr<FCk_HotPathNode>>;

    /**
     * BuildHotPathTree for callers that already hold a read scope and the timer-name map — a
     * per-frame loop that rebuilt the map on every call would pay #timers work per analysed frame
     * for nothing.
     */
    auto BuildHotPathTree(const FCk_TraceSession& Session,
                          const FCk_FrameAnalysisResult& Result,
                          const FTimerNameMap& TimerNames) const
        -> TArray<TSharedPtr<FCk_HotPathNode>>;

    /**
     * Per-category exclusive time, sorted descending, filtered by MinCategoryMs.
     * Same data the markdown category summary renders.
     */
    auto ComputeCategorySummary(const FCk_FrameAnalysisResult& Result,
                                const FTimerNameMap& TimerNames) const
        -> TArray<FCk_CategorySummaryEntry>;

    /** Top MaxCount timers by exclusive time, with simplified names. */
    auto ComputeTopTimers(const FCk_FrameAnalysisResult& Result,
                          const FTimerNameMap& TimerNames,
                          int32 MaxCount) const
        -> TArray<FCk_TopTimerEntry>;

private:

    // ---- Report sections ----

    auto GenerateHeader(const FCk_FrameAnalysisResult& Result,
                        TArray<FString>& Lines) const -> void;

    auto GenerateHotPaths(const FCk_FrameAnalysisResult& Result,
                          const FTimerNameMap& TimerNames,
                          TArray<FString>& Lines) const -> void;

    auto GenerateCategorySummary(const FCk_FrameAnalysisResult& Result,
                                const FTimerNameMap& TimerNames,
                                TArray<FString>& Lines) const -> void;

    auto GenerateWorkerThreads(const FCk_TraceSession& Session,
                               const FCk_FrameAnalysisResult& GameThreadResult,
                               TArray<FString>& Lines) const -> void;

    auto GenerateWaitBreakdown(const FCk_TraceSession& Session,
                               const FCk_FrameAnalysisResult& GameThreadResult,
                               TArray<FString>& Lines) const -> void;

    auto GenerateRawTimerList(const FCk_FrameAnalysisResult& Result,
                              const FTimerNameMap& TimerNames,
                              TArray<FString>& Lines) const -> void;

    // ---- Hot path tree internals ----

    /** Frame wrapper names to unwrap when finding root timers. */
    static auto IsFrameWrapper(const FString& TimerName) -> bool;

    /**
     * Recursively unwrap frame wrappers to find real root timers.
     * Returns map of TimerIndex → inclusive seconds.
     */
    auto UnwrapRoots(uint32 ParentTimerIndex,
                     const FCk_FrameAnalysisResult& Result,
                     const FTimerNameMap& TimerNames,
                     int32 Depth = 0) const -> TMap<uint32, double>;

    /** Intermediate data for wrapper collapsing. */
    struct FCollapsedTimer
    {
        uint32 TimerIndex = 0;
        double InclusiveMs = 0.0;
        double ExclusiveMs = 0.0;
        uint32 Count = 0;
        TArray<FString> Breadcrumbs; // collapsed wrapper path
    };

    /**
     * Collapse thin wrappers (low exclusive time, single dominant child).
     * Chains through multiple levels. Mirrors Python collapse_wrappers().
     */
    auto CollapseWrappers(uint32 TimerIndex, double InclMs, double ExclMs, uint32 Count,
                          const FCk_FrameAnalysisResult& Result,
                          const FTimerNameMap& TimerNames) const -> FCollapsedTimer;

    /** Get significant children of a timer above a threshold. */
    struct FChildInfo
    {
        uint32 TimerIndex;
        double InclusiveMs;
        double ExclusiveMs;
        uint32 Count;
    };

    static auto GetSignificantChildren(uint32 ParentTimerIndex,
                                       const FCk_FrameAnalysisResult& Result,
                                       double MinInclusiveMs = 0.5)
        -> TArray<FChildInfo>;

    /**
     * Recursively build tree lines for a timer and its children.
     * Handles wrapper collapsing, deduplication, and tree-drawing characters.
     */
    auto BuildTreeLines(uint32 TimerIndex, int32 Depth,
                        double InclMs, double ExclMs, uint32 Count,
                        const FCk_FrameAnalysisResult& Result,
                        const FTimerNameMap& TimerNames,
                        TSet<uint32>& ShownTimers,
                        TMap<int32, bool>& IsLastAtDepth,
                        const TArray<FString>* PreBreadcrumbs = nullptr) const
        -> TArray<FString>;

    /** Build tree-drawing prefix (e.g., "│  ├─ "). */
    static auto MakeTreePrefix(int32 Depth, const TMap<int32, bool>& IsLastAtDepth) -> FString;

    /** Node-producing analog of BuildTreeLines (shared collapse/dedup rules). */
    auto DoBuildTreeNode(uint32 TimerIndex, int32 Depth,
                         double InclMs, double ExclMs, uint32 Count,
                         const FCk_FrameAnalysisResult& Result,
                         const FTimerNameMap& TimerNames,
                         TSet<uint32>& ShownTimers,
                         const TArray<FString>* PreBreadcrumbs = nullptr) const
        -> TSharedPtr<FCk_HotPathNode>;

private:
    FCk_FrameReportConfig _Config;
    FCk_TimerCategorizer _Categorizer;
};

// --------------------------------------------------------------------------------------------------------------------
