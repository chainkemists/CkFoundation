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

// Owns the client/server build-version surface for a world, decoupled from the project's GameState/PlayerState
// class. On the server it spawns one ACk_NetVersionReport_UE per player (owned by that player's controller) and
// destroys it on logout. On every net role it acts as the registry the watermark reads from.
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
    // Watermark facade -------------------------------------------------------------------------------------

    // The server build id as seen by the local player (the report owned by the local controller). Empty if
    // there is no networked session or the report has not replicated/stamped yet.
    auto Get_LocalServerBuildId() const -> FString;

    // Every report EXCEPT the local player's, sorted by owning PlayerId. Used by the host to list connected
    // clients and flag any on the wrong version. Only fully populated on the server (listen host).
    auto Get_RemoteClientReports() const -> TArray<const ACk_NetVersionReport_UE*>;

public:
    // Called by ACk_NetVersionReport_UE in BeginPlay/EndPlay so the facade can enumerate them without a
    // per-frame actor iteration.
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
    // The local player's controller, resolved by IsLocalController() rather than positional
    // GetFirstPlayerController() (on a listen host the host's PC is not guaranteed to be first).
    auto Get_LocalController() const -> const APlayerController*;

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<ACk_NetVersionReport_UE>> _Reports;

    FDelegateHandle _PostLoginHandle;
    FDelegateHandle _LogoutHandle;
};

// --------------------------------------------------------------------------------------------------------------------
