#pragma once

#include "CkAssetExporter/Server/CkAssetExporter_RequestProcessor.h"

#include "CkCore/Macros/CkMacros.h"

#include "Containers/Ticker.h"

#include <CoreMinimal.h>
#include <EditorSubsystem.h>

#include "CkAssetExporter_BridgeSubsystem.generated.h"

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

    // No-op when already serving, when deferring to a live owner, or (after a quit handoff) until a request arrives.
    auto
    DoTryClaimServing() -> void;

    // Release server.json and stop serving WITHOUT exiting the editor; arms the re-claim-on-next-request gate.
    auto
    DoReleaseServing() -> void;

private:
    FCk_AssetExporter_RequestProcessor _Processor;
    FTSTicker::FDelegateHandle         _TickerHandle;

    bool _IsServing = false;

    // -StopServer against the bridge must hand off cleanly, not have the bridge re-grab the slot in the idle gap.
    bool _AwaitingRequestToReclaim = false;
};

// --------------------------------------------------------------------------------------------------------------------
