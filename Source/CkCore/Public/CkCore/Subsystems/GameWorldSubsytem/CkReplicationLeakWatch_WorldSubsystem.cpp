#include "CkReplicationLeakWatch_WorldSubsystem.h"

#include "CkCore/CkCoreLog.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Engine/NetDriver.h>
#include <Engine/World.h>
#include <EngineUtils.h>
#include <HAL/IConsoleManager.h>
#include <UObject/UObjectIterator.h>

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    auto
        ForEachReplicationLeakWatchSubsystem(
            const TFunction<void(UCk_ReplicationLeakWatch_WorldSubsystem_UE*)>& InFunc)
        -> void
    {
        for (TObjectIterator<UCk_ReplicationLeakWatch_WorldSubsystem_UE> It; It; ++It)
        {
            auto* Subsystem = *It;

            if (NOT ck::IsValid(Subsystem))
            { continue; }

            if (Subsystem->IsTemplate())
            { continue; }

            if (NOT ck::IsValid(Subsystem->GetWorld()))
            { continue; }

            InFunc(Subsystem);
        }
    }
}

namespace ck::replication_watch
{
    static TAutoConsoleVariable<int32> CVar_Enabled(
        TEXT("ck.net.repwatch.Enabled"),
        0,
        TEXT("Enable replicated-object leak monitoring.\n")
        TEXT("0 = Disabled, 1 = Enabled"),
        ECVF_Default);

    static TAutoConsoleVariable<float> CVar_SampleIntervalSeconds(
        TEXT("ck.net.repwatch.SampleIntervalSeconds"),
        5.0f,
        TEXT("Sampling interval in seconds."),
        ECVF_Default);

    static TAutoConsoleVariable<float> CVar_LeakAgeThresholdSeconds(
        TEXT("ck.net.repwatch.LeakAgeThresholdSeconds"),
        120.0f,
        TEXT("Objects older than this threshold are flagged as leak suspects."),
        ECVF_Default);

    static TAutoConsoleVariable<int32> CVar_GrowthThresholdCount(
        TEXT("ck.net.repwatch.GrowthThresholdCount"),
        50,
        TEXT("If total tracked replicated objects grow by this amount in one sample, raise a warning."),
        ECVF_Default);

    static TAutoConsoleVariable<int32> CVar_TopN(
        TEXT("ck.net.repwatch.TopN"),
        20,
        TEXT("Maximum number of top classes/owners to print in reports."),
        ECVF_Default);

    static TAutoConsoleVariable<int32> CVar_LogEachSuspect(
        TEXT("ck.net.repwatch.LogEachSuspect"),
        0,
        TEXT("Whether to log each suspect object line.\n")
        TEXT("0 = Summary only, 1 = Include per-object suspect rows"),
        ECVF_Default);

    static TAutoConsoleVariable<int32> CVar_AutoDumpOnGrowth(
        TEXT("ck.net.repwatch.AutoDumpOnGrowth"),
        0,
        TEXT("Automatically dump a report when growth threshold is hit.\n")
        TEXT("0 = Disabled, 1 = Enabled"),
        ECVF_Default);

    static TAutoConsoleVariable<int32> CVar_CommandStart(
        TEXT("ck.net.repwatch.CommandStart"),
        0,
        TEXT("Set to 1 to start monitoring in all active game worlds."),
        FConsoleVariableDelegate::CreateLambda([](const IConsoleVariable* InVar)
        {
            if (InVar->GetInt() <= 0)
            { return; }

            ForEachReplicationLeakWatchSubsystem([](UCk_ReplicationLeakWatch_WorldSubsystem_UE* InSubsystem)
            {
                InSubsystem->Request_StartMonitoring();
            });

            if (auto* ResetVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.net.repwatch.CommandStart"));
                ResetVar != nullptr)
            {
                ResetVar->Set(0, ECVF_SetByCode);
            }
        }),
        ECVF_Default);

