#include "CkWatermark_Subsystem.h"

#include "CkCore/Engine/CkGameInstance.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Game/CkGame_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkWatermark/Settings/CkWatermark_Settings.h"
#include "CkWatermark/Generated/CkWatermark_BuildId.h"

#include "CkEcs/Settings/CkEcs_Settings.h"
#include "CkWatermark/CkWatermark_Log.h"

#include <GameFramework/PlayerController.h>
#include <HAL/PlatformMisc.h>

extern ENGINE_API uint64 GFrameCounter;

// --------------------------------------------------------------------------------------------------------------------

namespace ck_watermark
{
    // Resolve the initial display policy from the -CkWatermark command-line argument.
    // FParse::Value works at any point during startup — no dependency on the
    // engine's CVar command-line processing pass (which may be skipped in Shipping).
    // Usage: -CkWatermark (defaults to Regular) or -CkWatermark=Regular|Detailed|Hidden
    static auto ResolveCommandLineDisplayPolicy(int32 InDefault) -> int32
    {
        FString Value;
        if (FParse::Value(FCommandLine::Get(), TEXT("-CkWatermark="), Value))
        {
            if (Value.Equals(TEXT("Minimal"), ESearchCase::IgnoreCase))
            { return static_cast<int32>(ECk_Watermark_DisplayPolicy::Minimal); }

            if (Value.Equals(TEXT("Detailed"), ESearchCase::IgnoreCase))
            { return static_cast<int32>(ECk_Watermark_DisplayPolicy::Detailed); }

            if (Value.Equals(TEXT("Hidden"), ESearchCase::IgnoreCase))
            { return static_cast<int32>(ECk_Watermark_DisplayPolicy::Hidden); }

            return static_cast<int32>(ECk_Watermark_DisplayPolicy::Regular);
        }

        if (FParse::Param(FCommandLine::Get(), TEXT("CkWatermark")))
        { return static_cast<int32>(ECk_Watermark_DisplayPolicy::Regular); }

        return InDefault;
    }

    namespace cvar
    {
#if CK_BUILD_SHIPPING
        static auto WatermarkDisplayPolicy = static_cast<int32>(ECk_Watermark_DisplayPolicy::Hidden);
#else
        static auto WatermarkDisplayPolicy = static_cast<int32>(ECk_Watermark_DisplayPolicy::Regular);
#endif
        static auto CVar_WatermarkDisplayPolicy = FAutoConsoleVariableRef(
            TEXT("ck.UI.WatermarkDisplayPolicy"),
            WatermarkDisplayPolicy,
            TEXT("Set the Watermark Widget Display Policy (Hidden, Minimal, Regular, Detailed)"),
            FConsoleVariableDelegate::CreateLambda([](IConsoleVariable* CVar)
            {
                if (ck::Is_NOT_Valid(UCk_GameInstance_UE::Get_Instance()))
                { return; }

                const auto& GameInstance = UCk_Utils_Game_UE::Get_GameInstance(nullptr);

                if (ck::Is_NOT_Valid(GameInstance))
                { return; }

                const auto& LocalPlayer = GameInstance->FindLocalPlayerFromControllerId(0);

                if (ck::Is_NOT_Valid(LocalPlayer))
                { return; }

                const auto& WatermarkSubsystem = LocalPlayer->GetSubsystem<UCk_Watermark_Subsystem_UE>();

                CK_ENSURE_IF_NOT(ck::IsValid(WatermarkSubsystem), TEXT("Could not retrieve Watermark Subsystem"))
                { return; }

                WatermarkSubsystem->Request_UpdateWatermarkDisplayPolicy(static_cast<ECk_Watermark_DisplayPolicy>(WatermarkDisplayPolicy));
            }));

        // ---- Replication stats CVARs -------------------------------------------------
        int32 ReplicationEnabled = 0;
        static auto CVar_ReplicationEnabled = FAutoConsoleVariableRef(
            TEXT("ck.UI.WatermarkReplication"),
            ReplicationEnabled,
            TEXT("Enable (1) or disable (0) replication stats in the watermark. Expensive — iterates all world actors."));

