#pragma once

#include "CoreMinimal.h"

#include <TraceServices/AnalysisService.h>
#include <TraceServices/Model/AnalysisSession.h>
#include <TraceServices/Model/TimingProfiler.h>
#include <TraceServices/Model/Frames.h>
#include <TraceServices/Model/Threads.h>

// --------------------------------------------------------------------------------------------------------------------

/**
 * Wrapper around UE TraceServices that opens a .utrace file and provides
 * convenient access to timing, frame, and thread providers.
 *
 * Usage:
 *   FCk_TraceSession Session;
 *   if (Session.Open(TEXT("C:/captures/my.utrace")))
 *   {
 *       uint32 GameThreadId = Session.GetGameThreadId();
 *       uint64 FrameCount = Session.GetFrameCount();
 *       // ... enumerate frames, read timelines, etc.
 *   }
 *
 * All provider access methods require the session to be open and analysis complete.
 * Thread-safety: Create a FAnalysisSessionReadScope before accessing providers.
 */
class CKINSIGHTSANALYZER_API FCk_TraceSession
{
public:
    FCk_TraceSession() = default;
    ~FCk_TraceSession();

    // Non-copyable, movable
    FCk_TraceSession(const FCk_TraceSession&) = delete;
    FCk_TraceSession& operator=(const FCk_TraceSession&) = delete;
    FCk_TraceSession(FCk_TraceSession&& Other) noexcept;
    FCk_TraceSession& operator=(FCk_TraceSession&& Other) noexcept;

    // ---- Session Lifecycle ----

    /** Open a .utrace file. Blocks until analysis is complete. Returns true on success. */
    auto Open(const FString& FilePath) -> bool;

    /**
     * Prepare the analysis service (must be called on the game thread).
     * This handles FModuleManager loading which is not thread-safe.
     */
    auto PrepareAnalysisService() -> bool;

    /**
     * Run analysis on a prepared service. Can be called from any thread.
     * Must call PrepareAnalysisService() first.
     */
    auto AnalyzeFile(const FString& FilePath) -> bool;

    /** Close the session and release all resources. */
    auto Close() -> void;

    /** Whether a session is currently open and analysis is complete. */
    auto IsOpen() const -> bool;

    /** Get the path of the currently open trace file. */
    auto GetFilePath() const -> const FString& { return _FilePath; }

    /** Get the duration of the trace in seconds. */
    auto GetDurationSeconds() const -> double;

    // ---- Provider Access ----
    // These require IsOpen() == true and should be called within a read scope.

    /** Get a read scope for thread-safe provider access. */
    auto CreateReadScope() const -> TraceServices::FAnalysisSessionReadScope;

    /** Get the raw analysis session. */
    auto GetSession() const -> const TraceServices::IAnalysisSession*;

    /** Get the timing profiler provider (timers, events, timelines). May return nullptr. */
    auto GetTimingProvider() const -> const TraceServices::ITimingProfilerProvider*;

    /** Get the frame provider (frame boundaries and indices). */
    auto GetFrameProvider() const -> const TraceServices::IFrameProvider*;

    /** Get the thread provider (thread names and IDs). */
    auto GetThreadProvider() const -> const TraceServices::IThreadProvider*;

    // ---- Convenience Accessors ----

    /** Auto-detect the game thread ID (by name "GameThread", fallback to most-events heuristic). */
    auto GetGameThreadId() const -> uint32;

    /** Get timeline index for a given thread ID. Returns INDEX_NONE on failure. */
    auto GetTimelineIndex(uint32 ThreadId) const -> uint32;

    /** Get total number of game frames in the trace. */
    auto GetFrameCount() const -> uint64;

    /** Get a specific game frame by index. Returns nullptr if index is invalid. */
    auto GetFrame(uint64 Index) const -> const TraceServices::FFrame*;

    /** Read all timer metadata. Callback receives the timer reader. */
    auto ReadTimers(TFunctionRef<void(const TraceServices::ITimingProfilerTimerReader&)> Callback) const -> void;

    /** Read a timeline by index. Callback receives the timeline. Returns false if index is invalid. */
    auto ReadTimeline(uint32 TimelineIndex,
                      TFunctionRef<void(const TraceServices::ITimingProfilerProvider::Timeline&)> Callback) const -> bool;

    /** Get all thread infos as an array of (ThreadId, Name, GroupName). */
    auto GetThreadInfos() const -> TArray<TraceServices::FThreadInfo>;

private:
    FString _FilePath;
    TSharedPtr<TraceServices::IAnalysisService> _AnalysisService;
    TSharedPtr<const TraceServices::IAnalysisSession> _Session;

    // Cached after Open()
    mutable uint32 _CachedGameThreadId = static_cast<uint32>(INDEX_NONE);
    mutable bool   _bGameThreadIdCached = false;
};

// --------------------------------------------------------------------------------------------------------------------
