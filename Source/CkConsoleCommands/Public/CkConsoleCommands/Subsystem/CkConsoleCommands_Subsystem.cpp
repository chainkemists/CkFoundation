#include "CkConsoleCommands_Subsystem.h"

#include "CkConsoleCommands/CkConsoleCommands_Log.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Game/CkGame_Utils.h"

#include <Engine/World.h>

#include "GameFramework/GameModeBase.h"

// --------------------------------------------------------------------------------------------------------------------

ACk_ConsoleCommandsHelper_UE::
    ACk_ConsoleCommandsHelper_UE()
{
    bReplicates = true;
    bAlwaysRelevant = true;
}

auto
    ACk_ConsoleCommandsHelper_UE::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    if (IsTemplate())
    { return; }

    if (GetWorld()->GetFirstPlayerController() != GetOwner())
    { return; }

    const auto Subsystem = GetWorld()->GetSubsystem<UCk_ConsoleCommands_Subsystem_UE>();
    Subsystem->_ConsoleCommandsHelper.Emplace(this);
}

auto
    ACk_ConsoleCommandsHelper_UE::
    Server_Request_RunConsoleCommand_Implementation(
        const FString& InCommand)
    -> void
{
    ck::console_commands::Log(TEXT("Running Console Command [{}] on the SERVER.{}"), InCommand, ck::Context(this));

    constexpr auto WriteToConsole = false;
    const auto& Output = GetWorld()->GetFirstPlayerController()->ConsoleCommand(InCommand, WriteToConsole);

    MC_Request_OutputOnAll(Output);
}

auto
    ACk_ConsoleCommandsHelper_UE::
    MC_Request_OutputOnAll_Implementation(
        const FString& InCommandOutput)
    -> void
{
    ck::console_commands::Log(TEXT("Console Command on Server Output: [{}]"), InCommandOutput);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ConsoleCommands_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);

    if (NOT UCk_Utils_Game_UE::Get_IsInGame(this))
    { return; }

    // ideally we will only have 1 console command registration, however due to the many number of Worlds in the Editor
    // it is difficult to make sure that the last created game world (and subsequently this subsystem) registers the command
    DoUnregisterConsoleCommand();

    _ConsoleCommand = IConsoleManager::Get().RegisterConsoleCommand
    (
        TEXT("RunConsoleCommand_OnServer"),
        TEXT("Runs console commands on Server"),
        FConsoleCommandWithArgsDelegate::CreateUObject(this, &UCk_ConsoleCommands_Subsystem_UE::RunConsoleCommand_OnServer),
        ECVF_Default
    );

    _PostLoginDelegateHandle = FGameModeEvents::GameModePostLoginEvent.AddUObject(
        this, &UCk_ConsoleCommands_Subsystem_UE::OnNewPlayerControllerAdded);
}

auto
    UCk_ConsoleCommands_Subsystem_UE::
    Deinitialize()
    -> void
{
    FGameModeEvents::GameModePostLoginEvent.Remove(_PostLoginDelegateHandle);

    DoUnregisterConsoleCommand();
    Super::Deinitialize();
}

auto
    UCk_ConsoleCommands_Subsystem_UE::
    OnNewPlayerControllerAdded(
        AGameModeBase*,
        APlayerController* InPlayerController)
    -> void
{
    if (GetWorld()->IsNetMode(NM_Client))
    { return; }

    DoSpawnConsoleCommandsActor(InPlayerController);
}

auto
    UCk_ConsoleCommands_Subsystem_UE::
    RunConsoleCommand_OnServer(
        const TArray<FString>& InCommand)
    -> void
{
    const auto Command = FString::Join(InCommand, TEXT(" "));

    Request_RunConsoleCommand_OnServer(Command);
}

auto
    UCk_ConsoleCommands_Subsystem_UE::
    Request_RunConsoleCommand_OnServer(
        const FString& InCommand)
    -> void
{
    ck::algo::ForEachIsValid(_ConsoleCommandsHelper, [&](const TObjectPtr<ACk_ConsoleCommandsHelper_UE> InHelper)
    {
        InHelper->Server_Request_RunConsoleCommand(InCommand);
    });
}

auto
    UCk_ConsoleCommands_Subsystem_UE::
    DoSpawnConsoleCommandsActor(
        APlayerController* InPlayerController)
    -> void
{
    _ConsoleCommandsHelper.Emplace(Cast<ACk_ConsoleCommandsHelper_UE>
    (
        UCk_Utils_Actor_UE::Request_SpawnActor
        (
            FCk_Utils_Actor_SpawnActor_Params
            {
                InPlayerController,
                ACk_ConsoleCommandsHelper_UE::StaticClass()
            }
            .Set_SpawnPolicy(ECk_Utils_Actor_SpawnActorPolicy::CannotSpawnInPersistentLevel)
            .Set_NetworkingType(ECk_Actor_NetworkingType::Replicated),
            [&](AActor* InActor) { }
        )
    ));
}

auto
    UCk_ConsoleCommands_Subsystem_UE::
    DoUnregisterConsoleCommand() const
    -> void
{
    if (ck::IsValid(_ConsoleCommand, ck::IsValid_Policy_NullptrOnly{}))
    {
        IConsoleManager::Get().UnregisterConsoleObject(_ConsoleCommand);
        _ConsoleCommand = nullptr;
    }
}

// --------------------------------------------------------------------------------------------------------------------
