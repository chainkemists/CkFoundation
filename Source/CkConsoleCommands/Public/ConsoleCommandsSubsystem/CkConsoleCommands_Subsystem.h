#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/World/CkEcsWorld.h"

#include <GameFramework/GameModeBase.h>
#include <Subsystems/WorldSubsystem.h>

#include "CkConsoleCommands_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCONSOLECOMMANDS_API ACk_ConsoleCommandsHelper_UE : public AActor
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
    void Server_Request_RunConsoleCommand(
        const FString&  InCommand);

private:
    UFUNCTION(NetMulticast, Reliable)
    void MC_Request_OutputOnAll(
        const FString& InCommandOutput);
};


// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, BlueprintType, DisplayName = "Ck Console Commands Subsystem")
class CKCONSOLECOMMANDS_API UCk_ConsoleCommands_Subsystem_UE : public UWorldSubsystem
{
    GENERATED_BODY()

    friend ACk_ConsoleCommandsHelper_UE;

public:
    CK_GENERATED_BODY(UCk_ConsoleCommands_Subsystem_UE);

public:
    auto Initialize(FSubsystemCollectionBase& Collection) -> void override;
    auto ShouldCreateSubsystem(UObject* InOuter) const -> bool override;

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

private:
    auto DoSpawnConsoleCommandsActor(APlayerController* InPlayerController) -> void;

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<ACk_ConsoleCommandsHelper_UE>> _ConsoleCommandsHelper;
};