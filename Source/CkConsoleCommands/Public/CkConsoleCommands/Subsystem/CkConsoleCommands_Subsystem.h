#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/World/CkEcsWorld.h"

#include <GameFramework/GameModeBase.h>
#include <Subsystems/WorldSubsystem.h>

#include "CkConsoleCommands_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCONSOLECOMMANDS_API ACk_ConsoleCommandsHelper_UE : public AInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_ConsoleCommandsHelper_UE);

public:
    ACk_ConsoleCommandsHelper_UE();

protected:
    auto BeginPlay() -> void override;

public:
    UFUNCTION(Server, Reliable)
    void Server_Request_RunConsoleCommandOnServer(
        const FString&  InCommand);

    UFUNCTION(Server, Reliable)
    void Server_Request_RunConsoleCommandOnAll(
        const FString&  InCommand);

    UFUNCTION(NetMulticast, Reliable)
    void MC_Request_RunConsoleCommandOnAll(
        const FString&  InCommand);

private:
    auto RunConsoleCommand(
        const FString& InCommand)
        -> void;

    UFUNCTION(NetMulticast, Reliable)
    void MC_Request_OutputOnAll(
        const FString& InCommandOutput,
        const FString& InNetModeName,
        const FString& InContext);

    UFUNCTION(Server, Reliable)
    void Server_Request_OutputOnAll(
        const FString& InCommandOutput,
        const FString& InNetModeName,
        const FString& InContext);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, BlueprintType, DisplayName = "CkSubsystem_ConsoleCommands")
class CKCONSOLECOMMANDS_API UCk_ConsoleCommands_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

    friend ACk_ConsoleCommandsHelper_UE;

public:
    CK_GENERATED_BODY(UCk_ConsoleCommands_Subsystem_UE);

public:
    auto Initialize(FSubsystemCollectionBase& Collection) -> void override;
    auto Deinitialize() -> void override;

public:
    UFUNCTION()
    void
    OnNewPlayerControllerAdded(
        AGameModeBase* InGameModeBase,
        APlayerController* InPlayerController);

    UFUNCTION()
    void
    RunConsoleCommand_OnServer(
        const TArray<FString>& InCommands);

    UFUNCTION(BlueprintCallable)
    void
    Request_RunConsoleCommand_OnServer(
        const FString& InCommand);

    UFUNCTION()
    void
    RunConsoleCommand_OnServerAndOwningClient(
        const TArray<FString>& InCommands);

    UFUNCTION(BlueprintCallable)
    void
    Request_RunConsoleCommand_OnServerAndOwningClient(
        const FString& InCommand);

    UFUNCTION()
    void
    RunConsoleCommand_OnAll(
        const TArray<FString>& InCommands);

    UFUNCTION(BlueprintCallable)
    void
    Request_RunConsoleCommand_OnAll(
        const FString& InCommand);

private:
    auto DoSpawnConsoleCommandsActor(APlayerController* InPlayerController) -> void;
    auto DoUnregisterConsoleCommand() const -> void;

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<ACk_ConsoleCommandsHelper_UE>> _ConsoleCommandsHelper;

private:
    inline static IConsoleObject* _ConsoleCommand = nullptr;

private:
    FDelegateHandle _PostLoginDelegateHandle;
};

// --------------------------------------------------------------------------------------------------------------------
