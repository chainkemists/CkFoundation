#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------
// The request-file processing CORE shared by the -ExportServer commandlet loop and the interactive-editor bridge.
// Owns the on-disk protocol dirs (<ProjectSaved>/CkAssetExporter/{Requests,Results}, server.json — computed once at
// construction) and the per-poll consumption of Requests/*.json -> Results/<basename>.json via the dispatch layer
// (op = export | list | dumpGraph | quit). Deliberately contains NO watchdog / sleep / lifetime policy: the driving
// loop owns idle + wall-clock timeouts (commandlet) or the editor's own lifetime (bridge).
// --------------------------------------------------------------------------------------------------------------------

// Outcome of a single ProcessPending poll pass. The driving loop needs BOTH signals: QuitRequested to break out and
// stop, AnyProcessed to reset its idle watchdog only when a request was actually consumed this pass.
struct FCk_AssetExporter_ProcessResult
{
    bool QuitRequested = false;
    bool AnyProcessed  = false;
};

// --------------------------------------------------------------------------------------------------------------------

// How Startup treats request files already sitting in Requests/ when serving begins.
enum class ECk_AssetExporter_StaleRequestPolicy : uint8
{
    // Delete every queued request. Crash-recovery for the dedicated -ExportServer commandlet, which OWNS the queue at
    // boot: a request left behind by a crashed prior server would otherwise be re-processed against whatever it asked
    // for.
    WipeStale,

    // Leave queued requests in place so they get served. The editor bridge uses this: it must never silently discard
    // work someone queued, and MUST NOT delete the very request that triggered a re-claim after a quit handoff.
    PreserveExisting,
};

// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_AssetExporter_RequestProcessor
{
public:
    FCk_AssetExporter_RequestProcessor();

public:
    // Create the Requests/Results dirs, (per policy) wipe stale requests, wait once for the asset registry to finish
    // scanning, then publish server.json (pid / startedAt / project / dirs — all absolute so external submitters
    // resolve them independent of the editor's cwd). Returns false ONLY if the protocol dirs could not be created.
    // Re-runnable: server.json is overwritten each call.
    auto
    Startup(
        ECk_AssetExporter_StaleRequestPolicy InStaleRequestPolicy = ECk_AssetExporter_StaleRequestPolicy::WipeStale) -> bool;

    // One poll pass: enumerate + sort request files, skip sub-second-old ones (mid-write settle guard), run each op,
    // write its result, delete the request AFTER the result is written, CollectGarbage. Never sleeps, never checks a
    // watchdog — that is the driving loop's job.
    auto
    ProcessPending() -> FCk_AssetExporter_ProcessResult;

    // Remove server.json so external drivers can tell serving has stopped. Safe to call whether or not Startup ran.
    auto
    Shutdown() -> void;

public:
    // True when at least one *.json sits in Requests/ — the bridge's re-claim gate after a quit handoff.
    auto
    Has_PendingRequests() const -> bool;

    auto Get_RequestsDir() const -> const FString&     { return _RequestsDir; }
    auto Get_ResultsDir() const -> const FString&      { return _ResultsDir; }
    auto Get_ServerStatusPath() const -> const FString& { return _ServerStatusPath; }

private:
    // Rewrites server.json with the current serving state (pid/startedAt/dirs + busy/currentOp/lastActivityAt) so a
    // FOREIGN session's -Status can tell an actively-working server from a forgotten idle one before deciding to
    // stop it. Written at Startup, before each request (busy=true + the request name), and after each (busy=false).
    auto
    Do_WriteStatusFile(
        bool InBusy,
        const FString& InCurrentOp) -> void;

private:
    FString _Root;
    FString _RequestsDir;
    FString _ResultsDir;
    FString _ServerStatusPath;
    FString _StartedAtIso;
};

// --------------------------------------------------------------------------------------------------------------------
