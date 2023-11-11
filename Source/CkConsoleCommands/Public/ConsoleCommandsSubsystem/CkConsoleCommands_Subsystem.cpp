#include "CkConsoleCommands_Subsystem.h"

#include "CkConsoleCommands/CkConsoleCommands_Log.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "Engine/World.h"

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
    Initialize(FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);

    if (NOT UCk_Utils_Game_UE::Get_IsInGame(this))
    { return; }

    IConsoleManager::Get().RegisterConsoleCommand
    (
        TEXT("RunConsoleCommand_OnServer"),
        TEXT("Runs console commands on Server"),
        FConsoleCommandWithArgsDelegate::CreateUObject(this, &UCk_ConsoleCommands_Subsystem_UE::RunConsoleCommand_OnServer),
        ECVF_Default
    );

    FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UCk_ConsoleCommands_Subsystem_UE::OnNewPlayerControllerAdded);
}

auto
    UCk_ConsoleCommands_Subsystem_UE::
    ShouldCreateSubsystem(
        UObject* InOuter) const
    -> bool
{
    const auto& ShouldCreateSubsystem = Super::ShouldCreateSubsystem(InOuter);

    if (NOT ShouldCreateSubsystem)
    { return false; }

    if (ck::Is_NOT_Valid(InOuter))
    { return true; }

    const auto& World = InOuter->GetWorld();

    if (ck::Is_NOT_Valid(World))
    { return true; }

    return DoesSupportWorldType(World->WorldType);
}

void
    UCk_ConsoleCommands_Subsystem_UE::
    OnNewPlayerControllerAdded(
        AGameModeBase*,
        APlayerController* InPlayerController)
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
    auto Command = FString{};
    for (const auto& CommandPiece : InCommand)
    {
        Command += " ";
        Command += CommandPiece;
    }
    Request_RunConsoleCommand_OnServer(Command);
}

auto
    UCk_ConsoleCommands_Subsystem_UE::
    Request_RunConsoleCommand_OnServer(
        const FString& InCommand) -> void
{
    for (const auto Helper : _ConsoleCommandsHelper)
    {
        Helper->Server_Request_RunConsoleCommand(InCommand);
    }
}

auto
    UCk_ConsoleCommands_Subsystem_UE::
    DoSpawnConsoleCommandsActor(APlayerController* InPlayerController)
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
