#include "CkActorRelay_GroupSubsystem.h"

#include "CkActorRelay_Subsystem.h"

#include "CkActorRelay/CkActorRelay_Log.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Debug/CkDebug_Utils.h"
#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

/*-----------------------------------------------------------------------------
                       VIRTUAL CONFIG DEFAULTS
------------------------------------------------------------------------------*/

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    Get_OwnershipPolicy() const
    -> ECk_ActorRelay_OwnershipPolicy
{
    return ECk_ActorRelay_OwnershipPolicy::PlayerOwned;
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    Get_ChannelCount() const
    -> int32
{
    return 1;
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    Get_MaxEntitiesPerChannel() const
    -> int32
{
    return 0;
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    Get_SelectionAlgorithm() const
    -> ECk_ActorRelay_SelectionAlgorithm
{
    return ECk_ActorRelay_SelectionAlgorithm::RoundRobin;
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    Get_DisconnectPolicy() const
    -> ECk_ActorRelay_DisconnectPolicy
{
    return ECk_ActorRelay_DisconnectPolicy::DestroyChannels;
}

/*-----------------------------------------------------------------------------
                          INITIALIZE / DEINITIALIZE
------------------------------------------------------------------------------*/

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    if (GetWorld()->IsNetMode(NM_Client))
    { return; }

    auto RelaySubsystem = GetWorld()->GetSubsystem<UCk_ActorRelay_Subsystem_UE>();

    CK_ENSURE_IF_NOT(ck::IsValid(RelaySubsystem),
        TEXT("UCk_ActorRelay_Subsystem_UE is not available during group subsystem initialization"))
    { return; }

    RelaySubsystem->DoRegisterGroup(this);

    _PostLoadMapWithWorldDelegateHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
        this, &UCk_ActorRelay_Group_Subsystem_Base_UE::OnPostLoadMapWithWorld);

    _PostLoginEventDelegateHandle = FGameModeEvents::GameModePostLoginEvent.AddUObject(
        this, &UCk_ActorRelay_Group_Subsystem_Base_UE::OnPostLoginEvent);

    _LogoutEventDelegateHandle = FGameModeEvents::GameModeLogoutEvent.AddUObject(
        this, &UCk_ActorRelay_Group_Subsystem_Base_UE::OnPlayerLogout);

    if (Get_OwnershipPolicy() == ECk_ActorRelay_OwnershipPolicy::ServerOwned)
    {
        DoSpawnChannels_Server();
    }
    else
    {
        for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            DoSpawnChannels_ForPlayer(It->Get());
        }
    }
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    Deinitialize()
    -> void
{
    Super::Deinitialize();

    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(_PostLoadMapWithWorldDelegateHandle);
    FGameModeEvents::GameModePostLoginEvent.Remove(_PostLoginEventDelegateHandle);
    FGameModeEvents::GameModeLogoutEvent.Remove(_LogoutEventDelegateHandle);

    auto RelaySubsystem = GetWorld()->GetSubsystem<UCk_ActorRelay_Subsystem_UE>();

    if (ck::IsValid(RelaySubsystem))
    {
        RelaySubsystem->DoUnregisterGroup(this);
    }
}

/*-----------------------------------------------------------------------------
                              CONSUMER API
------------------------------------------------------------------------------*/

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    Request_AcquireChannel()
    -> FCk_ActorRelay_ChannelResult
{
    CK_ENSURE_IF_NOT(Get_OwnershipPolicy() == ECk_ActorRelay_OwnershipPolicy::ServerOwned,
        TEXT("Request_AcquireChannel() called on a PlayerOwned group [{}]. Use Request_AcquireChannel_ForPlayer() instead."),
        Get_GroupTag())
    { return {}; }

    CK_ENSURE_IF_NOT(_ServerChannels.Num() > 0,
        TEXT("No server channels available for group [{}]"), Get_GroupTag())
    { return {}; }

    return DoSelectChannel_FromPool(_ServerChannels, _ServerRoundRobinIndex);
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    Request_AcquireChannel_ForPlayer(
        APlayerState* InPlayerState)
    -> FCk_ActorRelay_ChannelResult
{
    if (Get_OwnershipPolicy() == ECk_ActorRelay_OwnershipPolicy::ServerOwned)
    {
        return DoSelectChannel_FromPool(_ServerChannels, _ServerRoundRobinIndex);
    }

    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerState),
        TEXT("InPlayerState is invalid when acquiring channel for group [{}]"), Get_GroupTag())
    { return {}; }

    auto FoundPool = _PlayerChannels.Find(InPlayerState);

    CK_ENSURE_IF_NOT(FoundPool != nullptr,
        TEXT("No channels found for PlayerState [{}] in group [{}]"),
        InPlayerState->GetName(), Get_GroupTag())
    { return {}; }

    CK_ENSURE_IF_NOT(FoundPool->Num() > 0,
        TEXT("Channel pool is empty for PlayerState [{}] in group [{}]"),
        InPlayerState->GetName(), Get_GroupTag())
    { return {}; }

    auto& RoundRobinIndex = _PlayerRoundRobinIndices.FindOrAdd(InPlayerState, 0);
    return DoSelectChannel_FromPool(*FoundPool, RoundRobinIndex);
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    Request_AcquireAnyChannel()
    -> FCk_ActorRelay_ChannelResult
{
    if (Get_OwnershipPolicy() == ECk_ActorRelay_OwnershipPolicy::ServerOwned)
    {
        return DoSelectChannel_FromPool(_ServerChannels, _ServerRoundRobinIndex);
    }

    for (auto& [PlayerState, Pool] : _PlayerChannels)
    {
        if (Pool.Num() > 0)
        {
            auto& RoundRobinIndex = _PlayerRoundRobinIndices.FindOrAdd(PlayerState, 0);
            return DoSelectChannel_FromPool(Pool, RoundRobinIndex);
        }
    }

    CK_ENSURE_IF_NOT(false,
        TEXT("No channels available in any player pool for group [{}]"), Get_GroupTag())
    { return {}; }

    return {};
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    Get_ChannelCount_Active() const
    -> int32
{
    if (Get_OwnershipPolicy() == ECk_ActorRelay_OwnershipPolicy::ServerOwned)
    {
        return _ServerChannels.Num();
    }

    auto TotalCount = 0;
    for (const auto& [PlayerState, Channels] : _PlayerChannels)
    {
        TotalCount += Channels.Num();
    }
    return TotalCount;
}

/*-----------------------------------------------------------------------------
                           SELECTION LOGIC
------------------------------------------------------------------------------*/

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    DoSelectChannel_FromPool(
        TArray<TObjectPtr<ACk_ActorRelay_UE>>& InPool,
        int32& InOutRoundRobinIndex)
    -> FCk_ActorRelay_ChannelResult
{
    const auto MaxEntities = Get_MaxEntitiesPerChannel();
    const auto Algorithm = Get_SelectionAlgorithm();

    if (Algorithm == ECk_ActorRelay_SelectionAlgorithm::LeastLoaded)
    {
        auto BestChannel = static_cast<ACk_ActorRelay_UE*>(nullptr);
        auto BestCount = TNumericLimits<int32>::Max();

        for (const auto& Channel : InPool)
        {
            if (ck::Is_NOT_Valid(Channel))
            { continue; }

            const auto EntityCount = DoGetEntityCountOnChannel(Channel);

            if (MaxEntities > 0 && EntityCount >= MaxEntities)
            { continue; }

            if (EntityCount < BestCount)
            {
                BestCount = EntityCount;
                BestChannel = Channel;
            }
        }

        if (ck::Is_NOT_Valid(BestChannel))
        {
            CK_TRIGGER_ENSURE(
                TEXT("All channels at capacity for group [{}]. MaxEntitiesPerChannel=[{}], ChannelCount=[{}]"),
                Get_GroupTag(), MaxEntities, InPool.Num());
            return {};
        }

        auto ChannelEntity = UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(BestChannel);
        return FCk_ActorRelay_ChannelResult{TWeakObjectPtr<ACk_ActorRelay_UE>(BestChannel), ChannelEntity};
    }

    if (MaxEntities > 0)
    {
        const auto StartIndex = InOutRoundRobinIndex;
        auto TriedCount = 0;

        while (TriedCount < InPool.Num())
        {
            InOutRoundRobinIndex = UCk_Utils_Arithmetic_UE::Get_Increment_WithWrap(
                InOutRoundRobinIndex, FCk_IntRange{0, InPool.Num()}, ECk_Inclusiveness::Exclusive);

            auto Channel = InPool[InOutRoundRobinIndex];

            if (ck::IsValid(Channel) && DoGetEntityCountOnChannel(Channel) < MaxEntities)
            {
                auto ChannelEntity = UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(Channel);
                return FCk_ActorRelay_ChannelResult{TWeakObjectPtr<ACk_ActorRelay_UE>(Channel), ChannelEntity};
            }

            TriedCount++;
        }

        CK_TRIGGER_ENSURE(
            TEXT("All channels at capacity for group [{}]. MaxEntitiesPerChannel=[{}], ChannelCount=[{}]"),
            Get_GroupTag(), MaxEntities, InPool.Num());
        return {};
    }

    InOutRoundRobinIndex = UCk_Utils_Arithmetic_UE::Get_Increment_WithWrap(
        InOutRoundRobinIndex, FCk_IntRange{0, InPool.Num()}, ECk_Inclusiveness::Exclusive);

    auto Channel = InPool[InOutRoundRobinIndex];

    CK_ENSURE_IF_NOT(ck::IsValid(Channel),
        TEXT("Channel at index [{}] is invalid for group [{}]"),
        InOutRoundRobinIndex, Get_GroupTag())
    { return {}; }

    auto ChannelEntity = UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(Channel);
    return FCk_ActorRelay_ChannelResult{TWeakObjectPtr<ACk_ActorRelay_UE>(Channel), ChannelEntity};
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    DoGetEntityCountOnChannel(
        ACk_ActorRelay_UE* InChannelActor) const
    -> int32
{
    if (ck::Is_NOT_Valid(InChannelActor))
    { return 0; }

    auto ChannelEntity = UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(InChannelActor);

    if (ck::Is_NOT_Valid(ChannelEntity))
    { return 0; }

    return UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(ChannelEntity).Num();
}

/*-----------------------------------------------------------------------------
                          ACTOR REGISTRATION
------------------------------------------------------------------------------*/

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    DoRegisterChannelActor(
        ACk_ActorRelay_UE* InChannelActor)
    -> void
{
    if (Get_OwnershipPolicy() == ECk_ActorRelay_OwnershipPolicy::ServerOwned)
    {
        _ServerChannels.AddUnique(InChannelActor);
        return;
    }

    if (auto OwnerPlayerState = Cast<APlayerState>(InChannelActor->GetOwner());
        ck::IsValid(OwnerPlayerState))
    {
        _PlayerChannels.FindOrAdd(OwnerPlayerState).AddUnique(InChannelActor);
    }
}

/*-----------------------------------------------------------------------------
                          SPAWNING CHANNELS
------------------------------------------------------------------------------*/

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    DoSpawnChannels_Server()
    -> void
{
    const auto NumChannels = Get_ChannelCount();
    const auto ActorClass = Get_ActorClass();

    CK_ENSURE_IF_NOT(ck::IsValid(ActorClass),
        TEXT("ActorClass is invalid for ServerOwned group [{}]"), Get_GroupTag())
    { return; }

    ck::actorrelay::Log(TEXT("Spawning [{}] server channels for group [{}]"), NumChannels, Get_GroupTag());

    for (auto Index = 0; Index < NumChannels; ++Index)
    {
        [[maybe_unused]] auto Channel = Cast<ACk_ActorRelay_UE>
        (
            UCk_Utils_Actor_UE::Request_SpawnActor
            (
                FCk_Utils_Actor_SpawnActor_Params{GetWorld(), ActorClass}
                .Set_SpawnPolicy(ECk_Utils_Actor_SpawnActorPolicy::CannotSpawnInPersistentLevel)
                .Set_NetworkingType(ECk_Actor_NetworkingType::Replicated),
                [&](AActor* InActor)
                {
                    const auto NewChannel = Cast<ACk_ActorRelay_UE>(InActor);
                    NewChannel->InjectGroupSubsystemClass(this->GetClass());
                }
            )
        );
    }
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    DoSpawnChannels_ForPlayer(
        APlayerController* InPlayerController)
    -> void
{
    auto AlreadyRegistered = false;
    _RegisteredPlayerControllers.Add(InPlayerController, &AlreadyRegistered);

    if (AlreadyRegistered)
    { return; }

    const auto NumChannels = Get_ChannelCount();
    const auto ActorClass = Get_ActorClass();

    CK_ENSURE_IF_NOT(ck::IsValid(ActorClass),
        TEXT("ActorClass is invalid for PlayerOwned group [{}]"), Get_GroupTag())
    { return; }

    ck::actorrelay::Log(TEXT("Spawning [{}] player channels for PlayerController [{}] in group [{}]"),
        NumChannels, InPlayerController->GetName(), Get_GroupTag());

    for (auto Index = 0; Index < NumChannels; ++Index)
    {
        [[maybe_unused]] auto Channel = Cast<ACk_ActorRelay_UE>
        (
            UCk_Utils_Actor_UE::Request_SpawnActor
            (
                FCk_Utils_Actor_SpawnActor_Params{GetWorld(), ActorClass}
                .Set_SpawnPolicy(ECk_Utils_Actor_SpawnActorPolicy::CannotSpawnInPersistentLevel)
                .Set_NetworkingType(ECk_Actor_NetworkingType::Replicated),
                [&](AActor* InActor)
                {
                    const auto NewChannel = Cast<ACk_ActorRelay_UE>(InActor);
                    NewChannel->InjectGroupSubsystemClass(this->GetClass());

                    if (const auto PlayerState = InPlayerController->PlayerState;
                        ck::IsValid(PlayerState))
                    {
                        NewChannel->SetOwner(PlayerState);
                    }
                }
            )
        );
    }
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    DoDestroyChannels_ForPlayer(
        APlayerState* InPlayerState)
    -> void
{
    auto FoundPool = _PlayerChannels.Find(InPlayerState);

    if (FoundPool == nullptr)
    { return; }

    ck::actorrelay::Log(TEXT("Destroying [{}] channels for PlayerState [{}] in group [{}]"),
        FoundPool->Num(), InPlayerState->GetName(), Get_GroupTag());

    for (const auto& Channel : *FoundPool)
    {
        if (ck::Is_NOT_Valid(Channel))
        { continue; }

        auto ChannelEntity = UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(Channel);

        if (ck::IsValid(ChannelEntity))
        {
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(ChannelEntity);
        }

        Channel->Destroy();
    }

    _PlayerChannels.Remove(InPlayerState);
    _PlayerRoundRobinIndices.Remove(InPlayerState);
}

/*-----------------------------------------------------------------------------
                          LIFECYCLE EVENTS
------------------------------------------------------------------------------*/

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    OnPostLoadMapWithWorld(
        UWorld* InWorld)
    -> void
{
    if (ck::Is_NOT_Valid(InWorld))
    { return; }

    if (GetWorld()->IsNetMode(NM_Client))
    { return; }

    ck::actorrelay::Log(TEXT("OnPostLoadMapWithWorld: Cleaning up channels for group [{}]"), Get_GroupTag());

    _ServerRoundRobinIndex = 0;
    _PlayerRoundRobinIndices.Reset();

    for (const auto& ValidPCList = _RegisteredPlayerControllers.Array();
         const auto& PC : ValidPCList)
    {
        if (ck::IsValid(PC) && PC->GetWorld() == InWorld)
        { continue; }

        _RegisteredPlayerControllers.Remove(PC);

        if (ck::IsValid(PC) && ck::IsValid(PC->PlayerState))
        {
            _PlayerChannels.Remove(PC->PlayerState);
            _PlayerRoundRobinIndices.Remove(PC->PlayerState);
        }
    }

    _ServerChannels = ck::algo::Filter(_ServerChannels, [&](const ACk_ActorRelay_UE* InChannel)
    {
        return ck::IsValid(InChannel) && InChannel->GetWorld() == InWorld;
    });

    if (Get_OwnershipPolicy() == ECk_ActorRelay_OwnershipPolicy::ServerOwned)
    {
        if (_ServerChannels.Num() == 0)
        {
            DoSpawnChannels_Server();
        }
    }
    else
    {
        for (auto It = InWorld->GetPlayerControllerIterator(); It; ++It)
        {
            DoSpawnChannels_ForPlayer(It->Get());
        }
    }
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    OnPostLoginEvent(
        AGameModeBase* InGameMode,
        APlayerController* InNewPlayer)
    -> void
{
    if (Get_OwnershipPolicy() == ECk_ActorRelay_OwnershipPolicy::ServerOwned)
    { return; }

    if (NOT _RegisteredPlayerControllers.Contains(InNewPlayer))
    {
        ck::actorrelay::Log(TEXT("OnPostLoginEvent: Spawning channels for new player [{}] in group [{}]"),
            InNewPlayer->GetName(), Get_GroupTag());

        DoSpawnChannels_ForPlayer(InNewPlayer);
    }
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    OnPlayerLogout(
        AGameModeBase* InGameMode,
        AController* InController)
    -> void
{
    if (Get_OwnershipPolicy() == ECk_ActorRelay_OwnershipPolicy::ServerOwned)
    { return; }

    auto PlayerController = Cast<APlayerController>(InController);

    if (ck::Is_NOT_Valid(PlayerController))
    { return; }

    _RegisteredPlayerControllers.Remove(PlayerController);

    auto PlayerState = PlayerController->PlayerState;

    if (ck::Is_NOT_Valid(PlayerState))
    { return; }

    if (Get_DisconnectPolicy() == ECk_ActorRelay_DisconnectPolicy::DestroyChannels)
    {
        ck::actorrelay::Log(TEXT("OnPlayerLogout: Destroying channels for player [{}] in group [{}]"),
            PlayerController->GetName(), Get_GroupTag());

        DoDestroyChannels_ForPlayer(PlayerState);
    }
}

// --------------------------------------------------------------------------------------------------------------------
