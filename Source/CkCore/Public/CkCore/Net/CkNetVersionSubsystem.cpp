#include "CkNetVersionSubsystem.h"

#include "CkCore/Net/CkNetVersionReport.h"
#include "CkCore/Validation/CkIsValid.h"

#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_NetVersion_WorldSubsystem_UE::
    OnWorldBeginPlay(
        UWorld& InWorld)
    -> void
{
    Super::OnWorldBeginPlay(InWorld);

    if (NOT Get_IsServer())
    { return; }

    // Covers controllers that logged in before we subscribed below (e.g. the listen-host's own player).
    for (auto It = InWorld.GetPlayerControllerIterator(); It; ++It)
    {
        if (auto* Controller = It->Get())
        { DoSpawnReportForController(Controller); }
    }

    _PostLoginHandle = FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &ThisClass::DoHandlePostLogin);
    _LogoutHandle    = FGameModeEvents::GameModeLogoutEvent.AddUObject(this, &ThisClass::DoHandleLogout);
}

auto
    UCk_NetVersion_WorldSubsystem_UE::
    Deinitialize()
    -> void
{
    if (_PostLoginHandle.IsValid())
    { FGameModeEvents::GameModePostLoginEvent.Remove(_PostLoginHandle); }
    if (_LogoutHandle.IsValid())
    { FGameModeEvents::GameModeLogoutEvent.Remove(_LogoutHandle); }

    Super::Deinitialize();
}

auto
    UCk_NetVersion_WorldSubsystem_UE::
    Get_LocalServerBuildId() const
    -> FString
{
    const auto* LocalController = Get_LocalController();

    for (const auto& Report : _Reports)
    {
        const auto* ReportPtr = Report.Get();
        if (ck::IsValid(ReportPtr) && ReportPtr->GetOwner() == LocalController)
        { return ReportPtr->Get_ServerBuildId(); }
    }
    return {};
}

auto
    UCk_NetVersion_WorldSubsystem_UE::
    Get_RemoteClientReports() const
    -> TArray<const ACk_NetVersionReport_UE*>
{
    TArray<const ACk_NetVersionReport_UE*> Out;

    const auto* LocalController = Get_LocalController();

    for (const auto& Report : _Reports)
    {
        const auto* ReportPtr = Report.Get();
        if (ck::Is_NOT_Valid(ReportPtr) || ReportPtr->GetOwner() == LocalController)
        { continue; }
        Out.Add(ReportPtr);
    }

    Out.Sort([](const ACk_NetVersionReport_UE& InA, const ACk_NetVersionReport_UE& InB) -> bool
    {
        const auto Get_PlayerId = [](const ACk_NetVersionReport_UE& InReport) -> int32
        {
            const auto* Controller = Cast<APlayerController>(InReport.GetOwner());
            return (ck::IsValid(Controller) && ck::IsValid(Controller->PlayerState))
                ? Controller->PlayerState->GetPlayerId()
                : TNumericLimits<int32>::Max();
        };
        return Get_PlayerId(InA) < Get_PlayerId(InB);
    });

    return Out;
}

auto
    UCk_NetVersion_WorldSubsystem_UE::
    Request_RegisterReport(
        ACk_NetVersionReport_UE* InReport)
    -> void
{
    if (ck::Is_NOT_Valid(InReport))
    { return; }
    _Reports.AddUnique(InReport);
}

auto
    UCk_NetVersion_WorldSubsystem_UE::
    Request_UnregisterReport(
        ACk_NetVersionReport_UE* InReport)
    -> void
{
    _Reports.Remove(InReport);
}

auto
    UCk_NetVersion_WorldSubsystem_UE::
    DoHandlePostLogin(
        AGameModeBase* InGameMode,
        APlayerController* InNewPlayer)
    -> void
{
    // FGameModeEvents is process-global across PIE worlds — only act on logins into our world.
    if (ck::Is_NOT_Valid(InGameMode) || InGameMode->GetWorld() != GetWorld())
    { return; }

    DoSpawnReportForController(InNewPlayer);
}

auto
    UCk_NetVersion_WorldSubsystem_UE::
    DoHandleLogout(
        AGameModeBase* InGameMode,
        AController* InController)
    -> void
{
    if (ck::Is_NOT_Valid(InGameMode) || InGameMode->GetWorld() != GetWorld())
    { return; }

    const auto* Controller = Cast<APlayerController>(InController);
    if (ck::Is_NOT_Valid(Controller))
    { return; }

    // Collect first, then destroy — Destroy() fires the report's EndPlay which unregisters it from _Reports.
    TArray<ACk_NetVersionReport_UE*> ToDestroy;
    for (const auto& Report : _Reports)
    {
        auto* ReportPtr = Report.Get();
        if (ck::IsValid(ReportPtr) && ReportPtr->GetOwner() == Controller)
        { ToDestroy.Add(ReportPtr); }
    }
    for (auto* Report : ToDestroy)
    { Report->Destroy(); }
}

auto
    UCk_NetVersion_WorldSubsystem_UE::
    DoSpawnReportForController(
        APlayerController* InController)
    -> void
{
    if (ck::Is_NOT_Valid(InController))
    { return; }
    if (Get_HasReportForController(InController))
    { return; }

    auto SpawnParams = FActorSpawnParameters{};
    SpawnParams.Owner = InController;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    GetWorld()->SpawnActor<ACk_NetVersionReport_UE>(ACk_NetVersionReport_UE::StaticClass(), SpawnParams);
}

auto
    UCk_NetVersion_WorldSubsystem_UE::
    Get_HasReportForController(
        const APlayerController* InController) const
    -> bool
{
    for (const auto& Report : _Reports)
    {
        const auto* ReportPtr = Report.Get();
        if (ck::IsValid(ReportPtr) && ReportPtr->GetOwner() == InController)
        { return true; }
    }
    return false;
}

auto
    UCk_NetVersion_WorldSubsystem_UE::
    Get_LocalController() const
    -> const APlayerController*
{
    const auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return nullptr; }

    for (auto It = World->GetPlayerControllerIterator(); It; ++It)
    {
        const auto* Controller = It->Get();
        if (ck::IsValid(Controller) && Controller->IsLocalController())
        { return Controller; }
    }
    return nullptr;
}

auto
    UCk_NetVersion_WorldSubsystem_UE::
    Get_IsServer() const
    -> bool
{
    const auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return false; }

    const auto NetMode = World->GetNetMode();
    return NetMode == NM_DedicatedServer || NetMode == NM_ListenServer;
}

// --------------------------------------------------------------------------------------------------------------------
