// CkCueExecutor_Subsystem.cpp
//
// UCk_CueExecutor_Subsystem_Base_UE implementation:
//   - Initialize / Deinitialize
//   - Request_ExecuteCue* methods (transient, local, replicated)
//   - Player controller / executor actor management
//   - Pending cue queue and timeout handling

#include "CkCueSubsystem_Base.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"
#include "CkCore/Debug/CkDebug_Utils.h"
#include "CkCore/IO/CkIO_Utils.h"

#include "CkCue/CkCue_Fragment.h"
#include "CkCue/CkCue_Log.h"
#include "CkCue/Settings/CkCue_Settings.h"
#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkEntityBridge/Public/CkEntityBridge/CkEntityBridge_ConstructionScript.h"

#include <Net/UnrealNetwork.h>
#include <Net/Core/PushModel/PushModel.h>

/*─────────────────────────────────────────────────────────────────────────────┐
│                         CUE EXECUTOR SUBSYSTEM BASE                          │
└─────────────────────────────────────────────────────────────────────────────*/

namespace ck_cue_subsystem_base
{
    auto Get_CueSubsystemFromClass(TSubclassOf<UCk_CueSubsystem_Base_UE> InCueSubsystemClass) -> UCk_CueSubsystem_Base_UE*;
    auto ExecuteCueEntityScript(FCk_Handle InOwnerEntity, const FGameplayTag& InCueName, TSubclassOf<UCk_CueBase_EntityScript> InCueClass, const FInstancedStruct& InSpawnParams) -> FCk_Handle_PendingEntityScript;
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    if (GetWorld()->IsNetMode(NM_Client))
    { return; }

    _PostLoadMapWithWorldDelegateHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UCk_CueExecutor_Subsystem_Base_UE::OnPostLoadMapWithWorld);
    _PostLoginEventDelegateHandle = FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UCk_CueExecutor_Subsystem_Base_UE::OnPostLoginEvent);
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    Deinitialize()
    -> void
{
    Super::Deinitialize();

    if (_PendingCueTimeoutTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(_PendingCueTimeoutTickerHandle);
        _PendingCueTimeoutTickerHandle.Reset();
    }

    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(_PostLoadMapWithWorldDelegateHandle);
    FGameModeEvents::GameModePostLoginEvent.Remove(_PostLoginEventDelegateHandle);
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    Request_ExecuteCue_Transient(
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams,
        ECk_Cue_ReliabilityPolicy InReliability,
        ECk_Cue_MulticastPolicy InMulticastPolicy,
        ECk_Cue_ExecutionPolicy InExecutionPolicy)
    -> FCk_Handle_PendingEntityScript
{
    if (_CueExecutors.Num() == 0)
    {
        ck::cue::Warning(TEXT("No CueExecutor actors available yet. Caching transient cue [{}] for later execution"), InCueName);

        FCk_Handle InvalidHandle{};
        _PendingCues.Emplace(InvalidHandle, InCueName, InSpawnParams, InReliability, InMulticastPolicy, InExecutionPolicy);

        if (NOT _PendingCueTimeoutTickerHandle.IsValid())
        {
            _PendingCueTimeoutTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
                FTickerDelegate::CreateUObject(this, &UCk_CueExecutor_Subsystem_Base_UE::DoCheckPendingCueTimeout)
            );
        }

        return {};
    }

    auto CueExecutor = _CueExecutors[_NextAvailableExecutor];

    CK_ENSURE_IF_NOT(ck::IsValid(CueExecutor),
        TEXT("Next Available Cue Executor Actor at Index [{}] is INVALID"), _NextAvailableExecutor)
    { return {}; }

    auto CueExecutorEntity = UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(CueExecutor);
    return Request_ExecuteCue(CueExecutorEntity, InCueName, InSpawnParams, InReliability, InMulticastPolicy, InExecutionPolicy);
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    Request_ExecuteCue_Transient_Local(
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams)
    -> FCk_Handle_PendingEntityScript
{
    CK_ENSURE_IF_NOT(_CueExecutors.Num() > 0,
        TEXT("No CueExecutor Actors available. Unable to Execute Cue"))
    { return {}; }

    auto CueExecutor = _CueExecutors[_NextAvailableExecutor];

    CK_ENSURE_IF_NOT(ck::IsValid(CueExecutor),
        TEXT("Next Available Cue Executor Actor at Index [{}] is INVALID"), _NextAvailableExecutor)
    { return {}; }

    auto CueExecutorEntity = UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(CueExecutor);
    return Request_ExecuteCue_Local(CueExecutorEntity, InCueName, InSpawnParams);
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    Request_ExecuteCue(
        const FCk_Handle& InOwnerEntity,
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams,
        ECk_Cue_ReliabilityPolicy InReliability,
        ECk_Cue_MulticastPolicy InMulticastPolicy,
        ECk_Cue_ExecutionPolicy InExecutionPolicy)
    -> FCk_Handle_PendingEntityScript
{
    if (ck::Is_NOT_Valid(InOwnerEntity))
    {
        ck::cue::Verbose(TEXT("OwnerEntity is invalid when trying to execute Cue [{}]. Deferring to ExecuteCueEntityScript for policy check."), InCueName);
        return {};
    }

    if (InExecutionPolicy == ECk_Cue_ExecutionPolicy::Local)
    {
        return Request_ExecuteCue_Local(InOwnerEntity, InCueName, InSpawnParams);
    }

    if (InExecutionPolicy == ECk_Cue_ExecutionPolicy::ReplicatedAndLocal)
    {
        Request_ExecuteCue_Local(InOwnerEntity, InCueName, InSpawnParams);
    }

    if (_CueExecutors.Num() == 0)
    {
        ck::cue::Warning(TEXT("No CueExecutor actors available yet. Caching cue [{}] for later execution"), InCueName);

        _PendingCues.Emplace(InOwnerEntity, InCueName, InSpawnParams, InReliability, InMulticastPolicy, InExecutionPolicy);

        if (NOT _PendingCueTimeoutTickerHandle.IsValid())
        {
            _PendingCueTimeoutTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
                FTickerDelegate::CreateUObject(this, &UCk_CueExecutor_Subsystem_Base_UE::DoCheckPendingCueTimeout)
            );
        }

        return {};
    }

    if (GetWorld()->IsNetMode(NM_Standalone))
    {
        const auto& CueSubsystemClass = Get_CueSubsystemClass();
        auto CueSubsystem = ck_cue_subsystem_base::Get_CueSubsystemFromClass(CueSubsystemClass);
        CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
            TEXT("CueSubsystem is invalid in standalone mode"))
        { return {}; }

        const auto& CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
        return ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
    }

    _NextAvailableExecutor = UCk_Utils_Arithmetic_UE::Get_Increment_WithWrap(
        _NextAvailableExecutor, FCk_IntRange{0, _CueExecutors.Num()}, ECk_Inclusiveness::Exclusive);

    auto CueExecutor = _CueExecutors[_NextAvailableExecutor];
    CK_ENSURE_IF_NOT(ck::IsValid(CueExecutor),
        TEXT("Next Available Cue Executor Actor at Index [{}] is INVALID"), _NextAvailableExecutor)
    { return {}; }

    if (GetWorld()->IsNetMode(NM_Client))
    {
        auto LocalPC = GetWorld()->GetFirstPlayerController();
        if (ck::Is_NOT_Valid(LocalPC))
        {
            ck::cue::Warning(TEXT("Failed to execute cue [{}]: Local PlayerController is invalid"), InCueName);
            return {};
        }

        auto LocalPlayerState = LocalPC->PlayerState;
        if (ck::Is_NOT_Valid(LocalPlayerState))
        {
            ck::cue::Warning(TEXT("Failed to execute cue [{}]: Local PlayerState is invalid"), InCueName);
            return {};
        }

        auto ClientExecutor = _ExecutorsByPlayerState.FindRef(LocalPlayerState).Get();
        if (ck::Is_NOT_Valid(ClientExecutor))
        {
            ck::cue::Warning(TEXT("Failed to execute cue [{}]: Client executor not found for PlayerState"), InCueName);
            return {};
        }

        if (InMulticastPolicy == ECk_Cue_MulticastPolicy::ServerOnly ||
            InMulticastPolicy == ECk_Cue_MulticastPolicy::ServerAndSelf)
        {
            if (InMulticastPolicy == ECk_Cue_MulticastPolicy::ServerAndSelf)
            {
                Request_ExecuteCue_Local(InOwnerEntity, InCueName, InSpawnParams);
            }

            if (InReliability == ECk_Cue_ReliabilityPolicy::Reliable)
            {
                ClientExecutor->Server_RequestExecuteCue_ServerOnly_Reliable(InOwnerEntity, InCueName, InSpawnParams);
            }
            else
            {
                ClientExecutor->Server_RequestExecuteCue_ServerOnly(InOwnerEntity, InCueName, InSpawnParams);
            }
        }
        else if (InMulticastPolicy == ECk_Cue_MulticastPolicy::MulticastToOtherClients)
        {
            if (InReliability == ECk_Cue_ReliabilityPolicy::Reliable)
            {
                ClientExecutor->Server_RequestExecuteCue_ExcludingSender_Reliable(InOwnerEntity, InCueName, InSpawnParams);
            }
            else
            {
                ClientExecutor->Server_RequestExecuteCue_ExcludingSender(InOwnerEntity, InCueName, InSpawnParams);
            }
        }
        else
        {
            if (InReliability == ECk_Cue_ReliabilityPolicy::Reliable)
            {
                ClientExecutor->Server_RequestExecuteCue_Reliable(InOwnerEntity, InCueName, InSpawnParams);
            }
            else
            {
                ClientExecutor->Server_RequestExecuteCue(InOwnerEntity, InCueName, InSpawnParams);
            }
        }
        return {};
    }

    if (GetWorld()->IsNetMode(NM_DedicatedServer) || GetWorld()->IsNetMode(NM_ListenServer))
    {
        if (InMulticastPolicy == ECk_Cue_MulticastPolicy::ServerOnly ||
            InMulticastPolicy == ECk_Cue_MulticastPolicy::ServerAndSelf)
        {
            ck::cue::Verbose(TEXT("Executing server-only cue [{}] on entity [{}]"), InCueName, InOwnerEntity);
            const auto& CueSubsystemClass = Get_CueSubsystemClass();
            auto CueSubsystem = ck_cue_subsystem_base::Get_CueSubsystemFromClass(CueSubsystemClass);
            CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
                TEXT("CueSubsystem is invalid for server-only cue execution"))
            { return {}; }

            const auto& CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
            return ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
        }

        if (InMulticastPolicy == ECk_Cue_MulticastPolicy::MulticastToOtherClients)
        {
            auto OriginatingPlayerState = Cast<APlayerState>(CueExecutor->GetOwner());

            if (InReliability == ECk_Cue_ReliabilityPolicy::Reliable)
            {
                CueExecutor->Request_ExecuteCue_ExcludingSender_Reliable(InOwnerEntity, InCueName, InSpawnParams, OriginatingPlayerState);
            }
            else
            {
                CueExecutor->Request_ExecuteCue_ExcludingSender(InOwnerEntity, InCueName, InSpawnParams, OriginatingPlayerState);
            }
        }
        else
        {
            if (InReliability == ECk_Cue_ReliabilityPolicy::Reliable)
            {
                CueExecutor->Request_ExecuteCue_Reliable(InOwnerEntity, InCueName, InSpawnParams);
            }
            else
            {
                CueExecutor->Request_ExecuteCue(InOwnerEntity, InCueName, InSpawnParams);
            }
        }
    }

    return {};
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    Request_ExecuteCue_Local(
        const FCk_Handle& InOwnerEntity,
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams)
    -> FCk_Handle_PendingEntityScript
{
    const auto& CueSubsystemClass = Get_CueSubsystemClass();
    auto CueSubsystem = ck_cue_subsystem_base::Get_CueSubsystemFromClass(CueSubsystemClass);
    CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
        TEXT("CueSubsystem is invalid for local cue execution"))
    { return {}; }

    const auto& CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
    return ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    DoSpawnCueExecutorActorsForPlayerController(
        APlayerController* InPlayerController)
    -> void
{
    auto AlreadyContainsPC = false;
    _ValidPlayerControllers.Add(InPlayerController, &AlreadyContainsPC);

    if (AlreadyContainsPC)
    { return; }

    ck::cue::Log(TEXT("Spawning CueExecutor actor for PlayerController [{}]"), InPlayerController->GetName());

    [[maybe_unused]] auto CueExecutor = Cast<ACk_CueExecutor_UE>
    (
        UCk_Utils_Actor_UE::Request_SpawnActor
        (
            FCk_Utils_Actor_SpawnActor_Params{GetWorld(), ACk_CueExecutor_UE::StaticClass()}
            .Set_SpawnPolicy(ECk_Utils_Actor_SpawnActorPolicy::CannotSpawnInPersistentLevel)
            .Set_NetworkingType(ECk_Actor_NetworkingType::Replicated),
            [&](AActor* InActor)
            {
                const auto& NewCueExecutor = Cast<ACk_CueExecutor_UE>(InActor);
                NewCueExecutor->InjectCueExecutorSubsystemClass(this->GetClass());
                if (const auto PlayerState = InPlayerController->PlayerState;
                    ck::IsValid(PlayerState))
                {
                    NewCueExecutor->SetOwner(PlayerState);
                }
            }
        )
    );

    DoProcessPendingCues();
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    OnPostLoadMapWithWorld(
        UWorld* InWorld)
    -> void
{
    // NOTE: If Seamless Travel is enabled this (World) Subsystem will not be torn-down, but any spawned CueReplicator Actors will be destroyed.
    // Instead of adding the CueReplicator Actors to the list of actors that persist through the travel, we re-create them once the new world is loaded.
    // 'OnSwapPlayerControllers' from the GameMode is called before we enter this function, which means all available PC are the new ones created for
    // the world we just traveled to.

    if (ck::Is_NOT_Valid(InWorld))
    { return; }

    if (GetWorld()->IsNetMode(NM_Client))
    { return; }

    ck::cue::Log(TEXT("OnPostLoadMapWithWorld: Cleaning up executors for world [{}]"), InWorld->GetName());

    _NextAvailableExecutor = 0;

    for (const auto& ValidPlayerControllersList = _ValidPlayerControllers.Array();
         const auto& PC : ValidPlayerControllersList)
    {
        if (ck::IsValid(PC) && PC->GetWorld() == InWorld)
        { continue; }

        _ValidPlayerControllers.Remove(PC);

        if (ck::IsValid(PC) && ck::IsValid(PC->PlayerState))
        {
            _ExecutorsByPlayerState.Remove(PC->PlayerState);
        }

        _CueExecutors = ck::algo::Filter(_CueExecutors, [&](const ACk_CueExecutor_UE* InCueExecutor)
        {
            if (ck::Is_NOT_Valid(InCueExecutor))
            { return false; }

            if (ck::Is_NOT_Valid(PC))
            { return true; }

            return InCueExecutor->GetWorld() == PC->GetWorld();
        });
    }

    for (auto It = InWorld->GetPlayerControllerIterator(); It; ++It)
    {
       DoSpawnCueExecutorActorsForPlayerController(It->Get());
    }
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    OnPostLoginEvent(
        AGameModeBase* GameMode,
        APlayerController* NewPlayer)
    -> void
{
    if (NOT _ValidPlayerControllers.Contains(NewPlayer))
    {
        ck::cue::Log(TEXT("OnPostLoginEvent: Spawning executor for new player [{}]"), NewPlayer->GetName());
        DoSpawnCueExecutorActorsForPlayerController(NewPlayer);
    }
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    DoProcessPendingCues()
    -> void
{
    if (_PendingCues.IsEmpty())
    { return; }

    if (_CueExecutors.Num() == 0)
    { return; }

    ck::cue::Log(TEXT("Processing [{}] pending cues"), _PendingCues.Num());

    auto PendingCuesToProcess = MoveTemp(_PendingCues);
    _PendingCues.Reset();

    if (_PendingCueTimeoutTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(_PendingCueTimeoutTickerHandle);
        _PendingCueTimeoutTickerHandle.Reset();
    }

    for (const auto& PendingCue : PendingCuesToProcess)
    {
        ck::cue::Verbose(TEXT("Executing cached cue [{}]"), PendingCue.CueName);

        if (ck::Is_NOT_Valid(PendingCue.OwnerEntity))
        {
            Request_ExecuteCue_Transient(
                PendingCue.CueName,
                PendingCue.SpawnParams,
                PendingCue.Reliability,
                PendingCue.MulticastPolicy,
                PendingCue.ExecutionPolicy
            );
        }
        else
        {
            Request_ExecuteCue(
                PendingCue.OwnerEntity,
                PendingCue.CueName,
                PendingCue.SpawnParams,
                PendingCue.Reliability,
                PendingCue.MulticastPolicy,
                PendingCue.ExecutionPolicy
            );
        }
    }
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    DoCheckPendingCueTimeout(
        float InDeltaTime)
    -> bool
{
    DoProcessPendingCues();

    if (_PendingCues.IsEmpty())
    {
        _PendingCueTimeoutTickerHandle.Reset();
        return false;
    }

    const auto TimeoutSeconds = UCk_Utils_Cue_Settings_UE::Get_PendingCueTimeoutSeconds();
    const auto CurrentTime = FPlatformTime::Seconds();

    for (const auto& PendingCue : _PendingCues)
    {
        const auto ElapsedTime = CurrentTime - PendingCue.TimeRequested;

        if (ElapsedTime >= TimeoutSeconds)
        {
            CK_TRIGGER_ENSURE(TEXT("Pending cue [{}] has been waiting for [{}] seconds without executors becoming available. Timeout threshold: [{}]s"),
                PendingCue.CueName,
                ElapsedTime,
                TimeoutSeconds);

            return true;
        }
    }

    return true;
}
