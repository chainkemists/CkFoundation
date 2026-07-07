#pragma once

#include "CoreMinimal.h"
#include "CkInsightsAnalyzer/Core/CkFrameAnalyzer.h"
#include "CkInsightsAnalyzer/Report/CkFrameReport.h"
#include "CkInsightsAnalyzer/Report/CkMultiFrameReport.h"

// --------------------------------------------------------------------------------------------------------------------

class FCk_TraceSession;
class FJsonObject;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Machine-readable JSON reports from trace analysis results.
 *
 * Mirrors the data behind the markdown reports (FCk_FrameReport / FCk_MultiFrameReport) but emits
 * structured output for scripts, CI gates, and AI consumption:
 * - Times in milliseconds, camelCase keys, empty sections omitted.
 * - The call tree is the TRUE aggregated call tree built from raw timing events (events merged by
 *   call path) — no wrapper collapsing and no display-name simplification. The one exception is
 *   workerThreads[].topTimers, which reuses the shared summary computation and therefore carries
 *   simplified names, matching the markdown report.
 *
 * Usage:
 *   FString Json = FCk_JsonReport::GenerateSingleFrame(Session, Result, Config);
 *   FString Json = FCk_JsonReport::GenerateMultiFrame(Session, Stats, Config);
 */
class CKINSIGHTSANALYZER_API FCk_JsonReport
{
public:
    /**
     * Generate a JSON report for a single analyzed frame.
     *
     * Contains: trace overview, budget, frame duration, aggregated call tree
     * (pruned below Config.MinInclusiveMs), top timers by exclusive time
     * (Config.RawTimerCount), category breakdown (Config.MinCategoryMs), and
     * worker-thread summaries (Config.MinWorkerThreadMs / MaxWorkerThreads).
     */
    static auto GenerateSingleFrame(const FCk_TraceSession& Session,
                                    const FCk_FrameAnalysisResult& Result,
                                    const FCk_FrameReportConfig& Config) -> FString;

    /**
     * Generate a JSON report for a multi-frame analysis.
     *
     * Contains: trace overview, budget, aggregate frame-time statistics
     * (avg/min/max/p95/p99), worst frames with dominant cost, and category averages.
     * Stats come from FCk_MultiFrameReport::GetStats() after an Analyze* call.
     */
    static auto GenerateMultiFrame(const FCk_TraceSession& Session,
                                   const FCk_MultiFrameStats& Stats,
                                   const FCk_MultiFrameReportConfig& Config) -> FString;

private:
    /** Aggregated call-tree node: events merged by call path (same timer under same parent). */
    struct FCallTreeNode
    {
        uint32 TimerIndex = 0;
        double InclusiveSeconds = 0.0;
        uint32 Count = 0;
        TMap<uint32, int32> ChildByTimer; // TimerIndex → pool index, for path aggregation
        TArray<int32> Children;           // pool indices
    };

    /**
     * Build the aggregated call tree from raw timing events (sorted by start time, depth).
     * Nodes are pooled in OutPool; returns the root pool indices.
     */
    static auto BuildCallTree(const TArray<FCk_TimingEvent>& Events,
                              TArray<FCallTreeNode>& OutPool) -> TArray<int32>;

    /**
     * Serialize a call-tree node recursively. Children below MinInclusiveMs are pruned,
     * but their time still counts toward the parent's inclusive/exclusive totals.
     */
    static auto SerializeCallTreeNode(int32 NodeIndex,
                                      const TArray<FCallTreeNode>& Pool,
                                      const FCk_FrameReport::FTimerNameMap& TimerNames,
                                      double MinInclusiveMs) -> TSharedPtr<FJsonObject>;

    /** Trace overview: file, duration, game/render frame counts, thread list. */
    static auto MakeTraceOverview(const FCk_TraceSession& Session) -> TSharedPtr<FJsonObject>;

    /** Pretty-print a JSON object to string. */
    static auto ToJsonString(const TSharedRef<FJsonObject>& Root) -> FString;

    /** Round to 3 decimals so reports are stable and compact. */
    static auto Round3(double Value) -> double;
};

// --------------------------------------------------------------------------------------------------------------------
