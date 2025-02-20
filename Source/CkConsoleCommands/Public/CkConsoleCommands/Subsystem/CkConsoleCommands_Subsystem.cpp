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
    PrimaryActorTick.bCanEverTick = true;
}

auto
    ACk_ConsoleCommandsHelper_UE::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    if (IsTemplate())
    { return; }

    if (ck::Is_NOT_Valid(GetOwner()))
    {
        SetActorTickEnabled(true);
        return;
    }
    SetActorTickEnabled(false);
    CheckOwnerIsFirstController();
}

auto
    ACk_ConsoleCommandsHelper_UE::
    Tick(
        float DeltaTime)
    -> void
{
    Super::Tick(DeltaTime);

    if (ck::IsValid(GetOwner()))
    {
        SetActorTickEnabled(false);
        CheckOwnerIsFirstController();
    }
}

auto
    ACk_ConsoleCommandsHelper_UE::
    Server_Request_RunConsoleCommandOnServer_Implementation(
        const FString& InCommand)
    -> void
{
    RunConsoleCommand(InCommand);
}

auto
    ACk_ConsoleCommandsHelper_UE::
    Server_Request_RunConsoleCommandOnAll_Implementation(
        const FString& InCommand)
    -> void
{
    MC_Request_RunConsoleCommandOnAll(InCommand);
}

auto
    ACk_ConsoleCommandsHelper_UE::
    MC_Request_RunConsoleCommandOnAll_Implementation(
        const FString& InCommand)
    -> void
{
    RunConsoleCommand(InCommand);
}

auto
    ACk_ConsoleCommandsHelper_UE::
    MC_Request_OutputOnAll_Implementation(
        const FString& InCommandOutput,
        const FString& InNetModeName,
        const FString& InContext)
    -> void
{
    ck::console_commands::Log(TEXT("Console Command on the {} Output: [{}]{}"), InNetModeName, InCommandOutput, InContext);
}

auto
    ACk_ConsoleCommandsHelper_UE::
    Server_Request_OutputOnAll_Implementation(
        const FString& InCommandOutput,
        const FString& InNetModeName,
        const FString& InContext)
    -> void
{
    MC_Request_OutputOnAll(InCommandOutput, InNetModeName, InContext);
}

auto
    ACk_ConsoleCommandsHelper_UE::
    CheckOwnerIsFirstController()
    -> void
{
    if (GetWorld()->GetFirstPlayerController() != GetOwner())
    { return; }

    const auto Subsystem = GetWorld()->GetSubsystem<UCk_ConsoleCommands_Subsystem_UE>();
    Subsystem->_ConsoleCommandsHelper.Emplace(this);
}

auto
    ACk_ConsoleCommandsHelper_UE::
    RunConsoleCommand(
        const FString& InCommand)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(GetWorld()),
        TEXT("World is INVALID"))
    {
        ck::console_commands::Warning(TEXT("Could NOT run console command [{}] because the World is INVALID"), InCommand);
        return;
    }

    CK_ENSURE_IF_NOT(ck::IsValid(GetWorld()->GetFirstPlayerController()),
        TEXT("World [{}] first player controller is INVALID"))
    {
        ck::console_commands::Warning(
            TEXT("Could NOT run console command [{}] because the World [{}] FIRST PlayerController is INVALID"),
            InCommand, GetWorld());
        return;
    }

    const auto& NetModeString = HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
    const auto& ContextString = ck::Format_UE(TEXT("{}"), ck::Context(this));

    ck::console_commands::Log(TEXT("Running Console Command [{}] on the {}.{}"),
        InCommand,
        NetModeString,
        ContextString);

    constexpr auto WriteToConsole = false;
    const auto& Output = GetWorld()->GetFirstPlayerController()->ConsoleCommand(InCommand, WriteToConsole);

    Server_Request_OutputOnAll(Output, NetModeString, ContextString);
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

    _ConsoleCommand = IConsoleManager::Get().RegisterConsoleCommand
    (
        TEXT("RunConsoleCommand_OnAll"),
        TEXT("Runs console commands on all instances (Server + Clients)"),
        FConsoleCommandWithArgsDelegate::CreateUObject(this, &UCk_ConsoleCommands_Subsystem_UE::RunConsoleCommand_OnAll),
        ECVF_Default
    );

    _ConsoleCommand = IConsoleManager::Get().RegisterConsoleCommand
    (
        TEXT("RunConsoleCommand_OnServerAndOwner"),
        TEXT("Runs console commands on Server and Owning Client"),
        FConsoleCommandWithArgsDelegate::CreateUObject(this, &UCk_ConsoleCommands_Subsystem_UE::RunConsoleCommand_OnServerAndOwningClient),
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
    const auto& Command = FString::Join(InCommand, TEXT(" "));

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
        InHelper->Server_Request_RunConsoleCommandOnServer(InCommand);
    });
}

auto
    UCk_ConsoleCommands_Subsystem_UE::
    RunConsoleCommand_OnServerAndOwningClient(
        const TArray<FString>& InCommands)
    -> void
{
    const auto& Command = FString::Join(InCommands, TEXT(" "));

    Request_RunConsoleCommand_OnServerAndOwningClient(Command);
}

auto
    UCk_ConsoleCommands_Subsystem_UE::
    Request_RunConsoleCommand_OnServerAndOwningClient(
        const FString& InCommand)
    -> void
{
    ck::algo::ForEachIsValid(_ConsoleCommandsHelper, [&](const TObjectPtr<ACk_ConsoleCommandsHelper_UE> InHelper)
    {
        ck::console_commands::Log(TEXT("Running Console Command [{}] on the OWNING CLIENT"),
            InCommand);

        constexpr auto WriteToConsole = false;

        CK_ENSURE_IF_NOT(ck::IsValid(GetWorld()),
            TEXT("World is INVALID"))
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(GetWorld()->GetFirstPlayerController()),
            TEXT("World [{}] first player controller is INVALID"))
        { return; }

        const auto& Output = GetWorld()->GetFirstPlayerController()->ConsoleCommand(InCommand, WriteToConsole);

        ck::console_commands::Log(TEXT("Console Command on OWNING CLIENT Output: [{}]"), Output);

        InHelper->Server_Request_RunConsoleCommandOnServer(InCommand);
    });
}

auto
    UCk_ConsoleCommands_Subsystem_UE::
    RunConsoleCommand_OnAll(
        const TArray<FString>& InCommands)
    -> void
{
    const auto& Command = FString::Join(InCommands, TEXT(" "));

    Request_RunConsoleCommand_OnAll(Command);
}

auto
    UCk_ConsoleCommands_Subsystem_UE::
    Request_RunConsoleCommand_OnAll(
        const FString& InCommand)
    -> void
{
    ck::algo::ForEachIsValid(_ConsoleCommandsHelper, [&](const TObjectPtr<ACk_ConsoleCommandsHelper_UE> InHelper)
    {
        InHelper->Server_Request_RunConsoleCommandOnAll(InCommand);
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
