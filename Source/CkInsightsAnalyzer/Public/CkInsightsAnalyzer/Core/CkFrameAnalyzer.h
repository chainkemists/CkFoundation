#pragma once

#include "CoreMinimal.h"

#include <TraceServices/Model/TimingProfiler.h>
#include <TraceServices/Model/Frames.h>

// --------------------------------------------------------------------------------------------------------------------

class FCk_TraceSession;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Raw timing event extracted from a timeline.
 * Mirrors what Python's load_timing_events() produces per row.
 */
struct FCk_TimingEvent
{
    uint32 TimerIndex = 0;      // Index into timer array (maps to FTimingProfilerTimer)
    double StartTime = 0.0;     // Seconds
    double EndTime = 0.0;       // Seconds
    uint32 Depth = 0;
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Results of analyzing a single frame (or time range).
 *
 * Contains per-timer inclusive/exclusive times, call counts, and the parent→child
 * inclusive time graph — everything needed to build hot path trees and category summaries.
 */
struct CKINSIGHTSANALYZER_API FCk_FrameAnalysisResult
{
    // The frame that was analyzed (if from a specific frame)
    uint64 FrameIndex = 0;
    double FrameStartTime = 0.0;
    double FrameEndTime = 0.0;

    /** Total frame duration in milliseconds. */
    double FrameDurationMs = 0.0;

    /** Thread ID that was analyzed (typically game thread). */
    uint32 ThreadId = 0;

    /** Per-timer inclusive time in seconds. TimerIndex → total inclusive seconds. */
    TMap<uint32, double> TimerInclusive;

    /** Per-timer exclusive time in seconds. TimerIndex → total exclusive seconds. */
    TMap<uint32, double> TimerExclusive;

    /** Per-timer call count. TimerIndex → count. */
    TMap<uint32, uint32> TimerCount;

    /**
     * Parent→child inclusive time graph.
     * ChildrenOf[ParentTimerIndex][ChildTimerIndex] = total inclusive seconds of child under parent.
     * Used to build hierarchical hot path trees.
     */
    TMap<uint32, TMap<uint32, double>> ChildrenOf;

    /** The raw events for this frame (sorted by start time, depth). */
    TArray<FCk_TimingEvent> Events;

    /** Timer index of the frame root (depth-0 event). */
    uint32 FrameRootTimerIndex = static_cast<uint32>(INDEX_NONE);

    /** Check if the result has data. */
    auto IsValid() const -> bool { return Events.Num() > 0; }

    // ---- Convenience accessors (times in milliseconds) ----

    /** Get inclusive time for a timer in ms. Returns 0 if not found. */
    auto GetInclusiveMs(uint32 TimerIndex) const -> double
    {
        const double* Val = TimerInclusive.Find(TimerIndex);
        return Val ? (*Val) * 1000.0 : 0.0;
    }

    /** Get exclusive time for a timer in ms. Returns 0 if not found. */
    auto GetExclusiveMs(uint32 TimerIndex) const -> double
    {
        const double* Val = TimerExclusive.Find(TimerIndex);
        return Val ? (*Val) * 1000.0 : 0.0;
    }

    /** Get call count for a timer. Returns 0 if not found. */
    auto GetCount(uint32 TimerIndex) const -> uint32
    {
        const uint32* Val = TimerCount.Find(TimerIndex);
        return Val ? *Val : 0;
    }

    /** Get children of a timer with their inclusive time in ms. Sorted by inclusive time descending. */
    auto GetChildren(uint32 ParentTimerIndex, double MinInclusiveMs = 0.5) const
        -> TArray<TPair<uint32, double>>;
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Per-frame analysis engine.
 *
 * Extracts timing events from a trace session for a specific frame and computes:
 * - Inclusive time per timer (wall time including children)
 * - Exclusive time per timer (self time, children subtracted)
 * - Call counts
 * - Parent→child inclusive time graph
 *
 * The exclusive time algorithm is a stack-based O(n) computation matching
 * the Python analyze_frame.py implementation.
 */
class CKINSIGHTSANALYZER_API FCk_FrameAnalyzer
{
public:
    /**
     * Analyze a single frame on the game thread.
     *
     * @param Session   Open trace session
     * @param FrameIndex  Game frame index to analyze
     * @return Analysis result with timing data
     */
    static auto AnalyzeFrame(const FCk_TraceSession& Session, uint64 FrameIndex)
        -> FCk_FrameAnalysisResult;

    /**
     * Analyze a time range on a specific thread.
     *
     * @param Session       Open trace session
     * @param ThreadId      Thread to analyze
     * @param StartTime     Start time in seconds
     * @param EndTime       End time in seconds
     * @param FrameIndex    Optional frame index for labeling (default 0)
     * @return Analysis result with timing data
     */
    static auto AnalyzeTimeRange(const FCk_TraceSession& Session,
                                 uint32 ThreadId,
                                 double StartTime, double EndTime,
                                 uint64 FrameIndex = 0)
        -> FCk_FrameAnalysisResult;

    /**
     * Analyze a specific thread for all events in its timeline.
     * Useful for worker thread analysis.
     *
     * @param Session   Open trace session
     * @param ThreadId  Thread to analyze
     * @return Analysis result with timing data
     */
    static auto AnalyzeThread(const FCk_TraceSession& Session, uint32 ThreadId)
        -> FCk_FrameAnalysisResult;

    /**
     * Compute exclusive times from a set of pre-extracted events.
     * This is the core algorithm — stack-based O(n) exclusive time calculation.
     *
     * Populates the result's TimerInclusive, TimerExclusive, TimerCount, and ChildrenOf.
     *
     * @param Events    Sorted timing events
     * @param Result    Output result to populate
     */
    static auto ComputeExclusiveTimes(const TArray<FCk_TimingEvent>& Events,
                                      FCk_FrameAnalysisResult& Result) -> void;

private:
    /** Extract events from a timeline within a time range. */
    static auto ExtractEvents(const FCk_TraceSession& Session,
                              uint32 TimelineIndex,
                              double StartTime, double EndTime)
        -> TArray<FCk_TimingEvent>;
};

// --------------------------------------------------------------------------------------------------------------------