    static TAutoConsoleVariable<int32> CVar_CommandStop(
        TEXT("ck.net.repwatch.CommandStop"),
        0,
        TEXT("Set to 1 to stop monitoring in all active game worlds."),
        FConsoleVariableDelegate::CreateLambda([](const IConsoleVariable* InVar)
        {
            if (InVar->GetInt() <= 0)
            { return; }

            ForEachReplicationLeakWatchSubsystem([](UCk_ReplicationLeakWatch_WorldSubsystem_UE* InSubsystem)
            {
                InSubsystem->Request_StopMonitoring();
            });

            if (auto* ResetVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.net.repwatch.CommandStop"));
                ResetVar != nullptr)
            {
                ResetVar->Set(0, ECVF_SetByCode);
            }
        }),
        ECVF_Default);

    static TAutoConsoleVariable<int32> CVar_CommandSample(
        TEXT("ck.net.repwatch.CommandSample"),
        0,
        TEXT("Set to 1 to force a sample in all active game worlds."),
        FConsoleVariableDelegate::CreateLambda([](const IConsoleVariable* InVar)
        {
            if (InVar->GetInt() <= 0)
            { return; }

            ForEachReplicationLeakWatchSubsystem([](UCk_ReplicationLeakWatch_WorldSubsystem_UE* InSubsystem)
            {
                InSubsystem->Request_SampleNow();
            });

            if (auto* ResetVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.net.repwatch.CommandSample"));
                ResetVar != nullptr)
            {
                ResetVar->Set(0, ECVF_SetByCode);
            }
        }),
        ECVF_Default);

    static TAutoConsoleVariable<int32> CVar_CommandDump(
        TEXT("ck.net.repwatch.CommandDump"),
        0,
        TEXT("Set to 1 to dump a replication leak report in all active game worlds."),
        FConsoleVariableDelegate::CreateLambda([](const IConsoleVariable* InVar)
        {
            if (InVar->GetInt() <= 0)
            { return; }

            ForEachReplicationLeakWatchSubsystem([](UCk_ReplicationLeakWatch_WorldSubsystem_UE* InSubsystem)
            {
                InSubsystem->Request_DumpReport();
            });

            if (auto* ResetVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.net.repwatch.CommandDump"));
                ResetVar != nullptr)
            {
                ResetVar->Set(0, ECVF_SetByCode);
            }
        }),
        ECVF_Default);

