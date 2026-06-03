#include "CkActorRelay_GroupSubsystem.h"

#include "CkActorRelay_Subsystem.h"

#include "CkActorRelay/CkActorRelay_Log.h"
#include "CkActorRelay/Settings/CkActorRelay_Settings.h"

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

    InCollection.InitializeDependency<UCk_ActorRelay_Subsystem_UE>();

    auto RelaySubsystem = GetWorld()->GetSubsystem<UCk_ActorRelay_Subsystem_UE>();

    CK_ENSURE_IF_NOT(ck::IsValid(RelaySubsystem),
        TEXT("UCk_ActorRelay_Subsystem_UE is not available during group subsystem initialization"))
    { return; }

    RelaySubsystem->DoRegisterGroup(this);

    if (GetWorld()->IsNetMode(NM_Client))
    { return; }

    _PostLoadMapWithWorldDelegateHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
        this, &UCk_ActorRelay_Group_Subsystem_Base_UE::OnPostLoadMapWithWorld);

    _PostLoginEventDelegateHandle = FGameModeEvents::GameModePostLoginEvent.AddUObject(
        this, &UCk_ActorRelay_Group_Subsystem_Base_UE::OnPostLoginEvent);

    _LogoutEventDelegateHandle = FGameModeEvents::GameModeLogoutEvent.AddUObject(
        this, &UCk_ActorRelay_Group_Subsystem_Base_UE::OnPlayerLogout);

    // Channel spawning is deferred to OnWorldBeginPlay. Spawning replicated
    // actors before UWorld::HasBegunPlay() returns true makes UE classify them
    // as level-startup actors (bNetStartup=true), which causes Iris to
    // serialize them to clients by path reference instead of by class. The
    // client then tries to resolve the path in its level package, finds
    // nothing (these actors are dynamically spawned and don't exist on disk),
    // and "Could not find static actor" / "Failed to instantiate Handle"
    // ensures fire in UObjectReplicationBridge.
    //
    // If the subsystem is created mid-game (rare — seamless travel etc.) the
    // world has already begun play, so spawn immediately; OnWorldBeginPlay
    // will not fire for an already-begun-play world.
    if (GetWorld()->HasBegunPlay())
    {
        DoSpawnChannels();
    }
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    OnWorldBeginPlay(
        UWorld& InWorld)
    -> void
{
    Super::OnWorldBeginPlay(InWorld);

    if (InWorld.IsNetMode(NM_Client))
    { return; }

    DoSpawnChannels();
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    DoSpawnChannels()
    -> void
{
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

    if (auto RelaySubsystem = GetWorld()->GetSubsystem<UCk_ActorRelay_Subsystem_UE>();
        ck::IsValid(RelaySubsystem))
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
    -> FCk_Handle_PendingActorRelay
{
    CK_ENSURE_IF_NOT(Get_OwnershipPolicy() == ECk_ActorRelay_OwnershipPolicy::ServerOwned,
        TEXT("Request_AcquireChannel() called on a PlayerOwned group [{}]. Use Request_AcquireChannel_ForPlayer() instead."),
        Get_GroupTag())
    { return {}; }

    auto Pending = FCk_Handle_PendingActorRelay{};
    Pending._GroupSubsystem = this;
    Pending._Kind = ECk_ActorRelay_AcquireKind::Server;
    return Pending;
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    Request_AcquireChannel_ForPlayer(
        APlayerState* InPlayerState)
    -> FCk_Handle_PendingActorRelay
{
    auto Pending = FCk_Handle_PendingActorRelay{};
    Pending._GroupSubsystem = this;

    if (Get_OwnershipPolicy() == ECk_ActorRelay_OwnershipPolicy::ServerOwned)
    {
        Pending._Kind = ECk_ActorRelay_AcquireKind::Server;
        return Pending;
    }

    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerState),
        TEXT("InPlayerState is invalid when acquiring channel for group [{}]"), Get_GroupTag())
    { return {}; }

    Pending._Kind = ECk_ActorRelay_AcquireKind::ForPlayer;
    Pending._PlayerState = InPlayerState;
    return Pending;
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    Request_AcquireAnyChannel()
    -> FCk_Handle_PendingActorRelay
{
    auto Pending = FCk_Handle_PendingActorRelay{};
    Pending._GroupSubsystem = this;

    if (Get_OwnershipPolicy() == ECk_ActorRelay_OwnershipPolicy::ServerOwned)
    {
        Pending._Kind = ECk_ActorRelay_AcquireKind::Server;
        return Pending;
    }

    Pending._Kind = ECk_ActorRelay_AcquireKind::Any;
    return Pending;
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    DoBroadcastChannelReadyChanged()
    -> void
{
    _OnChannelReadyChanged.Broadcast();
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    Try_ResolvePending(
        FCk_Handle_PendingActorRelay& InPending)
    -> FCk_ActorRelay_ChannelResult
{
    // Thin public forwarder so consumers that want sync-or-null (e.g. ECS processors with
    // per-tick retry) don't have to subscribe to _OnChannelReadyChanged via Promise_OnAcquired.
    // Internal callers still use DoTryResolve directly via friendship.
    return DoTryResolve(InPending);
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    DoTryResolve(
        FCk_Handle_PendingActorRelay& InPending)
    -> FCk_ActorRelay_ChannelResult
{
    switch (InPending._Kind)
    {
        case ECk_ActorRelay_AcquireKind::Server:
        {
            if (_ServerChannels.Num() == 0)
            { return {}; }

            auto Result = DoSelectChannel_FromPool(_ServerChannels, _ServerRoundRobinIndex);

            if (ck::Is_NOT_Valid(Result))
            {
                DoMaybeGrowPool(_ServerChannels, nullptr);
            }

            return Result;
        }
        case ECk_ActorRelay_AcquireKind::ForPlayer:
        {
            auto PlayerState = InPending._PlayerState.Get();
            if (ck::Is_NOT_Valid(PlayerState))
            { return {}; }

            auto FoundPool = _PlayerChannels.Find(PlayerState);
            if (NOT FoundPool || FoundPool->Num() == 0)
            { return {}; }

            auto& RoundRobinIndex = _PlayerRoundRobinIndices.FindOrAdd(PlayerState, 0);
            auto Result = DoSelectChannel_FromPool(*FoundPool, RoundRobinIndex);

            if (ck::Is_NOT_Valid(Result))
            {
                DoMaybeGrowPool(*FoundPool, PlayerState);
            }

            return Result;
        }
        case ECk_ActorRelay_AcquireKind::Any:
        {
            for (auto& [PlayerState, Pool] : _PlayerChannels)
            {
                if (Pool.Num() > 0)
                {
                    auto& RoundRobinIndex = _PlayerRoundRobinIndices.FindOrAdd(PlayerState, 0);
                    if (auto Result = DoSelectChannel_FromPool(Pool, RoundRobinIndex);
                        ck::IsValid(Result))
                    { return Result; }
                }
            }
            return {};
        }
    }

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
        int32& InOutRoundRobinIndex) const
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

            if (NOT UCk_Utils_OwningActor_UE::Get_IsActorEcsReady(Channel))
            { continue; }

            const auto EntityCount = DoGet_EntityCountOnChannel(Channel);

            if (MaxEntities > 0 && EntityCount >= MaxEntities)
            { continue; }

            if (EntityCount < BestCount)
            {
                BestCount = EntityCount;
                BestChannel = Channel;
            }
        }

        if (ck::Is_NOT_Valid(BestChannel))
        { return {}; }

        const auto ChannelEntity = UCk_Utils_OwningActor_UE::TryGet_ActorEntityHandle(BestChannel);
        return FCk_ActorRelay_ChannelResult{TWeakObjectPtr<ACk_ActorRelay_UE>(BestChannel), ChannelEntity};
    }

    if (MaxEntities > 0)
    {
        auto TriedCount = 0;

        while (TriedCount < InPool.Num())
        {
            InOutRoundRobinIndex = UCk_Utils_Arithmetic_UE::Get_Increment_WithWrap(
                InOutRoundRobinIndex, FCk_IntRange{0, InPool.Num()}, ECk_Inclusiveness::Exclusive);

            auto Channel = InPool[InOutRoundRobinIndex];

            if (ck::IsValid(Channel) &&
                UCk_Utils_OwningActor_UE::Get_IsActorEcsReady(Channel) &&
                DoGet_EntityCountOnChannel(Channel) < MaxEntities)
            {
                const auto ChannelEntity = UCk_Utils_OwningActor_UE::TryGet_ActorEntityHandle(Channel);
                return FCk_ActorRelay_ChannelResult{TWeakObjectPtr<ACk_ActorRelay_UE>(Channel), ChannelEntity};
            }

            TriedCount++;
        }

        return {};
    }

    auto TriedCount = 0;

    while (TriedCount < InPool.Num())
    {
        InOutRoundRobinIndex = UCk_Utils_Arithmetic_UE::Get_Increment_WithWrap(
            InOutRoundRobinIndex, FCk_IntRange{0, InPool.Num()}, ECk_Inclusiveness::Exclusive);

        if (auto Channel = InPool[InOutRoundRobinIndex];
            ck::IsValid(Channel) && UCk_Utils_OwningActor_UE::Get_IsActorEcsReady(Channel))
        {
            const auto ChannelEntity = UCk_Utils_OwningActor_UE::TryGet_ActorEntityHandle(Channel);
            return FCk_ActorRelay_ChannelResult{TWeakObjectPtr<ACk_ActorRelay_UE>(Channel), ChannelEntity};
        }

        TriedCount++;
    }

    return {};
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    DoGet_EntityCountOnChannel(
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

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    DoMaybeGrowPool(
        TArray<TObjectPtr<ACk_ActorRelay_UE>>& InPool,
        APlayerState* InOwnerPlayerState)
    -> void
{
    // Channel spawning is server-authoritative; never grow on a client.
    if (GetWorld()->IsNetMode(NM_Client))
    { return; }

    const auto MaxEntities = Get_MaxEntitiesPerChannel();

    // Unlimited-capacity groups (e.g. Generic, MaxEntities == 0) host everything on a single
    // channel, so there is no saturation to react to — the warm pool is their steady state.
    if (MaxEntities <= 0)
    { return; }

    if (InPool.Num() >= Get_ChannelCount())
    { return; }

    // Only grow once EVERY channel is both ECS-ready and full. A not-yet-ready channel means a
    // previous grow (or the warm pool) is still coming online — wait for it rather than spawning
    // a burst of channels, which would re-create the very Iris first-packet pressure this
    // lazy-spawn change exists to avoid.
    for (const auto& Channel : InPool)
    {
        const auto IsReadyAndFull =
            ck::IsValid(Channel) &&
            UCk_Utils_OwningActor_UE::Get_IsActorEcsReady(Channel) &&
            DoGet_EntityCountOnChannel(Channel) >= MaxEntities;

        if (NOT IsReadyAndFull)
        { return; }
    }

    ck::actorrelay::Log(TEXT("Growing pool for group [{}] to [{}] channels (capacity-driven)"),
        Get_GroupTag(), InPool.Num() + 1);

    if (ck::IsValid(InOwnerPlayerState))
    {
        [[maybe_unused]] auto Channel = DoSpawnAndRegister_Channel(
            [InOwnerPlayerState](ACk_ActorRelay_UE* InNewChannel)
            {
                InNewChannel->SetOwner(InOwnerPlayerState);
            });
    }
    else
    {
        [[maybe_unused]] auto Channel = DoSpawnAndRegister_Channel();
    }
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
    DoSpawnAndRegister_Channel(
        const TFunction<void(ACk_ActorRelay_UE*)>& InPreFinishSpawnFunc)
    -> ACk_ActorRelay_UE*
{
    const auto ActorClass = Get_ActorClass();

    CK_ENSURE_IF_NOT(ck::IsValid(ActorClass),
        TEXT("ActorClass is invalid for group [{}]"), Get_GroupTag())
    { return nullptr; }

    auto Channel = Cast<ACk_ActorRelay_UE>
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

                if (InPreFinishSpawnFunc)
                {
                    InPreFinishSpawnFunc(NewChannel);
                }
            }
        )
    );

    if (ck::IsValid(Channel))
    {
        DoRegisterChannelActor(Channel);
    }

    return Channel;
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    Get_WarmChannelCount() const
    -> int32
{
    const auto WarmCount = UCk_Utils_ActorRelay_Settings_UE::Get_WarmChannelCount();
    return FMath::Clamp(WarmCount, 1, Get_ChannelCount());
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    DoSpawnChannels_Server()
    -> void
{
    const auto NumChannels = Get_WarmChannelCount();

    ck::actorrelay::Log(TEXT("Spawning [{}] warm server channels (cap [{}]) for group [{}]"),
        NumChannels, Get_ChannelCount(), Get_GroupTag());

    for (auto Index = 0; Index < NumChannels; ++Index)
    {
        [[maybe_unused]] auto Channel = DoSpawnAndRegister_Channel();
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

    const auto NumChannels = Get_WarmChannelCount();

    ck::actorrelay::Log(TEXT("Spawning [{}] warm player channels (cap [{}]) for PlayerController [{}] in group [{}]"),
        NumChannels, Get_ChannelCount(), InPlayerController->GetName(), Get_GroupTag());

    for (auto Index = 0; Index < NumChannels; ++Index)
    {
        [[maybe_unused]] auto Channel = DoSpawnAndRegister_Channel(
            [&](ACk_ActorRelay_UE* InNewChannel)
            {
                if (const auto PlayerState = InPlayerController->PlayerState;
                    ck::IsValid(PlayerState))
                {
                    InNewChannel->SetOwner(PlayerState);
                }
            });
    }
}

auto
    UCk_ActorRelay_Group_Subsystem_Base_UE::
    DoDestroyChannels_ForPlayer(
        APlayerState* InPlayerState)
    -> void
{
    const auto FoundPool = _PlayerChannels.Find(InPlayerState);

    if (ck::Is_NOT_Valid(FoundPool, ck::IsValid_Policy_NullptrOnly{}))
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