        float ReplicationFrequency = 60.f;
        static auto CVar_ReplicationFrequency = FAutoConsoleVariableRef(
            TEXT("ck.UI.WatermarkReplicationFrequency"),
            ReplicationFrequency,
            TEXT("How often (in seconds) to refresh replication stats. Default: 60."));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Watermark_Subsystem_UE::
    Deinitialize()
    -> void
{
    if (ck::IsValid(_WatermarkWidget))
    {
        if (const auto CachedSlateWidget = _WatermarkWidget->GetCachedWidget(); CachedSlateWidget.IsValid())
        {
            if (const auto* LocalPlayer = GetLocalPlayer();
                ck::IsValid(LocalPlayer) && ck::IsValid(LocalPlayer->ViewportClient))
            {
                LocalPlayer->ViewportClient->RemoveViewportWidgetContent(CachedSlateWidget.ToSharedRef());
            }
        }

        _WatermarkWidget = nullptr;
    }

    Super::Deinitialize();
}

auto
    UCk_Watermark_Subsystem_UE::
    PlayerControllerChanged(
        APlayerController* InNewPlayerController)
    -> void
{
    if (ck::Is_NOT_Valid(InNewPlayerController))
    { return; }

    if (ck::Is_NOT_Valid(_WatermarkWidget))
    {
        DoCreateAndSetWatermarkWidget(InNewPlayerController);
    }

    if (NOT _StaticInfoLogged)
    {
        _StaticInfoLogged = true;
        DoLogStaticInfo(InNewPlayerController);
    }

    if (ck::Is_NOT_Valid(_WatermarkWidget))
    { return; }

    const auto& LocalPlayer = InNewPlayerController->GetLocalPlayer();

    if (ck::Is_NOT_Valid(LocalPlayer))
    { return; }

    const auto& ClientGameViewport = LocalPlayer->ViewportClient;

    if (ck::Is_NOT_Valid(ClientGameViewport))
    { return; }

    // Remove any previously-added Slate widget so we never end up with duplicates
    // (PlayerControllerChanged can fire more than once, e.g. listen-server travel).
    if (const auto CachedSlateWidget = _WatermarkWidget->GetCachedWidget(); CachedSlateWidget.IsValid())
    {
        ClientGameViewport->RemoveViewportWidgetContent(CachedSlateWidget.ToSharedRef());
    }

    ClientGameViewport->AddViewportWidgetContent(_WatermarkWidget->TakeWidget(), UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Widget_ZOrder());
}

auto
    UCk_Watermark_Subsystem_UE::
    Request_UpdateWatermarkDisplayPolicy(
        ECk_Watermark_DisplayPolicy InDisplayPolicy) const
    -> void
{
    if (ck::Is_NOT_Valid(_WatermarkWidget))
    { return; }

    _WatermarkWidget->Request_SetDisplayPolicy(InDisplayPolicy);
}

auto
    UCk_Watermark_Subsystem_UE::
    ForceRebuildWidget()
    -> void
{
    if (ck::Is_NOT_Valid(_WatermarkWidget))
    { return; }

    _WatermarkWidget->TakeWidget();
}

auto
    UCk_Watermark_Subsystem_UE::
    DoCreateAndSetWatermarkWidget(
        APlayerController* InPlayerController)
    -> void
{
    if (ck::IsValid(_WatermarkWidget))
    { return; }

    // Apply command-line override once, before the first widget is created.
    static bool bCommandLineResolved = false;
    if (!bCommandLineResolved)
    {
        bCommandLineResolved = true;
        ck_watermark::cvar::WatermarkDisplayPolicy = ck_watermark::ResolveCommandLineDisplayPolicy(ck_watermark::cvar::WatermarkDisplayPolicy);
    }

    _WatermarkWidget = NewObject<UCkWatermark_Panel_UWidget_UE>(GetLocalPlayer());

    CK_ENSURE_IF_NOT(ck::IsValid(_WatermarkWidget), TEXT("Failed to create the Watermark Panel Widget!"))
    { return; }

    _WatermarkWidget->Request_SetDisplayPolicy(static_cast<ECk_Watermark_DisplayPolicy>(ck_watermark::cvar::WatermarkDisplayPolicy));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Watermark_Subsystem_UE::
    DoLogStaticInfo(
        const APlayerController* InPlayerController) const
    -> void
{
    ck::watermark::Log(TEXT("========================================"));
    ck::watermark::Log(TEXT("  Watermark - Startup Info"));
    ck::watermark::Log(TEXT("========================================"));

    // ---- Build Config --------------------------------------------------------
#if UE_BUILD_SHIPPING
    ck::watermark::Log(TEXT("  Build Config : SHIPPING"));
#elif UE_BUILD_TEST
    ck::watermark::Log(TEXT("  Build Config : TEST"));
#elif UE_BUILD_DEBUG
    ck::watermark::Log(TEXT("  Build Config : DEBUG"));
#else
    ck::watermark::Log(TEXT("  Build Config : DEV"));
#endif

    // ---- Build IDs -----------------------------------------------------------
    {
        static auto BakedHead = FString{UTF8_TO_TCHAR(CkWatermarkBuildId::HeadHash)};
        auto HeadMatchesAny   = false;
        for (auto I = 0; I < CkWatermarkBuildId::BranchCount; ++I)
        {
            auto BranchName = FString{UTF8_TO_TCHAR(CkWatermarkBuildId::BranchNames[I])};
            auto MergeHash = FString{UTF8_TO_TCHAR(CkWatermarkBuildId::MergeBaseHashes[I])};
            const auto Active = (BakedHead == MergeHash);

            if (Active)
            { HeadMatchesAny = true; }

            ck::watermark::Log(TEXT("  {}: {}{}"), BranchName, MergeHash, Active ? TEXT(" [active]") : TEXT(""));
        }

        ck::watermark::LogIf(NOT HeadMatchesAny, TEXT("  HEAD: {} [active]"), BakedHead);
    }

    ck::watermark::Log(TEXT("  ----------------------------------------"));

    // ---- Device Info ---------------------------------------------------------
    {
        auto CpuBrand = FPlatformMisc::GetCPUBrand().TrimStartAndEnd();
        ck::watermark::Log(TEXT("  CPU          : {}"), CpuBrand.IsEmpty() ? FString{TEXT("---")} : CpuBrand);
    }
    {
        auto OsVer = FPlatformMisc::GetOSVersion();
        ck::watermark::Log(TEXT("  OS           : {}"), OsVer.IsEmpty() ? FString{TEXT("---")} : OsVer);
    }
    {
        auto Physical = FPlatformMisc::NumberOfCores();
        auto Logical  = FPlatformMisc::NumberOfCoresIncludingHyperthreads();
        ck::watermark::Log(TEXT("  Cores        : {}c / {}t"), Physical, Logical);
    }
    {
        auto NetType = TEXT("Unknown");
        switch (FPlatformMisc::GetNetworkConnectionType())
        {
            case ENetworkConnectionType::None:         NetType = TEXT("None");      break;
            case ENetworkConnectionType::AirplaneMode: NetType = TEXT("Airplane");  break;
            case ENetworkConnectionType::Cell:         NetType = TEXT("Cell");      break;
            case ENetworkConnectionType::WiFi:         NetType = TEXT("WiFi");      break;
            case ENetworkConnectionType::WiMAX:        NetType = TEXT("WiMAX");     break;
            case ENetworkConnectionType::Bluetooth:    NetType = TEXT("Bluetooth"); break;
            case ENetworkConnectionType::Ethernet:     NetType = TEXT("Ethernet");  break;
            default:                                                                break;
        }
        ck::watermark::Log(TEXT("  Net Type     : {}"), NetType);
    }
    {
        auto Role = TEXT("---");
        if (ck::IsValid(InPlayerController))
        {
            if (const auto* World = InPlayerController->GetWorld())
            {
                switch (World->GetNetMode())
                {
                    case NM_Standalone:      Role = TEXT("SinglePlayer"); break;
                    case NM_DedicatedServer: Role = TEXT("Server");       break;
                    case NM_ListenServer:    Role = TEXT("ListenServer"); break;
                    case NM_Client:          Role = TEXT("Client");       break;
                    default:                                              break;
                }
            }
        }
        ck::watermark::Log(TEXT("  Role         : {}"), Role);
    }

    ck::watermark::Log(TEXT("  ----------------------------------------"));

    // ---- ECS Debug -----------------------------------------------------------
    {
        auto EcsDbg = TEXT("---");
        switch (UCk_Utils_Ecs_Settings_UE::Get_HandleDebuggerBehavior())
        {
            case ECk_Ecs_HandleDebuggerBehavior::Disable:                      EcsDbg = TEXT("Off");     break;
            case ECk_Ecs_HandleDebuggerBehavior::Enable:                       EcsDbg = TEXT("On");      break;
            case ECk_Ecs_HandleDebuggerBehavior::EnableWithBlueprintDebugging: EcsDbg = TEXT("On (BP)"); break;
            default:                                                                                     break;
        }
        ck::watermark::Log(TEXT("  ECS DBG      : {}"), EcsDbg);
    }
    {
        auto EntityMap = TEXT("---");
        switch (UCk_Utils_Ecs_Settings_UE::Get_EntityMapPolicy())
        {
            case ECk_Ecs_EntityMap_Policy::DoNotLog:  EntityMap = TEXT("Off");        break;
            case ECk_Ecs_EntityMap_Policy::AlwaysLog: EntityMap = TEXT("Always Log"); break;
            default:                                                                  break;
        }
        ck::watermark::Log(TEXT("  EntityMap    : {}"), EntityMap);
    }
    {
        auto Cpp        = UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Cpp();
        auto BP         = UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Blueprint();
        auto AS         = UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Angelscript();
        auto Callstacks = FString{};

        if (NOT Cpp && NOT BP && NOT AS)
        { Callstacks = TEXT("---"); }
        else
        {
            if (Cpp)
            { Callstacks += TEXT("C++"); }

            if (BP)
            { Callstacks += Callstacks.IsEmpty() ? TEXT("BP") : TEXT(" BP"); }

            if (AS)
            { Callstacks += Callstacks.IsEmpty() ? TEXT("AS") : TEXT(" AS"); }
        }
        ck::watermark::Log(TEXT("  Callstacks   : {}"), Callstacks);
    }

    ck::watermark::Log(TEXT("========================================"));
}

// ---- Static convenience helpers -----------------------------------------------------

void
    UCk_Watermark_Subsystem_UE::
    NotifyActivityActive(
        APlayerController* InPlayerController,
        FName              InActivityId,
        const FText&       InDisplayLabel)
{
    if (ck::Is_NOT_Valid(InPlayerController))
    { return; }

    const auto* LocalPlayer = InPlayerController->GetLocalPlayer();

    if (ck::Is_NOT_Valid(LocalPlayer))
    { return; }

    if (auto* Sub = LocalPlayer->GetSubsystem<UCk_Watermark_Subsystem_UE>())
    {
        Sub->Request_ActivityActive(InActivityId, InDisplayLabel);
    }
}

void
    UCk_Watermark_Subsystem_UE::
    NotifyActivityInactive(
        APlayerController* InPlayerController,
        FName              InActivityId)
{
    if (ck::Is_NOT_Valid(InPlayerController))
    { return; }

    const auto* LocalPlayer = InPlayerController->GetLocalPlayer();

    if (ck::Is_NOT_Valid(LocalPlayer))
    { return; }

    if (auto* Sub = LocalPlayer->GetSubsystem<UCk_Watermark_Subsystem_UE>())
    {
        Sub->Request_ActivityInactive(InActivityId);
    }
}

// ---- Activity Bar API ---------------------------------------------------------------

void
    UCk_Watermark_Subsystem_UE::
    Request_ActivityActive(
        FName InActivityId,
        const FText& InDisplayLabel)
{
    // If this Id is already active, skip — the existing entry will naturally
    // show the held underline accent (GFrameCounter > ActivatedFrame).
    for (const FCkWatermarkActivityState& Existing : _ActivityStates)
    {
        if (Existing.Id == InActivityId && Existing.bActive)
        { return; }
    }

    // When a new Id appears, reset all sequence counters so numbers stay low.
    if (!_ActivitySequenceCounters.Contains(InActivityId))
    {
        _ActivitySequenceCounters.Reset();
    }

    // Create a new entry so each press/release cycle appears as a
    // separate chip in the history (e.g. LMB, LMB, TAB).
    int32& SeqCounter = _ActivitySequenceCounters.FindOrAdd(InActivityId);
    SeqCounter += 1;

    FCkWatermarkActivityState NewState;
    NewState.Id              = InActivityId;
    NewState.Label           = InDisplayLabel;
    NewState.SequenceNumber  = SeqCounter;
    NewState.bActive         = true;
    NewState.ActivatedFrame  = GFrameCounter;
    _ActivityStates.Add(MoveTemp(NewState));

    DoTrimActivityHistory();
    DoSortActivityStates();
    ++_ActivityVersion;
}

void
    UCk_Watermark_Subsystem_UE::
    Request_ActivityInactive(
        FName InActivityId)
{
    // Find the most recent ACTIVE entry with this Id (search from the end).
    for (int32 i = _ActivityStates.Num() - 1; i >= 0; --i)
    {
        if (_ActivityStates[i].Id == InActivityId && _ActivityStates[i].bActive)
        {
            _ActivityStates[i].bActive          = false;
            _ActivityStates[i].DeactivatedFrame = GFrameCounter;
            break;
        }
    }

    DoTrimActivityHistory();
    DoSortActivityStates();
    ++_ActivityVersion;
}

auto
    UCk_Watermark_Subsystem_UE::
    Get_ActivityStates() const
    -> const TArray<FCkWatermarkActivityState>&
{
    return _ActivityStates;
}

auto
    UCk_Watermark_Subsystem_UE::
    Get_ActivityVersion() const
    -> uint32
{
    return _ActivityVersion;
}

auto
    UCk_Watermark_Subsystem_UE::
    DoSortActivityStates()
    -> void
{
    // Inactive items first (oldest deactivation on the left), then active items
    // (oldest activation on the left, newest on the right).
    _ActivityStates.Sort([](const FCkWatermarkActivityState& A, const FCkWatermarkActivityState& B) -> bool
    {
        if (A.bActive != B.bActive)
        {
            return !A.bActive; // Inactive before active.
        }
        if (!A.bActive)
        {
            return A.DeactivatedFrame < B.DeactivatedFrame;
        }
        return A.ActivatedFrame < B.ActivatedFrame;
    });
}

auto
    UCk_Watermark_Subsystem_UE::
    DoTrimActivityHistory()
    -> void
{
    const int32 MaxHistory = UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_ActivityBar_MaxHistory();

    if (_ActivityStates.Num() <= MaxHistory)
    { return; }

    // Remove oldest inactive entries first. They will be at the front after sorting,
    // but sorting hasn't happened yet so we need to identify them manually.
    // Remove inactive entries with the smallest DeactivatedFrame until we're within budget.
    while (_ActivityStates.Num() > MaxHistory)
    {
        int32 OldestInactiveIdx = INDEX_NONE;
        uint64 OldestFrame = UINT64_MAX;

        for (int32 i = 0; i < _ActivityStates.Num(); ++i)
        {
            if (!_ActivityStates[i].bActive && _ActivityStates[i].DeactivatedFrame < OldestFrame)
            {
                OldestFrame       = _ActivityStates[i].DeactivatedFrame;
                OldestInactiveIdx = i;
            }
        }

        if (OldestInactiveIdx != INDEX_NONE)
        {
            _ActivityStates.RemoveAtSwap(OldestInactiveIdx);
        }
        else
        {
            // All entries are active — nothing to trim.
            break;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
