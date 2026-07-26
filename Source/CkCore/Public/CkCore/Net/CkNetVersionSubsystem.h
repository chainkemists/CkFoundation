#pragma once

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include <CoreMinimal.h>

#include "CkNetVersionSubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class ACk_NetVersionReport_UE;
class AController;
class AGameModeBase;
class APlayerController;

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCORE_API UCk_NetVersion_WorldSubsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_NetVersion_WorldSubsystem_UE);

public:
    auto OnWorldBeginPlay(
        UWorld& InWorld) -> void override;
    auto Deinitialize() -> void override;

public:
    // Empty if there is no networked session or the report has not replicated/stamped yet.
    auto Get_LocalServerBuildId() const -> FString;

    // Every report EXCEPT the local player's, sorted by owning PlayerId; only fully populated on the server.
    auto Get_RemoteClientReports() const -> TArray<const ACk_NetVersionReport_UE*>;

public:
    // Called by ACk_NetVersionReport_UE in BeginPlay/EndPlay so the facade never iterates actors per frame.
    auto Request_RegisterReport(
        ACk_NetVersionReport_UE* InReport) -> void;
    auto Request_UnregisterReport(
        ACk_NetVersionReport_UE* InReport) -> void;

private:
    auto DoHandlePostLogin(
        AGameModeBase* InGameMode,
        APlayerController* InNewPlayer) -> void;
    auto DoHandleLogout(
        AGameModeBase* InGameMode,
        AController* InController) -> void;
    auto DoSpawnReportForController(
        APlayerController* InController) -> void;
    auto Get_IsServer() const -> bool;

private:
    auto Get_HasReportForController(
        const APlayerController* InController) const -> bool;
    // Resolved by IsLocalController(), not GetFirstPlayerController() — on a listen host the host's PC
    // is not guaranteed to be first.
    auto Get_LocalController() const -> const APlayerController*;

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<ACk_NetVersionReport_UE>> _Reports;

    FDelegateHandle _PostLoginHandle;
    FDelegateHandle _LogoutHandle;
};

// --------------------------------------------------------------------------------------------------------------------
