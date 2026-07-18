#pragma once

#include "CkAssetExporter/Server/CkAssetExporter_RequestProcessor.h"

#include "CkCore/Macros/CkMacros.h"

#include "Containers/Ticker.h"

#include <CoreMinimal.h>
#include <EditorSubsystem.h>

#include "CkAssetExporter_BridgeSubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Editor-subsystem bridge that lets the user's interactive editor serve the SAME Requests -> Results file protocol the
// -ExportServer commandlet serves — so an agent's exports cost zero editor boots while the editor is already open.
//
// A 0.5s ticker lazily CLAIMS serving: if no server.json exists it claims (starts the shared request processor); if
// one exists it defers to a live owner (another editor or the -ExportServer commandlet) and only reclaims a
// demonstrably-stale slot. While claimed it drains pending requests each tick. A quit op (e.g. -StopServer) releases
// the claim WITHOUT exiting the editor, then re-claims only once new requests arrive — a clean handoff.
//
// Dormant during ANY commandlet run (IsRunningCommandlet) so it never double-serves alongside -ExportServer or a
// one-shot export commandlet. No idle / wall-clock watchdogs here — the editor's lifetime is the user's business.
// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKASSETEXPORTER_API UCkAssetExporter_BridgeSubsystem : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCkAssetExporter_BridgeSubsystem);

public:
    auto
    Initialize(
        FSubsystemCollectionBase& InCollection) -> void override;

    auto
    Deinitialize() -> void override;

private:
    auto
    OnTick(
        float InDeltaTime) -> bool;

    // Attempt to become the serving process. No-op when already serving, when deferring to a live owner, or (after a
    // quit handoff) until a new request arrives. Sets _IsServing on success.
    auto
    DoTryClaimServing() -> void;

    // Release server.json and stop serving WITHOUT exiting the editor; arms the re-claim-on-next-request gate.
    auto
    DoReleaseServing() -> void;

private:
    FCk_AssetExporter_RequestProcessor _Processor;
    FTSTicker::FDelegateHandle         _TickerHandle;

    bool _IsServing = false;

    // After a quit op releases the claim, only re-claim once Requests/ is non-empty — so -StopServer against the
    // bridge hands off cleanly instead of the bridge immediately re-grabbing the slot in the idle gap.
    bool _AwaitingRequestToReclaim = false;
};

// --------------------------------------------------------------------------------------------------------------------
