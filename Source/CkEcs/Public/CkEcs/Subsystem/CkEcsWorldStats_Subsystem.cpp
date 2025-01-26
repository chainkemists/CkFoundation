#include "CkEcsWorldStats_Subsystem.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Game/CkGame_Utils.h"

#include "Net/Core/PushModel/PushModel.h"

#include <Net/UnrealNetwork.h>

#include <Async/Async.h>
#include <Engine/Classes/Engine/GameViewportClient.h>
#include <Stats/StatsData.h>

// --------------------------------------------------------------------------------------------------------------------

ACk_EcsWorld_StatReplicatorActor_UE::
    ACk_EcsWorld_StatReplicatorActor_UE()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    NetPriority = 1.0f; // lower than zero triggers the ensure in ActorReplicationBridge on line 267
}

auto
    ACk_EcsWorld_StatReplicatorActor_UE::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    constexpr auto Params = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _AssociatedEcsWorldTickingGroupAverageCycleMs, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _AssociatedEcsWorldTickingGroup, Params);
}

auto
    ACk_EcsWorld_StatReplicatorActor_UE::
    Initialize(
        const FGameplayTag& InAssociatedEcsWorldTickingGroup)
    -> void
{
    if (NOT IsNetMode(NM_DedicatedServer))
    { return; }

    _AssociatedEcsWorldTickingGroup = InAssociatedEcsWorldTickingGroup;

    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _AssociatedEcsWorldTickingGroup, this);
}

auto
    ACk_EcsWorld_StatReplicatorActor_UE::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    _EcsWorldStatsSubsystem = GetWorld()->GetSubsystem<UCk_EcsWorld_Stats_Subsystem_UE>();

    CK_ENSURE_IF_NOT(ck::IsValid(_EcsWorldStatsSubsystem), TEXT("Failed to retrieve Ecs World Stats Subsystem"))
    { return; }

    if (IsNetMode(NM_DedicatedServer))
    { return; }

    if (ck::Is_NOT_Valid(_AssociatedEcsWorldTickingGroup))
    { return; }

    _EcsWorldStatsSubsystem->_StatReplicatorActors.Add(_AssociatedEcsWorldTickingGroup, this);
}

auto
    ACk_EcsWorld_StatReplicatorActor_UE::
    OnRep_AssociatedEcsWorldTickingGroupAverageCycleMs()
    -> void
{
    if (ck::Is_NOT_Valid(_AssociatedEcsWorldTickingGroup))
    { return; }

    if (IsNetMode(NM_DedicatedServer))
    { return; }

    if (ck::Is_NOT_Valid(_EcsWorldStatsSubsystem))
    { return; }

    auto* FoundEcsWorldActorStatData = _EcsWorldStatsSubsystem->_EcsWorldsCollectingStats_OnServer.FindByPredicate([&](const auto& InEcsWorldActorStatData)
    {
        return _AssociatedEcsWorldTickingGroup == InEcsWorldActorStatData.EcsWorldMinimalInfo.Get_EcsWorldTickingGroup();
    });

    if (ck::Is_NOT_Valid(FoundEcsWorldActorStatData, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    FoundEcsWorldActorStatData->TickInclusiveAverageCycleMs = _AssociatedEcsWorldTickingGroupAverageCycleMs;
}

auto
    ACk_EcsWorld_StatReplicatorActor_UE::
    OnRep_AssociatedEcsWorldTickingGroup()
    -> void
{
    if (ck::Is_NOT_Valid(_EcsWorldStatsSubsystem))
    { return; }

    _EcsWorldStatsSubsystem->_StatReplicatorActors.Add(_AssociatedEcsWorldTickingGroup, this);
}

auto
    ACk_EcsWorld_StatReplicatorActor_UE::
    Set_AssociatedEcsWorldTickingGroupAverageCycleMs(
        float InAverageCycleMs)
    -> void
{
    _AssociatedEcsWorldTickingGroupAverageCycleMs = InAverageCycleMs;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _AssociatedEcsWorldTickingGroupAverageCycleMs, this);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EcsWorld_Stats_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);
    _EcsWorldSubsystem = InCollection.InitializeDependency<UCk_EcsWorld_Subsystem_UE>();
}