    static TAutoConsoleVariable<int32> CVar_CommandResetBaseline(
        TEXT("ck.net.repwatch.CommandResetBaseline"),
        0,
        TEXT("Set to 1 to clear tracked baseline/history in all active game worlds."),
        FConsoleVariableDelegate::CreateLambda([](const IConsoleVariable* InVar)
        {
            if (InVar->GetInt() <= 0)
            { return; }

            ForEachReplicationLeakWatchSubsystem([](UCk_ReplicationLeakWatch_WorldSubsystem_UE* InSubsystem)
            {
                InSubsystem->Request_ResetBaseline();
            });

            if (auto* ResetVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.net.repwatch.CommandResetBaseline"));
                ResetVar != nullptr)
            {
                ResetVar->Set(0, ECVF_SetByCode);
            }
        }),
        ECVF_Default);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    Tick(float InDeltaSeconds)
    -> void
{
    Super::Tick(InDeltaSeconds);

    if (NOT ck::IsValid(GetWorld()))
    { return; }

    if (IsMonitoringEnabledByCVar())
    {
        if (NOT _IsMonitoring)
        {
            Request_StartMonitoring();
        }
    }
    else if (_IsMonitoring)
    {
        Request_StopMonitoring();
    }

    if (NOT _IsMonitoring)
    { return; }

    _TimeSinceLastSampleSeconds += static_cast<double>(InDeltaSeconds);

    if (_TimeSinceLastSampleSeconds < static_cast<double>(GetCurrentSampleIntervalSeconds()))
    { return; }

    Request_SampleNow();
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    GetStatId() const
    -> TStatId
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UCk_ReplicationLeakWatch_WorldSubsystem_UE, STATGROUP_Tickables);
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    Request_StartMonitoring()
    -> void
{
    _IsMonitoring = true;
    _TimeSinceLastSampleSeconds = 0.0;

    ck::core::Warning(TEXT("[RepWatch] START World=[{}]"), GetWorld());
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    Request_StopMonitoring()
    -> void
{
    _IsMonitoring = false;
    _TimeSinceLastSampleSeconds = 0.0;

    ck::core::Warning(TEXT("[RepWatch] STOP World=[{}]"), GetWorld());
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    Request_SampleNow()
    -> void
{
    if (NOT ck::IsValid(GetWorld()))
    { return; }

    _TimeSinceLastSampleSeconds = 0.0;
    const auto NowSeconds = static_cast<double>(GetWorld()->GetTimeSeconds());

    _LastSnapshot = BuildSnapshot(NowSeconds);
    _NumSamples += 1;

    const auto Growth = _LastSnapshot.TotalTrackedCount - _LastTotalTrackedCount;
    const auto GrowthThreshold = GetCurrentGrowthThresholdCount();

    if (_NumSamples > 1 && Growth >= GrowthThreshold)
    {
        ck::core::Warning(TEXT("[RepWatch] Growth spike World=[{}] Growth=[{}] Threshold=[{}] Total=[{}]"),
            GetWorld(),
            Growth,
            GrowthThreshold,
            _LastSnapshot.TotalTrackedCount);

        if (GetShouldAutoDumpOnGrowth())
        {
            Request_DumpReport();
        }
    }

    _LastTotalTrackedCount = _LastSnapshot.TotalTrackedCount;
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    Request_DumpReport()
    -> void
{
    if (NOT ck::IsValid(GetWorld()))
    { return; }

    if (_NumSamples == 0)
    {
        Request_SampleNow();
    }

    ck::core::Warning(TEXT("----- [RepWatch] Report BEGIN World=[{}] -----"), GetWorld());
    DumpSnapshot(_LastSnapshot);
    DumpSuspects(static_cast<double>(GetWorld()->GetTimeSeconds()));
    ck::core::Warning(TEXT("----- [RepWatch] Report END World=[{}] -----"), GetWorld());
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    Request_ResetBaseline()
    -> void
{
    _TrackedObjects.Reset();
    _LastSnapshot = FReplicationSnapshot{};
    _NumSamples = 0;
    _LastTotalTrackedCount = 0;
    _TimeSinceLastSampleSeconds = 0.0;

    ck::core::Warning(TEXT("[RepWatch] Baseline reset World=[{}]"), GetWorld());
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    IsMonitoringEnabledByCVar() const
    -> bool
{
    return ck::replication_watch::CVar_Enabled.GetValueOnGameThread() != 0;
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    GetCurrentSampleIntervalSeconds() const
    -> float
{
    return FMath::Max(0.1f, ck::replication_watch::CVar_SampleIntervalSeconds.GetValueOnGameThread());
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    GetCurrentLeakAgeThresholdSeconds() const
    -> float
{
    return FMath::Max(1.0f, ck::replication_watch::CVar_LeakAgeThresholdSeconds.GetValueOnGameThread());
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    GetCurrentGrowthThresholdCount() const
    -> int32
{
    return FMath::Max(1, ck::replication_watch::CVar_GrowthThresholdCount.GetValueOnGameThread());
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    GetCurrentTopN() const
    -> int32
{
    return FMath::Clamp(ck::replication_watch::CVar_TopN.GetValueOnGameThread(), 1, 1024);
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    GetShouldLogEachSuspect() const
    -> bool
{
    return ck::replication_watch::CVar_LogEachSuspect.GetValueOnGameThread() != 0;
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    GetShouldAutoDumpOnGrowth() const
    -> bool
{
    return ck::replication_watch::CVar_AutoDumpOnGrowth.GetValueOnGameThread() != 0;
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    IsObjectInCurrentWorld(UObject* InObject) const
    -> bool
{
    if (NOT ck::IsValid(InObject))
    { return false; }

    const auto* World = GetWorld();
    if (NOT ck::IsValid(World))
    { return false; }

    if (InObject->GetWorld() == World)
    { return true; }

    if (const auto* OuterActor = InObject->GetTypedOuter<AActor>();
        ck::IsValid(OuterActor))
    {
        return OuterActor->GetWorld() == World;
    }

    if (const auto* OuterWorld = InObject->GetTypedOuter<UWorld>();
        ck::IsValid(OuterWorld))
    {
        return OuterWorld == World;
    }

    return false;
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    ResolveReplicatedOwner(UObject* InObject) const
    -> UObject*
{
    if (NOT ck::IsValid(InObject))
    { return nullptr; }

    if (const auto* Actor = Cast<AActor>(InObject))
    { return const_cast<AActor*>(Actor); }

    if (const auto* Component = Cast<UActorComponent>(InObject))
    {
        if (ck::IsValid(Component->GetOwner()))
        { return Component->GetOwner(); }

        return const_cast<UActorComponent*>(Component);
    }

    auto* Outer = InObject->GetOuter();
    while (ck::IsValid(Outer, ck::IsValid_Policy_IncludePendingKill{}))
    {
        if (auto* OwnerActor = Cast<AActor>(Outer);
            ck::IsValid(OwnerActor, ck::IsValid_Policy_IncludePendingKill{}))
        {
            return OwnerActor;
        }

        if (auto* OwnerComponent = Cast<UActorComponent>(Outer);
            ck::IsValid(OwnerComponent, ck::IsValid_Policy_IncludePendingKill{}))
        {
            if (ck::IsValid(OwnerComponent->GetOwner(), ck::IsValid_Policy_IncludePendingKill{}))
            {
                return OwnerComponent->GetOwner();
            }

            return OwnerComponent;
        }

        Outer = Outer->GetOuter();
    }

    return nullptr;
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    IsReplicatedOwner(UObject* InObject) const
    -> bool
{
    if (NOT ck::IsValid(InObject, ck::IsValid_Policy_IncludePendingKill{}))
    { return false; }

    if (const auto* Actor = Cast<AActor>(InObject))
    {
        return Actor->GetIsReplicated();
    }

    if (const auto* Component = Cast<UActorComponent>(InObject))
    {
        return Component->GetIsReplicated() ||
            (ck::IsValid(Component->GetOwner(), ck::IsValid_Policy_IncludePendingKill{}) && Component->GetOwner()->GetIsReplicated());
    }

    return false;
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    GetObjectTypeLabel(EObjectType InType) const
    -> const TCHAR*
{
    switch (InType)
    {
        case EObjectType::Actor: return TEXT("Actor");
        case EObjectType::Component: return TEXT("Component");
        case EObjectType::UObject: return TEXT("UObject");
        default: return TEXT("Unknown");
    }
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    GatherReplicatedActors(TArray<AActor*>& OutActors) const
    -> void
{
    OutActors.Reset();

    if (NOT ck::IsValid(GetWorld()))
    { return; }

    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        auto* Actor = *It;
        if (ck::IsValid(Actor) && Actor->GetIsReplicated())
        {
            OutActors.Add(Actor);
        }
    }
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    GatherReplicatedComponents(
        const TArray<AActor*>& InActors,
        TArray<UActorComponent*>& OutComponents) const
    -> void
{
    OutComponents.Reset();

    for (const auto* Actor : InActors)
    {
        if (NOT ck::IsValid(Actor))
        { continue; }

        auto Components = TArray<UActorComponent*>{};
        Actor->GetComponents(Components);

        for (auto* Component : Components)
        {
            if (ck::IsValid(Component) && Component->GetIsReplicated())
            {
                OutComponents.Add(Component);
            }
        }
    }
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    GatherReplicatedUObjects(TArray<UObject*>& OutObjects) const
    -> void
{
    OutObjects.Reset();

    for (TObjectIterator<UObject> It; It; ++It)
    {
        auto* Obj = *It;

        if (NOT ck::IsValid(Obj, ck::IsValid_Policy_IncludePendingKill{}))
        { continue; }

        if (Obj->IsTemplate())
        { continue; }

        if (Obj->IsA<AActor>() || Obj->IsA<UActorComponent>())
        { continue; }

        if (NOT IsObjectInCurrentWorld(Obj))
        { continue; }

        if (NOT Obj->IsSupportedForNetworking())
        { continue; }

        auto* Owner = ResolveReplicatedOwner(Obj);
        if (NOT IsReplicatedOwner(Owner))
        { continue; }

        OutObjects.Add(Obj);
    }
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    TouchTracked(
        UObject* InObject,
        UObject* InOwner,
        EObjectType InType,
        double InNowSeconds)
    -> void
{
    if (NOT ck::IsValid(InObject, ck::IsValid_Policy_IncludePendingKill{}))
    { return; }

    auto& Tracked = _TrackedObjects.FindOrAdd(FObjectKey(InObject));
    Tracked.Object = InObject;
    Tracked.Owner = InOwner;
    Tracked.Type = InType;
    Tracked.LastSeenAtSeconds = InNowSeconds;
    Tracked.SeenSamples += 1;
    Tracked.bOwnerReplicated = IsReplicatedOwner(InOwner);

    if (Tracked.FirstSeenAtSeconds <= 0.0)
    {
        Tracked.FirstSeenAtSeconds = InNowSeconds;
    }
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    CompactTrackedMap()
    -> void
{
    auto KeysToRemove = TArray<FObjectKey>{};
    KeysToRemove.Reserve(_TrackedObjects.Num());

    for (const auto& Pair : _TrackedObjects)
    {
        const auto& Tracked = Pair.Value;
        if (NOT Tracked.Object.IsValid(true))
        {
            KeysToRemove.Add(Pair.Key);
        }
    }

    for (const auto& Key : KeysToRemove)
    {
        _TrackedObjects.Remove(Key);
    }
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    BuildSnapshot(
        double InNowSeconds)
    -> FReplicationSnapshot
{
    auto Snapshot = FReplicationSnapshot{};
    Snapshot.TimestampSeconds = InNowSeconds;

    auto ReplicatedActors = TArray<AActor*>{};
    auto ReplicatedComponents = TArray<UActorComponent*>{};
    auto ReplicatedObjects = TArray<UObject*>{};

    GatherReplicatedActors(ReplicatedActors);
    GatherReplicatedComponents(ReplicatedActors, ReplicatedComponents);
    GatherReplicatedUObjects(ReplicatedObjects);

    Snapshot.ReplicatedActorCount = ReplicatedActors.Num();
    Snapshot.ReplicatedComponentCount = ReplicatedComponents.Num();
    Snapshot.ReplicatedObjectCount = ReplicatedObjects.Num();
    Snapshot.TotalTrackedCount = Snapshot.ReplicatedActorCount + Snapshot.ReplicatedComponentCount + Snapshot.ReplicatedObjectCount;

    for (auto* Actor : ReplicatedActors)
    {
        if (NOT ck::IsValid(Actor))
        { continue; }

        auto* Owner = ResolveReplicatedOwner(Actor);
        Snapshot.ActorClassCounts.FindOrAdd(Actor->GetClass()) += 1;
        Snapshot.OwnerCounts.FindOrAdd(Owner) += 1;
        TouchTracked(Actor, Owner, EObjectType::Actor, InNowSeconds);
    }

    for (auto* Component : ReplicatedComponents)
    {
        if (NOT ck::IsValid(Component))
        { continue; }

        auto* Owner = ResolveReplicatedOwner(Component);
        Snapshot.ComponentClassCounts.FindOrAdd(Component->GetClass()) += 1;
        Snapshot.OwnerCounts.FindOrAdd(Owner) += 1;
        TouchTracked(Component, Owner, EObjectType::Component, InNowSeconds);
    }

    for (auto* Object : ReplicatedObjects)
    {
        if (NOT ck::IsValid(Object))
        { continue; }

        auto* Owner = ResolveReplicatedOwner(Object);
        Snapshot.ObjectClassCounts.FindOrAdd(Object->GetClass()) += 1;
        Snapshot.OwnerCounts.FindOrAdd(Owner) += 1;
        TouchTracked(Object, Owner, EObjectType::UObject, InNowSeconds);
    }

    CompactTrackedMap();
    return Snapshot;
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    DumpSnapshot(
        const FReplicationSnapshot& InSnapshot) const
    -> void
{
    auto* World = GetWorld();

    if (NOT ck::IsValid(World))
    { return; }

    const auto* NetDriver = World->GetNetDriver();
    const auto TopN = GetCurrentTopN();

    ck::core::Warning(TEXT("[RepWatch] Time=[{:.2f}] NetMode=[{}] NetDriver=[{}] ClientConnections=[{}]"),
        InSnapshot.TimestampSeconds,
        static_cast<int32>(World->GetNetMode()),
        NetDriver,
        NetDriver ? NetDriver->ClientConnections.Num() : 0);

    ck::core::Warning(TEXT("[RepWatch] Counts Actors=[{}] Components=[{}] UObjects=[{}] Total=[{}] Samples=[{}]"),
        InSnapshot.ReplicatedActorCount,
        InSnapshot.ReplicatedComponentCount,
        InSnapshot.ReplicatedObjectCount,
        InSnapshot.TotalTrackedCount,
        _NumSamples);

    const auto DumpTopClasses = [&](const TCHAR* InLabel, const TMap<UClass*, int32>& InMap)
    {
        auto Pairs = TArray<TPair<UClass*, int32>>{};
        Pairs.Reserve(InMap.Num());
        for (const auto& Pair : InMap)
        {
            Pairs.Add(Pair);
        }

        Pairs.Sort([](const TPair<UClass*, int32>& Lhs, const TPair<UClass*, int32>& Rhs)
        {
            return Lhs.Value > Rhs.Value;
        });

        const auto NumToPrint = FMath::Min(TopN, Pairs.Num());
        for (auto Index = 0; Index < NumToPrint; ++Index)
        {
            ck::core::Warning(TEXT("[RepWatch] {}[{}] Count=[{}] Key=[{}]"),
                InLabel,
                Index,
                Pairs[Index].Value,
                Pairs[Index].Key);
        }
    };

    const auto DumpTopOwners = [&](const TCHAR* InLabel, const TMap<UObject*, int32>& InMap)
    {
        auto Pairs = TArray<TPair<UObject*, int32>>{};
        Pairs.Reserve(InMap.Num());
        for (const auto& Pair : InMap)
        {
            Pairs.Add(Pair);
        }

        Pairs.Sort([](const TPair<UObject*, int32>& Lhs, const TPair<UObject*, int32>& Rhs)
        {
            return Lhs.Value > Rhs.Value;
        });

        const auto NumToPrint = FMath::Min(TopN, Pairs.Num());
        for (auto Index = 0; Index < NumToPrint; ++Index)
        {
            ck::core::Warning(TEXT("[RepWatch] {}[{}] Count=[{}] Key=[{}]"),
                InLabel,
                Index,
                Pairs[Index].Value,
                Pairs[Index].Key);
        }
    };

    DumpTopClasses(TEXT("TopActorClass"), InSnapshot.ActorClassCounts);
    DumpTopClasses(TEXT("TopComponentClass"), InSnapshot.ComponentClassCounts);
    DumpTopClasses(TEXT("TopObjectClass"), InSnapshot.ObjectClassCounts);
    DumpTopOwners(TEXT("TopOwner"), InSnapshot.OwnerCounts);
}

auto
    UCk_ReplicationLeakWatch_WorldSubsystem_UE::
    DumpSuspects(
        double InNowSeconds) const
    -> void
{
    const auto LeakAgeThresholdSeconds = static_cast<double>(GetCurrentLeakAgeThresholdSeconds());
    const auto bLogEachSuspect = GetShouldLogEachSuspect();

    auto NumSuspects = 0;

    for (const auto& Pair : _TrackedObjects)
    {
        const auto& Tracked = Pair.Value;
        const auto* Object = Tracked.Object.Get();

        if (NOT ck::IsValid(Object, ck::IsValid_Policy_IncludePendingKill{}))
        { continue; }

        const auto AgeSeconds = InNowSeconds - Tracked.FirstSeenAtSeconds;
        const auto bOwnerInvalid = NOT ck::IsValid(Tracked.Owner.Get(), ck::IsValid_Policy_IncludePendingKill{});
        const auto bOwnerNotReplicated = NOT Tracked.bOwnerReplicated;

        const auto bSuspectByAge = AgeSeconds >= LeakAgeThresholdSeconds;
        const auto bSuspect = bSuspectByAge || bOwnerInvalid || bOwnerNotReplicated;

        if (NOT bSuspect)
        { continue; }

        NumSuspects += 1;

        if (bLogEachSuspect)
        {
            ck::core::Warning(
                TEXT("[RepWatch] Suspect Type=[{}] AgeSec=[{:.2f}] Samples=[{}] OwnerReplicated=[{}] Object=[{}] Owner=[{}]"),
                GetObjectTypeLabel(Tracked.Type),
                AgeSeconds,
                Tracked.SeenSamples,
                Tracked.bOwnerReplicated ? 1 : 0,
                Object,
                Tracked.Owner.Get());
        }
    }

    ck::core::Warning(TEXT("[RepWatch] Suspects Count=[{}] LeakAgeThresholdSec=[{:.2f}]"),
        NumSuspects,
        LeakAgeThresholdSeconds);
}

// --------------------------------------------------------------------------------------------------------------------
