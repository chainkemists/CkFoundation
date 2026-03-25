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

    /** Minimum exclusive time (ms) for a timer to appear in the category summary. */
    double MinCategoryMs = 0.3;

    /** Minimum wall time (ms) for a worker thread to be shown. */
    double MinWorkerThreadMs = 5.0;

    /** Maximum number of worker threads to show. */
    int32 MaxWorkerThreads = 8;

    /** Whether to include the raw ranked timer list. */
    bool bShowRawTimerList = false;

    /** Number of raw timers to show (if enabled). */
    int32 RawTimerCount = 50;

    /** Whether to include worker thread analysis. */
    bool bShowWorkerThreads = true;

    /** Whether to include category summary. */
    bool bShowCategorySummary = true;

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
            bShowRawTimerList = true;
            RawTimerCount = 50;
            bShowWorkerThreads = true;
            bShowCategorySummary = true;
            break;
        case ECkReportDepth::Standard:
            MaxTreeDepth = 5;
            MaxRootTimers = 12;
            MinInclusiveMs = 1.0;
            MinCategoryMs = 0.3;
            MaxWorkerThreads = 8;
            MinWorkerThreadMs = 5.0;
            bShowRawTimerList = false;
            bShowWorkerThreads = true;
            bShowCategorySummary = true;
            break;
        case ECkReportDepth::Concise:
            MaxTreeDepth = 3;
            MaxRootTimers = 8;
            MinInclusiveMs = 2.0;
            MinCategoryMs = 0.5;
            bShowRawTimerList = false;
            bShowWorkerThreads = false;
            bShowCategorySummary = true;
            break;
        case ECkReportDepth::HotPathsOnly:
            MaxTreeDepth = 4;
            MaxRootTimers = 10;
            MinInclusiveMs = 1.0;
            bShowRawTimerList = false;
            bShowWorkerThreads = false;
            bShowCategorySummary = false;
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

    /** Get/set the report configuration. */
    auto GetConfig() const -> const FCk_FrameReportConfig& { return _Config; }
    auto SetConfig(const FCk_FrameReportConfig& Config) -> void { _Config = Config; }

private:

    // ---- Timer name resolution ----

    /** Resolve a timer index to its name via the session's timer reader. */
    using FTimerNameMap = TMap<uint32, FString>;

    /** Build a map of TimerIndex → Name from the session. */
    static auto BuildTimerNameMap(const FCk_TraceSession& Session) -> FTimerNameMap;

    /** Get timer name, with fallback. */
    static auto GetTimerName(const FTimerNameMap& Names, uint32 TimerIndex) -> FString;

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

private:
    FCk_FrameReportConfig _Config;
    FCk_TimerCategorizer _Categorizer;
};

// --------------------------------------------------------------------------------------------------------------------