auto
    UCk_EcsWorld_Stats_Subsystem_UE::
    Deinitialize()
    -> void
{
#if STATS
    FCriticalSection CriticalSection;
    {
        FScopeLock Lock(&CriticalSection);
        const auto& Stats = FStatsThreadState::GetLocalState();
        Stats.NewFrameDelegate.Remove(_OnNewFrameDelegateHandle);
    }
#endif

    Super::Deinitialize();
}

auto
    UCk_EcsWorld_Stats_Subsystem_UE::
    ShouldCreateSubsystem(
        UObject* Outer) const
    -> bool
{
#if STATS
    return Super::ShouldCreateSubsystem(Outer);
#else
    return false;
#endif
}

auto
    UCk_EcsWorld_Stats_Subsystem_UE::
    OnWorldBeginPlay(
        UWorld& InWorld)
    -> void
{
    Super::OnWorldBeginPlay(InWorld);

    if (ck::Is_NOT_Valid(_EcsWorldSubsystem))
    { return; }

    for (const auto& WorldActors = _EcsWorldSubsystem->_WorldActors_ByEcsWorldTickingGroup;
        const auto& Kvp : WorldActors)
    {
        const auto& WorldActor = Kvp.Value;

        if (ck::Is_NOT_Valid(WorldActor))
        { continue; }

        const auto& EcsWorldDisplayName  = WorldActor->Get_EcsWorldDisplayName();
        const auto& EcsWorldTickingGroup = WorldActor->Get_EcsWorldTickingGroup();
        const auto& StatCollectionPolicy = WorldActor->Get_StatCollectionPolicy();

        if (StatCollectionPolicy == ECk_Ecs_WorldStatCollection_Policy::CollectOnLocalClientOnly || StatCollectionPolicy == ECk_Ecs_WorldStatCollection_Policy::CollectOnLocalClientAndServer)
        {
            _EcsWorldsCollectingStats_OnClient.Add(FEcsWorldStatsData{
                FCk_EcsWorldWithStats_MinimalInfo{EcsWorldDisplayName, EcsWorldTickingGroup}, WorldActor->Get_TickStatName()});
        }

        if (StatCollectionPolicy == ECk_Ecs_WorldStatCollection_Policy::CollectOnServerOnly || StatCollectionPolicy == ECk_Ecs_WorldStatCollection_Policy::CollectOnLocalClientAndServer)
        {
            _EcsWorldsCollectingStats_OnServer.Add(FEcsWorldStatsData{
                FCk_EcsWorldWithStats_MinimalInfo{EcsWorldDisplayName, EcsWorldTickingGroup}, WorldActor->Get_TickStatName()});
        }
    }

    if (InWorld.IsNetMode(NM_DedicatedServer) && NOT _EcsWorldsCollectingStats_OnServer.IsEmpty())
    {
        for (const auto& [EcsWorldMinimalInfo, TickStatName, TickInclusiveAverageCycleMs] : _EcsWorldsCollectingStats_OnServer)
        {
            const auto EcsWorldStatReplicatorActor = Cast<ACk_EcsWorld_StatReplicatorActor_UE>
            (
                UCk_Utils_Actor_UE::Request_SpawnActor
                (
                    FCk_Utils_Actor_SpawnActor_Params{&InWorld, ACk_EcsWorld_StatReplicatorActor_UE::StaticClass()}
                    .Set_SpawnPolicy(ECk_Utils_Actor_SpawnActorPolicy::CannotSpawnInPersistentLevel)
                    .Set_NetworkingType(ECk_Actor_NetworkingType::Replicated),
                    [&](AActor* InActor)
                    {
                        const auto& NewWorldActor = Cast<ACk_EcsWorld_StatReplicatorActor_UE>(InActor);
                        NewWorldActor->Initialize(EcsWorldMinimalInfo.Get_EcsWorldTickingGroup());
                    }
                )
            );

            _StatReplicatorActors.Add(EcsWorldMinimalInfo.Get_EcsWorldTickingGroup(), EcsWorldStatReplicatorActor);
        }

#if	STATS
        FCriticalSection CriticalSection;
        {
            FScopeLock Lock(&CriticalSection);
            const auto& Stats = FStatsThreadState::GetLocalState();
            _OnNewFrameDelegateHandle = Stats.NewFrameDelegate.AddUObject(this, &ThisType::OnNewFrame);
        }
#endif

        DoTryEnableEcsWorldStat();
    }

    if (NOT InWorld.IsNetMode(NM_DedicatedServer) && NOT _EcsWorldsCollectingStats_OnClient.IsEmpty())
    {
#if	STATS
        FCriticalSection CriticalSection;
        {
            FScopeLock Lock(&CriticalSection);
            const auto& Stats = FStatsThreadState::GetLocalState();
            _OnNewFrameDelegateHandle = Stats.NewFrameDelegate.AddUObject(this, &ThisType::OnNewFrame);
        }
#endif

        DoTryEnableEcsWorldStat();
    }
}

