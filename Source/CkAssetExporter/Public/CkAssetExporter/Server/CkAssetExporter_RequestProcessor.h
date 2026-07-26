#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------
// Deliberately contains NO watchdog / sleep / lifetime policy: the driving loop owns idle + wall-clock timeouts
// (commandlet) or the editor's own lifetime (bridge).
// --------------------------------------------------------------------------------------------------------------------

struct FCk_AssetExporter_ProcessResult
{
    bool QuitRequested = false;

    // Lets the driving loop reset its idle watchdog only when a request was actually consumed this pass.
    bool AnyProcessed  = false;
};

// --------------------------------------------------------------------------------------------------------------------

enum class ECk_AssetExporter_StaleRequestPolicy : uint8
{
    // Crash-recovery for the -ExportServer commandlet, which OWNS the queue at boot.
    WipeStale,

    // The editor bridge: it must never discard work someone queued, and MUST NOT delete the very request that
    // triggered a re-claim after a quit handoff.
    PreserveExisting,
};

// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_AssetExporter_RequestProcessor
{
public:
    FCk_AssetExporter_RequestProcessor();

public:
    // Returns false ONLY if the protocol dirs could not be created. Re-runnable: server.json is overwritten each call.
    auto
    Startup(
        ECk_AssetExporter_StaleRequestPolicy InStaleRequestPolicy = ECk_AssetExporter_StaleRequestPolicy::WipeStale) -> bool;

    auto
    ProcessPending() -> FCk_AssetExporter_ProcessResult;

    // Safe to call whether or not Startup ran.
    auto
    Shutdown() -> void;

public:
    auto
    Has_PendingRequests() const -> bool;

    auto Get_RequestsDir() const -> const FString&     { return _RequestsDir; }
    auto Get_ResultsDir() const -> const FString&      { return _ResultsDir; }
    auto Get_ServerStatusPath() const -> const FString& { return _ServerStatusPath; }

private:
    // Written at Startup, before each request (busy=true + the request name), and after each (busy=false), so a
    // FOREIGN session's -Status can tell an actively-working server from a forgotten idle one before stopping it.
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
