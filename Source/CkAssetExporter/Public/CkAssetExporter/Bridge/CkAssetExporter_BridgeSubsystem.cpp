#include "CkAssetExporter_BridgeSubsystem.h"

#include "CkAssetExporter_Log.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "AssetRegistry/AssetRegistryModule.h"

#include <CoreGlobals.h>
#include <Dom/JsonObject.h>
#include <HAL/FileManager.h>
#include <HAL/PlatformProcess.h>
#include <Misc/FileHelper.h>
#include <Misc/Optional.h>
#include <Serialization/JsonReader.h>
#include <Serialization/JsonSerializer.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_asset_exporter_bridge
{
    constexpr auto PollSeconds = float{0.5f};

    // Unset if server.json is missing / malformed / lacks the field.
    static auto
    Read_ServerPid(
        const FString& InServerStatusPath)
        -> TOptional<uint32>
    {
        auto JsonString = FString{};
        if (NOT FFileHelper::LoadFileToString(JsonString, *InServerStatusPath))
        { return {}; }

        auto Status = TSharedPtr<FJsonObject>{};
        const auto Reader = TJsonReaderFactory<>::Create(JsonString);
        if (NOT FJsonSerializer::Deserialize(Reader, Status) || NOT Status.IsValid())
        { return {}; }

        auto Pid = uint32{0};
        if (NOT Status->TryGetNumberField(TEXT("pid"), Pid))
        { return {}; }

        return Pid;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetExporter_BridgeSubsystem::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    // NEVER activate under a commandlet: those runs own the queue themselves, and two servers would double-serve.
    if (IsRunningCommandlet())
    { return; }

    using namespace ck_asset_exporter_bridge;
    _TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UCkAssetExporter_BridgeSubsystem::OnTick), PollSeconds);

    ck::asset_exporter::Display(TEXT("[Bridge] editor bridge armed — polling for export requests every 500ms"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetExporter_BridgeSubsystem::
    Deinitialize()
    -> void
{
    if (_TickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(_TickerHandle);
        _TickerHandle.Reset();
    }

    if (_IsServing)
    {
        _Processor.Shutdown();
        _IsServing = false;
    }

    Super::Deinitialize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetExporter_BridgeSubsystem::
    OnTick(
        float InDeltaTime)
    -> bool
{
    if (NOT _IsServing)
    { DoTryClaimServing(); }

    if (_IsServing)
    {
        const auto PollResult = _Processor.ProcessPending();
        if (PollResult.QuitRequested)
        { DoReleaseServing(); }
    }

    constexpr auto KeepTicking = true;
    return KeepTicking;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetExporter_BridgeSubsystem::
    DoTryClaimServing()
    -> void
{
    using namespace ck_asset_exporter_bridge;

    if (_AwaitingRequestToReclaim && NOT _Processor.Has_PendingRequests())
    { return; }

    // Defer claiming until the registry scan finishes: Startup's WaitForCompletion would otherwise BLOCK the editor
    // game thread and freeze the UI for the whole post-boot scan.
    const auto* AssetRegistryModule = FModuleManager::GetModulePtr<FAssetRegistryModule>(TEXT("AssetRegistry"));
    if (AssetRegistryModule == nullptr || AssetRegistryModule->Get().IsLoadingAssets())
    { return; }

    auto& FileManager = IFileManager::Get();
    const auto& ServerStatusPath = _Processor.Get_ServerStatusPath();

    if (FileManager.FileExists(*ServerStatusPath))
    {
        const auto OurPid   = FPlatformProcess::GetCurrentProcessId();
        const auto OwnerPid = Read_ServerPid(ServerStatusPath);

        const auto IsOwnedByLiveOther =
            OwnerPid.IsSet() &&
            OwnerPid.GetValue() != OurPid &&
            FPlatformProcess::IsApplicationRunning(OwnerPid.GetValue());

        if (IsOwnedByLiveOther)
        {
            // An unreadable name defers too: never risk two servers writing the same Results. Reclaiming is only for
            // a live owner that is demonstrably an unrelated process which inherited the recorded pid.
            const auto OwnerName    = FPlatformProcess::GetApplicationName(OwnerPid.GetValue());
            const auto DeferToOwner = OwnerName.IsEmpty() || OwnerName.Contains(TEXT("Editor"));

            if (DeferToOwner)
            { return; }

            ck::asset_exporter::Display(
                TEXT("[Bridge] server.json pid {} is a live non-editor process [{}] — reclaiming (stale pid reuse)"),
                OwnerPid.GetValue(), OwnerName);
        }
        // Otherwise: dead owner, unreadable pid, our own leftover, or reclaimable pid reuse — fall through and
        // (re)claim. Startup rewrites server.json, so the stale file needs no explicit delete.
    }

    // PreserveExisting: a re-claim is TRIGGERED by a pending request, so startup must never discard queued work.
    if (NOT _Processor.Startup(ECk_AssetExporter_StaleRequestPolicy::PreserveExisting))
    {
        ck::asset_exporter::Warning(TEXT("[Bridge] could not start serving (request dirs unavailable) — will retry"));
        return;
    }

    _IsServing = true;
    _AwaitingRequestToReclaim = false;
    ck::asset_exporter::Display(TEXT("[Bridge] serving requests (pid {})"), FPlatformProcess::GetCurrentProcessId());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetExporter_BridgeSubsystem::
    DoReleaseServing()
    -> void
{
    _Processor.Shutdown();
    _IsServing = false;
    _AwaitingRequestToReclaim = true;

    ck::asset_exporter::Display(
        TEXT("[Bridge] quit handled — released serving (will re-claim when new requests arrive)"));
}

// --------------------------------------------------------------------------------------------------------------------