auto
    UCk_EcsWorld_Stats_Subsystem_UE::
    Tick(
        float InDeltaTime)
    -> void
{
    Super::Tick(InDeltaTime);

    if (NOT Get_AreAllWorldSubsystemsInitialized())
    { return; }

    if (NOT GetWorld()->IsNetMode(NM_DedicatedServer) && NOT _EcsWorldsCollectingStats_OnClient.IsEmpty())
    {
        // Enabling ANY other stat completely overrides your -nodisplay flag set earlier!
        // If the command `stat none` is executed, it will disable our non-displayed ecs world stats
        // P.S. If the command is executed on the server well, there will be infinite recursion
        DoTryEnableEcsWorldStat();
    }
}

auto
    UCk_EcsWorld_Stats_Subsystem_UE::
    GetStatId() const
    -> TStatId
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UCk_EcsWorld_Stats_Subsystem_UE, STATGROUP_Tickables);
}

auto
    UCk_EcsWorld_Stats_Subsystem_UE::
    OnNewFrame(
        int64 InNewFrame)
    -> void
{
    auto WorldPtr = TWeakObjectPtr<UCk_EcsWorld_Stats_Subsystem_UE>{this};

    AsyncTask(ENamedThreads::GameThread, [WorldPtr]()
    {
        if (ck::Is_NOT_Valid(WorldPtr))
        { return; }

        if (const auto& World = WorldPtr->GetWorld();
            ck::Is_NOT_Valid(World))
        { return; }

        const auto* LatestStatsData = FLatestGameThreadStatsData::Get().Latest;

        if (ck::Is_NOT_Valid(LatestStatsData, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        for (const auto& ActiveStatGroup : LatestStatsData->ActiveStatGroups)
        {
            for (const auto& StatMessage : ActiveStatGroup.FlatAggregate)
            {
                WorldPtr->DoTryUpdateEcsWorldStatData(StatMessage);
            }
        }
    });
}

auto
    UCk_EcsWorld_Stats_Subsystem_UE::
    DoTryUpdateEcsWorldStatData(
        const FComplexStatMessage& InStatMessage)
    -> void
{
    const auto& FindEcsWorldStatData = [&](TArray<FEcsWorldStatsData>& InEcsWorldStatDataList) -> FEcsWorldStatsData*
    {
        auto* FoundEcsWorldActorStatData = InEcsWorldStatDataList.FindByPredicate([&](const FEcsWorldStatsData& InEcsWorldActorStatData)
        {
            const auto& TickStatName = InEcsWorldActorStatData.TickStatName;
            const auto& StatMessageShortName = FStatNameAndInfo::GetShortNameFrom(InStatMessage.NameAndInfo.GetRawName());

            return TickStatName == StatMessageShortName;
        });

        return FoundEcsWorldActorStatData;
    };

    const auto& StatCycleMs = FPlatformTime::ToMilliseconds(InStatMessage.GetValue_Duration(EComplexStatField::IncAve));
    auto* EcsWorldStat =  GetWorld()->IsNetMode(NM_DedicatedServer)
                                        ? FindEcsWorldStatData(_EcsWorldsCollectingStats_OnServer)
                                        : FindEcsWorldStatData(_EcsWorldsCollectingStats_OnClient);

    if (ck::Is_NOT_Valid(EcsWorldStat, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    if (NOT GetWorld()->IsNetMode(NM_DedicatedServer))
    {
        EcsWorldStat->TickInclusiveAverageCycleMs = StatCycleMs;
        return;
    }

    const auto& EcsWorldTickingGroup = EcsWorldStat->EcsWorldMinimalInfo.Get_EcsWorldTickingGroup();
    const auto& FoundStatReplicatorActor = _StatReplicatorActors.Find(EcsWorldTickingGroup);

    CK_ENSURE_IF_NOT(ck::IsValid(FoundStatReplicatorActor, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Failed to find the Ecs World Stat Replicator Actor associated with Ecs World Ticking Group [{}] on the SERVER"), EcsWorldTickingGroup)
    { return; }

    const auto& StatReplicatorActorWeak = *FoundStatReplicatorActor;

    if (ck::Is_NOT_Valid(StatReplicatorActorWeak))
    { return; }

    StatReplicatorActorWeak->Set_AssociatedEcsWorldTickingGroupAverageCycleMs(StatCycleMs);
}

auto
    UCk_EcsWorld_Stats_Subsystem_UE::
    DoTryEnableEcsWorldStat() const
    -> void
{
    if (ck::Is_NOT_Valid(GEngine))
    { return; }

    /*
     * Executing the command when the stat is already enabled disables it.
     * If the game is run twice with the "New Editor Window (PIE)" option, the first run will enable the option and,
     * because the game is run in the same process as the editor, when the game is run the second time the stat will already be enabled.
     * In this case we don't want to run the command again which would result in the stat getting disabled.
     */
    const auto& GameViewport = GEngine->GameViewport;

    const auto& DoEnableEcsWorldStat = [&]()
    {
        static auto Cmd = FString(TEXT("stat CkEcsWorldActor_Tick -nodisplay"));
        GEngine->Exec(GetWorld(), *Cmd);
    };

    if (ck::Is_NOT_Valid(GameViewport))
    {
        DoEnableEcsWorldStat();
        return;
    }

    if (NOT GameViewport->IsStatEnabled(TEXT("CkEcsWorldActor_Tick")))
    {
        DoEnableEcsWorldStat();
        return;
    }

    if (static auto ForceEnableStatOnceViewportIsWarmedUp = true;
        ForceEnableStatOnceViewportIsWarmedUp)
    {
        ForceEnableStatOnceViewportIsWarmedUp = false;
        DoEnableEcsWorldStat();
        return;
    }
}

auto
    UCk_EcsWorld_Stats_Subsystem_UE::
    Get_EcsWorldsCollectingStats(
        ECk_ClientServer InStatCollectionSource) const
    -> TArray<FCk_EcsWorldWithStats_MinimalInfo>
{
    const auto& ExtractEcsWorldNames = [](const TArray<FEcsWorldStatsData>& InEcsWorldStatsDataList)
    {
        return ck::algo::Transform<TArray<FCk_EcsWorldWithStats_MinimalInfo>>(InEcsWorldStatsDataList, [](const FEcsWorldStatsData& InEcsWorldStatsData)
        {
            return InEcsWorldStatsData.EcsWorldMinimalInfo;
        });
    };

    switch (InStatCollectionSource)
    {
        case ECk_ClientServer::Client: { return ExtractEcsWorldNames(_EcsWorldsCollectingStats_OnClient); }
        case ECk_ClientServer::Server: { return ExtractEcsWorldNames(_EcsWorldsCollectingStats_OnServer); }
        default:
        {
            CK_INVALID_ENUM(InStatCollectionSource);
            return {};
        }
    }
}

auto
    UCk_EcsWorld_Stats_Subsystem_UE::
    Get_StatDataForEcsWorldTickingGroup(
        FGameplayTag InEcsWorld,
        ECk_ClientServer InStatCollectionSource) const
    -> float
{
    const auto& FindEcsWorldStatValue = [&](const TArray<FEcsWorldStatsData>& InEcsWorldStatsDataList) -> float
    {
        const auto& FoundEcsWorldStatData = InEcsWorldStatsDataList.FindByPredicate([&](const FEcsWorldStatsData& InEcsWorldStatsData)
        {
            return InEcsWorldStatsData.EcsWorldMinimalInfo.Get_EcsWorldTickingGroup() == InEcsWorld;
        });

        CK_ENSURE_IF_NOT(ck::IsValid(FoundEcsWorldStatData, ck::IsValid_Policy_NullptrOnly{}),
            TEXT("Failed to find Stat Data for Ecs World Ticking Group [{}] on [{}]"),
            InEcsWorld,
            InStatCollectionSource)
        { return {}; }

        return FoundEcsWorldStatData->TickInclusiveAverageCycleMs;
    };

    switch (InStatCollectionSource)
    {
        case ECk_ClientServer::Client: { return FindEcsWorldStatValue(_EcsWorldsCollectingStats_OnClient); }
        case ECk_ClientServer::Server: { return FindEcsWorldStatValue(_EcsWorldsCollectingStats_OnServer); }
        default:
        {
            CK_INVALID_ENUM(InStatCollectionSource);
            return {};
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
