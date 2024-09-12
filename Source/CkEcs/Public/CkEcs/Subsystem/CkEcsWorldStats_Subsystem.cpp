#include "CkEcsWorldStats_Subsystem.h"

#include <Stats/StatsData.h>

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
    const auto& Stats = FStatsThreadState::GetLocalState();
    Stats.NewFrameDelegate.Remove(_OnNewFrameDelegateHandle);

    Super::Deinitialize();
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

        const auto& EcsWorldTickingGroup = WorldActor->Get_EcsWorldTickingGroup();
        const auto& StatCollectionPolicy = WorldActor->Get_StatCollectionPolicy();

        if (StatCollectionPolicy == ECk_Ecs_WorldStatCollection_Policy::CollectOnLocalClientOnly || StatCollectionPolicy == ECk_Ecs_WorldStatCollection_Policy::CollectOnLocalClientAndServer)
        {
            _EcsWorldsCollectingStats_OnClient.Add(FEcsWorldStatsData{EcsWorldTickingGroup, WorldActor->Get_TickStatName()});
        }

        if (StatCollectionPolicy == ECk_Ecs_WorldStatCollection_Policy::CollectOnServerOnly || StatCollectionPolicy == ECk_Ecs_WorldStatCollection_Policy::CollectOnLocalClientAndServer)
        {
            _EcsWorldsCollectingStats_OnServer.Add(FEcsWorldStatsData{EcsWorldTickingGroup, WorldActor->Get_TickStatName()});
        }
    }

    if (InWorld.IsNetMode(NM_DedicatedServer) && NOT _EcsWorldsCollectingStats_OnServer.IsEmpty())
    {
        const auto& Stats = FStatsThreadState::GetLocalState();
        _OnNewFrameDelegateHandle = Stats.NewFrameDelegate.AddUObject(this, &ThisType::OnNewFrame);

        // TODO: Spawn Ecs World Stat Replicator Actor
    }

    if (NOT InWorld.IsNetMode(NM_DedicatedServer) && NOT _EcsWorldsCollectingStats_OnClient.IsEmpty())
    {
        const auto& Stats = FStatsThreadState::GetLocalState();
        _OnNewFrameDelegateHandle = Stats.NewFrameDelegate.AddUObject(this, &ThisType::OnNewFrame);
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

    // TODO: Toggle on/off relevant stats
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
    AsyncTask(ENamedThreads::GameThread, [this]()
    {
        if (ck::Is_NOT_Valid(this))
        { return; }

        const auto& World = this->GetWorld();

        if (ck::Is_NOT_Valid(World))
        { return; }

        const auto* LatestStatsData = FLatestGameThreadStatsData::Get().Latest;

        if (ck::Is_NOT_Valid(LatestStatsData, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        const auto& TryUpdateAssociatedEcsWorldStat = [&](const FComplexStatMessage& InStatMessage)
        {
            auto& WorldActorsToUse = World->IsNetMode(NM_DedicatedServer)
                                        ? _EcsWorldsCollectingStats_OnServer
                                        : _EcsWorldsCollectingStats_OnClient;

            auto* FoundEcsWorldActorStatData = WorldActorsToUse.FindByPredicate([&](const FEcsWorldStatsData& InEcsWorldActorStatData)
            {
                const auto& WorldStatName = InEcsWorldActorStatData.EcsWorldTickStatName;
                const auto& StatMessageShortName = FStatNameAndInfo::GetShortNameFrom(InStatMessage.NameAndInfo.GetRawName());

                return WorldStatName == StatMessageShortName;
            });

            if (ck::Is_NOT_Valid(FoundEcsWorldActorStatData, ck::IsValid_Policy_NullptrOnly{}))
            { return; }

            FoundEcsWorldActorStatData->TickInclusiveAverageCycleMs = FPlatformTime::ToMilliseconds(InStatMessage.GetValue_Duration(EComplexStatField::IncAve));
        };

        for(int32 GroupIndex = 0; GroupIndex < LatestStatsData->ActiveStatGroups.Num(); ++GroupIndex)
        {
            const auto& StatGroup = LatestStatsData->ActiveStatGroups[GroupIndex];

            for (const auto& StatMessage : StatGroup.FlatAggregate)
            {
                TryUpdateAssociatedEcsWorldStat(StatMessage);
            }
        }
    });
}

auto
    UCk_EcsWorld_Stats_Subsystem_UE::
    Get_EcsWorldsCollectingStats(
        ECk_ClientServer InStatCollectionSource) const
    -> TArray<FGameplayTag>
{
    const auto& ExtractEcsWorldNames = [](const TArray<FEcsWorldStatsData>& InEcsWorldStatsDataList)
    {
        return ck::algo::Transform<TArray<FGameplayTag>>(InEcsWorldStatsDataList, [](const FEcsWorldStatsData& InEcsWorldStatsData)
        {
            return InEcsWorldStatsData.EcsWorldTickingGroup;
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
        ECk_ClientServer InStatCollectionSource) const -> float
{
    const auto& TryGetStatData = [&](const TArray<FEcsWorldStatsData>& InEcsWorldStatsDataList) -> float
    {
        const auto& FoundEcsWorldStatData = InEcsWorldStatsDataList.FindByPredicate([&](const FEcsWorldStatsData& InEcsWorldStatsData)
        {
            return InEcsWorldStatsData.EcsWorldTickingGroup == InEcsWorld;
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
        case ECk_ClientServer::Client: { return TryGetStatData(_EcsWorldsCollectingStats_OnClient); }
        case ECk_ClientServer::Server: { return TryGetStatData(_EcsWorldsCollectingStats_OnServer); }
        default:
        {
            CK_INVALID_ENUM(InStatCollectionSource);
            return {};
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
