#pragma once

#include "CoreMinimal.h"

#include <TraceServices/AnalysisService.h>
#include <TraceServices/Model/AnalysisSession.h>
#include <TraceServices/Model/TimingProfiler.h>
#include <TraceServices/Model/Frames.h>
#include <TraceServices/Model/Threads.h>

// --------------------------------------------------------------------------------------------------------------------

/**
 * Metadata for a screenshot embedded in an Insights trace.
 *
 * Payload bytes are deliberately excluded. Use FCk_TraceSession::TryCopyScreenshotData
 * only when a caller needs to decode or persist a particular screenshot.
 */
struct FCk_TraceScreenshot
{
    /** TraceServices screenshot identifier, used with TryCopyScreenshotData. */
    uint32 Id = static_cast<uint32>(INDEX_NONE);

    FString Name;
    double  TimestampSeconds = 0.0;
    uint32  Width = 0;
    uint32  Height = 0;

    /** Bytes declared by the screenshot header and bytes currently available in the trace. */
    uint32 ExpectedPayloadByteSize = 0;
    uint32 PayloadByteSize = 0;
    bool   bIsPayloadComplete = false;

    /** The frame selected by TraceServices' preceding-frame lookup, or INDEX_NONE when no frame precedes the timestamp. */
    int64 GameFrameIndex = INDEX_NONE;
    int64 RenderFrameIndex = INDEX_NONE;

    /** True only when TimestampSeconds is inside the selected frame's actual [StartTime, EndTime] interval. */
    bool bIsInsideGameFrame = false;
    bool bIsInsideRenderFrame = false;
};

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
     * Run analysis on a prepared service, blocking until complete. Must call
     * PrepareAnalysisService() first. Game-thread callers only (registered trace
     * modules, e.g. ChaosVD, assume OnAnalysisBegin runs on the game thread) —
     * for non-blocking UI use, prefer StartAnalysis() + IsAnalysisComplete().
     */
    auto AnalyzeFile(const FString& FilePath) -> bool;

    /**
     * Begin analysis WITHOUT blocking. MUST be called on the game thread —
     * registered trace modules (e.g. ChaosVD) ensure(IsInGameThread()) in their
     * OnAnalysisBegin, which fires synchronously here. The heavy processing
     * continues on TraceServices' own analysis thread; poll IsAnalysisComplete().
     * Providers are readable (under a read scope) while analysis is running.
     */
    auto StartAnalysis(const FString& FilePath) -> bool;

    /** Whether analysis started via StartAnalysis() has finished. */
    auto IsAnalysisComplete() const -> bool;

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

    /** Get total number of render frames in the trace. 0 when the capture has no rendering frames (e.g. -nullrhi). */
    auto GetRenderFrameCount() const -> uint64;

    /** Get a specific game frame by index. Returns nullptr if index is invalid. */
    auto GetFrame(uint64 Index) const -> const TraceServices::FFrame*;

    /** Read all timer metadata. Callback receives the timer reader. */
    auto ReadTimers(TFunctionRef<void(const TraceServices::ITimingProfilerTimerReader&)> Callback) const -> void;

    /** Read a timeline by index. Callback receives the timeline. Returns false if index is invalid. */
    auto ReadTimeline(uint32 TimelineIndex,
                      TFunctionRef<void(const TraceServices::ITimingProfilerProvider::Timeline&)> Callback) const -> bool;

    /** Get all thread infos as an array of (ThreadId, Name, GroupName). */
    auto GetThreadInfos() const -> TArray<TraceServices::FThreadInfo>;

    /**
     * Enumerate screenshots represented by Screenshot-category trace log messages.
     *
     * Metadata is sorted by timestamp and then ID. The scan does not copy image bytes.
     * Missing providers, categories, log records, or screenshot payloads produce an empty
     * result or incomplete metadata rather than an error.
     */
    auto GetScreenshots() const -> TArray<FCk_TraceScreenshot>;

    /**
     * Copy the embedded payload for one complete screenshot.
     *
     * Returns false and clears OutData when the session/provider/screenshot is missing
     * or the trace only contains a partial payload.
     */
    auto TryCopyScreenshotData(uint32 InScreenshotId, TArray<uint8>& OutData) const -> bool;

private:
    FString _FilePath;
    TSharedPtr<TraceServices::IAnalysisService> _AnalysisService;
    TSharedPtr<const TraceServices::IAnalysisSession> _Session;

    // Cached after Open()
    mutable uint32 _CachedGameThreadId = static_cast<uint32>(INDEX_NONE);
    mutable bool   _GameThreadIdCached = false;
};

// --------------------------------------------------------------------------------------------------------------------
