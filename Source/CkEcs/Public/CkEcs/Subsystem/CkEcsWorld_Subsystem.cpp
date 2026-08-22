#include "CkEcsWorld_Subsystem.h"

#include "CkCore/Actor/CkActor_Utils.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Scheduler/CkProcessorGraph.h"
#include "CkEcs/Scheduler/CkProcessorRegistry.h"
#include "CkEcs/Subsystem/CkEcsEditor_Subsystem.h"

#include "CkProfile/Stats/CkStats.h"

#include <Engine/World.h>
#include <Engine/Engine.h>
#include <HAL/FileManager.h>
#include <Misc/FileHelper.h>
#include <Misc/Paths.h>
#include <Templates/Function.h>

// --------------------------------------------------------------------------------------------------------------------

DECLARE_STATS_GROUP(TEXT("CkEcsWorldActor_Tick"), STATGROUP_CkEcsWorldActor_Tick, STATCAT_Advanced);

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::
    Get_TickPlanForHold(
        ECk_EcsWorld_LoadHold InHold,
        float InDeltaSeconds)
    -> ck::FCk_TickPlan
{
    switch (InHold)
    {
        case ECk_EcsWorld_LoadHold::None:
        {
            return FCk_TickPlan{ECk_SchedulerTickScope::Full, FCk_Time{InDeltaSeconds}};
        }
        case ECk_EcsWorld_LoadHold::Rebuilding:
        {
            return FCk_TickPlan{ECk_SchedulerTickScope::LoadKernel, FCk_Time{InDeltaSeconds}};
        }
        case ECk_EcsWorld_LoadHold::Teardown:
        case ECk_EcsWorld_LoadHold::Escalated:
        case ECk_EcsWorld_LoadHold::Draining:
        case ECk_EcsWorld_LoadHold::Converging:
        {
            return FCk_TickPlan{ECk_SchedulerTickScope::Full, FCk_Time{0.0f}};
        }
    }

    return FCk_TickPlan{ECk_SchedulerTickScope::Full, FCk_Time{InDeltaSeconds}};
}

// --------------------------------------------------------------------------------------------------------------------

ACk_EcsWorld_Actor_UE::
    ACk_EcsWorld_Actor_UE()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bTickEvenWhenPaused = false;
    bReplicates = false;
    bAlwaysRelevant = true;
}

auto
    ACk_EcsWorld_Actor_UE::
    Tick(
        float DeltaSeconds)
    -> void
{
    Super::Tick(DeltaSeconds);

    CK_ENSURE_IF_NOT(_Scheduler.IsSet() and ck::IsValid(_Registry),
        TEXT("EcsWorld Actor [{}] ticking without a valid scheduler or registry"), GetName())
    { return; }

    const auto TickStatCounter = FScopeCycleCounter{_TickStatId};

    // What a load lets this world do is ONE table, keyed on the phase the load is in — see Get_TickPlanForHold.
    // The two things it buys: feature processors stay frozen against a half-rebuilt world, and time-paced world
    // policy (population pacing, refills, timers, cadences) never observes one — a census read mid-rebuild is a
    // lie, and acting on it double-populates the world (the save-inflation incident, 2026-07-29). A construction
    // that wall-clock waits cannot finish under a load and orphans loudly: construction must not depend on time.
    const auto* Subsystem = DoGet_OwningSubsystem();
    const auto Plan = ck::Get_TickPlanForHold(
        Subsystem != nullptr ? Subsystem->Get_LoadHold() : ECk_EcsWorld_LoadHold::None,
        DeltaSeconds);

    _Scheduler->Tick(Plan._TickTime, _Registry, Plan._Scope);
}

auto
    ACk_EcsWorld_Actor_UE::
    DoGet_OwningSubsystem()
    -> const UCk_EcsWorld_Subsystem_UE*
{
    if (_OwningSubsystem.IsValid())
    { return _OwningSubsystem.Get(); }

    const auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return nullptr; }

    auto* Subsystem = World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
    _OwningSubsystem = Subsystem;
    return Subsystem;
}

auto
    ACk_EcsWorld_Actor_UE::
    Initialize(
        ck::FProcessorScheduler&& InScheduler,
        const FCk_Registry& InRegistry,
        ETickingGroup InTickGroup)
    -> void
{
    _Scheduler.Emplace(MoveTemp(InScheduler));
    _Registry = InRegistry;
    _UnrealTickingGroup = InTickGroup;

    _TickStatName = ck::Format_UE(TEXT("[{}] EcsScheduler_Actor"), _UnrealTickingGroup);
    _TickStatId = CK_CREATE_DYNAMIC_STAT_ID(STATGROUP_CkEcsWorldActor_Tick, _TickStatName);
    _EcsWorldDisplayName = FName{_TickStatName};

    SetTickGroup(_UnrealTickingGroup);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EcsWorld_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);

    _OwnedRegistry = MakeUnique<ck::registry_table::EnttRegistryType>();

    const auto RegistryHandle = ck::registry_table::Allocate(_OwnedRegistry.Get());

    const auto TransientEntityId = FCk_Entity{_OwnedRegistry->create()};

    _Registry = FCk_Registry{RegistryHandle};
    _Registry.SetContext<ck::FCtx_TransientEntity>(ck::FCtx_TransientEntity{TransientEntityId});

    _TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(_Registry);
    UCk_Utils_Handle_UE::Set_DebugName(_TransientEntity, TEXT("Transient Entity"));

    // Bind the world here, not at OnWorldBeginPlay: actor construction at PIE start can call
    // Request_SpawnEntity in the Initialize→OnWorldBeginPlay window, and a world-less transient
    // trips Get_WorldForEntity's "transient always has a valid world" invariant there.
    if (auto* World = GetWorld(); ck::IsValid(World))
    {
        _TransientEntity.Add<TWeakObjectPtr<UWorld>>(World);
    }
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Get_LoadHold() const
    -> ECk_EcsWorld_LoadHold
{
    return _LoadHold;
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Set_LoadHold(
        ECk_EcsWorld_LoadHold InHold)
    -> void
{
    _LoadHold = InHold;
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Get_IsLoadGateActive() const
    -> bool
{
    return _LoadHold != ECk_EcsWorld_LoadHold::None;
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Get_IsLoadGateEscalated() const
    -> bool
{
    return _LoadHold == ECk_EcsWorld_LoadHold::Escalated;
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Set_IsLoadGateActive(
        bool InActive)
    -> void
{
    _LoadHold = InActive ? ECk_EcsWorld_LoadHold::Rebuilding : ECk_EcsWorld_LoadHold::None;
}

auto
    UCk_EcsWorld_Subsystem_UE::
    DoGet_LoadHoldSeedProvider()
    -> FLoadHoldSeedProvider&
{
    static FLoadHoldSeedProvider Provider;
    return Provider;
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Set_LoadHoldSeedProvider(
        FLoadHoldSeedProvider InProvider)
    -> void
{
    DoGet_LoadHoldSeedProvider() = MoveTemp(InProvider);
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Clear_LoadHoldSeedProvider()
    -> void
{
    DoGet_LoadHoldSeedProvider() = FLoadHoldSeedProvider{};
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Get_IsInLoaderSpawnWindow() const
    -> bool
{
    return _LoaderSpawnWindowDepth > 0;
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Push_LoaderSpawnWindow()
    -> void
{
    ++_LoaderSpawnWindowDepth;
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Pop_LoaderSpawnWindow()
    -> void
{
    CK_ENSURE_IF_NOT(_LoaderSpawnWindowDepth > 0,
        TEXT("Pop_LoaderSpawnWindow with no window open — unbalanced FCk_ScopedLoaderSpawnWindow"))
    { return; }

    --_LoaderSpawnWindowDepth;
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Get_IsInRendezvousSpawnWindow() const
    -> bool
{
    return _RendezvousSpawnWindowDepth > 0;
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Push_RendezvousSpawnWindow()
    -> void
{
    ++_RendezvousSpawnWindowDepth;
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Pop_RendezvousSpawnWindow()
    -> void
{
    CK_ENSURE_IF_NOT(_RendezvousSpawnWindowDepth > 0,
        TEXT("Pop_RendezvousSpawnWindow with no window open — unbalanced FCk_ScopedRendezvousSpawnWindow"))
    { return; }

    --_RendezvousSpawnWindowDepth;
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Deinitialize()
        -> void
{
    for (auto& [TickGroup, Actor] : _WorldActors)
    {
        if (Actor.IsValid())
        {
            Actor->_Scheduler.Reset();
        }
    }

    _WorldActors.Reset();

    // Free the slot BEFORE destroying the registry: in between, a ghost handle resolves to null
    // rather than reaching freed memory.
    ck::registry_table::Free(_Registry.Get_RegistryHandle());

    _Registry        = FCk_Registry{};
    _TransientEntity = FCk_Handle{};
    _OwnedRegistry.Reset();

    Super::Deinitialize();
}

auto
    UCk_EcsWorld_Subsystem_UE::
    OnWorldBeginPlay(
        UWorld& InWorld) -> void
{
    Super::OnWorldBeginPlay(InWorld);

    // Backfill for the case where Initialize ran before GetWorld() was valid. Guarded because EnTT
    // emplace asserts on double-add (which also covers OnWorldBeginPlay re-entry on seamless travel).
    if (NOT _TransientEntity.Has<TWeakObjectPtr<UWorld>>())
    {
        _TransientEntity.Add<TWeakObjectPtr<UWorld>>(&InWorld);
    }

    // Before the graph, not after: a world that comes up mid-load must already be held when its first scheduler
    // actor ticks. WHICH phase is the provider's answer rather than this call's assumption — a world about to be
    // rebuilt and a world that only owes coherence need different scopes and different spawn admission. The seed
    // is bounded by the phase that owns it; whoever raised the hold releases it.
    if (const auto& SeedProvider = DoGet_LoadHoldSeedProvider();
        static_cast<bool>(SeedProvider))
    {
        if (const auto SeededHold = SeedProvider(InWorld);
            SeededHold != ECk_EcsWorld_LoadHold::None)
        {
            Set_LoadHold(SeededHold);
        }
    }

    DoBuildGraphAndSpawnActors(InWorld);
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Get_OnPreBuildProcessorGraph()
    -> FOnPreBuildProcessorGraph&
{
    static FOnPreBuildProcessorGraph Delegate;
    return Delegate;
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Request_RebuildProcessorGraph()
    -> void
{
    if (_PendingRebuildGraph)
    { return; }

    _PendingRebuildGraph = true;

    _OnEndFrameHandle = FCoreDelegates::OnEndFrame.AddWeakLambda(this, [this]()
    {
        OnEndFrame_DoRebuild();
    });
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Request_PumpToQuiescence(
        ck::ECk_SchedulerTickScope InScope)
    -> int32
{
    auto TotalPumpCount = int32{0};
    auto SkippedGroupCount = int32{0};

    // ASCENDING tick-group order, not TMap iteration order. FGroup_Overlap is the only TG_PostPhysics group and
    // therefore lives in its own world actor, while the granted physics steps run in the TG_PrePhysics one; a map
    // layout that visited PostPhysics first would recompute probe overlaps against the PREVIOUS frame's poses and
    // then report the overlap queue drained on a frame whose overlaps had not been recomputed at all. That is
    // deterministic per build and unspecified by contract, which is the worst combination — the engine's own tick
    // groups already state the order, so use them.
    auto OrderedTickGroups = TArray<TEnumAsByte<ETickingGroup>>{};
    _WorldActors.GenerateKeyArray(OrderedTickGroups);
    OrderedTickGroups.Sort([](const TEnumAsByte<ETickingGroup>& InA, const TEnumAsByte<ETickingGroup>& InB) -> bool
    { return InA.GetValue() < InB.GetValue(); });

    for (const auto& TickGroup : OrderedTickGroups)
    {
        const auto& Actor = _WorldActors[TickGroup];

        if (NOT Actor.IsValid())
        { continue; }

        if (NOT Actor->_Scheduler.IsSet())
        { continue; }

        if (Actor->_Scheduler->Get_IsTickInProgress())
        {
            ++SkippedGroupCount;

            // A load pumps deliberately, every frame, for as long as the world takes to converge — so under a
            // hold this is expected traffic rather than a re-entrant surprise, and one Warning per group per
            // frame would bury the log (and the AutoTest runner escalates Warnings to failures). The skip is
            // still COUNTED either way; Get_LastPumpSkippedGroupCount is what a convergence read must consult.
            if (Get_IsLoadGateActive())
            {
                ck::ecs::Verbose(TEXT("Request_PumpToQuiescence skipping tick group [{}]: a Tick is already in progress (re-entrant pump)"),
                    TickGroup);
            }
            else
            {
                ck::ecs::Warning(TEXT("Request_PumpToQuiescence skipping tick group [{}]: a Tick is already in progress (re-entrant pump)"),
                    TickGroup);
            }
            continue;
        }

        Actor->_Scheduler->Tick(FCk_Time{0.0}, _Registry, InScope);
        TotalPumpCount += Actor->_Scheduler->Get_LastFramePumpCount();
    }

    _LastPumpSkippedGroupCount = SkippedGroupCount;

    return TotalPumpCount;
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Get_LastPumpSkippedGroupCount() const
    -> int32
{
    return _LastPumpSkippedGroupCount;
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Get_WorstFramePumpCount() const
    -> int32
{
    auto Worst = int32{0};
    for (const auto& [TickGroup, Actor] : _WorldActors)
    {
        if (NOT Actor.IsValid())
        { continue; }

        const auto& SchedulerOpt = Actor->Get_Scheduler();
        if (NOT SchedulerOpt.IsSet())
        { continue; }

        Worst = FMath::Max(Worst, SchedulerOpt.GetValue().Get_LastFramePumpCount());
    }
    return Worst;
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Get_MaxPumpIterations() const
    -> int32
{
    auto MaxIterations = int32{0};
    for (const auto& [TickGroup, Actor] : _WorldActors)
    {
        if (NOT Actor.IsValid())
        { continue; }

        const auto& SchedulerOpt = Actor->Get_Scheduler();
        if (NOT SchedulerOpt.IsSet())
        { continue; }

        MaxIterations = FMath::Max(MaxIterations, SchedulerOpt.GetValue().Get_MaxPumpIterations());
    }
    return MaxIterations;
}

auto
    UCk_EcsWorld_Subsystem_UE::
    OnEndFrame_DoRebuild()
    -> void
{
    FCoreDelegates::OnEndFrame.Remove(_OnEndFrameHandle);
    _OnEndFrameHandle.Reset();
    _PendingRebuildGraph = false;

    auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return; }

    DoTeardownAndRebuild(*World);
}

auto
    UCk_EcsWorld_Subsystem_UE::
    DoTeardownAndRebuild(
        UWorld& InWorld)
    -> void
{
    ck::ecs::Verbose(TEXT("Rebuilding ECS processor graph..."));

    // Reset schedulers first so FProcessor_ScriptQueryHosted destructors fire EndPlay on script instances.
    for (auto& [TickGroup, Actor] : _WorldActors)
    {
         if (ck::IsValid(Actor))
        {
            Actor->_Scheduler.Reset();
        }
    }

    for (auto& [TickGroup, Actor] : _WorldActors)
    {
        if (ck::IsValid(Actor))
        {
            Actor->Destroy();
        }
    }
    _WorldActors.Reset();

    DoBuildGraphAndSpawnActors(InWorld);

    ck::ecs::Verbose(TEXT("ECS processor graph rebuilt."));
}

auto
    UCk_EcsWorld_Subsystem_UE::
    DoBuildGraphAndSpawnActors(
        UWorld& InWorld)
    -> void
{
    // Last chance for external modules (notably CkDynamic) to inject script-defined descriptors before
    // the graph builder snapshots the list.
    Get_OnPreBuildProcessorGraph().Broadcast(InWorld);

    const auto& Descriptors = ck::FProcessorRegistry::Get().Get_AllDescriptors();

    CK_ENSURE_IF_NOT(NOT Descriptors.IsEmpty(),
        TEXT("No processors registered in FProcessorRegistry. ECS Pipeline will NOT work."))
    { return; }

    auto GraphBuilder = ck::FProcessorGraphBuilder{};
    auto Graph = GraphBuilder.Build(
        Descriptors,
        _Registry,
        _TransientEntity,
        ECk_UnresolvedRefPolicy::Permissive,
        ECk_ProcessorWorldTypeContext::Runtime);

    ck::ecs::Verbose(TEXT("Processor graph built. Partitions: [{}]"), Graph._Partitions.Num());

    const auto& EcsWorldActorClass = ACk_EcsWorld_Actor_UE::StaticClass();

    for (auto& [TickGroup, Partition] : Graph._Partitions)
    {
        if (Partition._Nodes.IsEmpty())
        { continue; }

        auto Scheduler = ck::FProcessorScheduler{MoveTemp(Partition)};

        const auto ActorName = ck::Format_UE(TEXT("[{}] EcsScheduler_Actor"), TickGroup);

        auto WorldActor = Cast<ACk_EcsWorld_Actor_UE>
        (
            UCk_Utils_Actor_UE::Request_SpawnActor
            (
                FCk_Utils_Actor_SpawnActor_Params{&InWorld, EcsWorldActorClass}
                .Set_Label(ActorName)
                .Set_SpawnPolicy(ECk_Utils_Actor_SpawnActorPolicy::CannotSpawnInPersistentLevel),
                [&](AActor* InActor)
                {
                    auto NewWorldActor = Cast<ACk_EcsWorld_Actor_UE>(InActor);
                    NewWorldActor->Initialize(MoveTemp(Scheduler), _Registry, TickGroup);
                }
            )
        );

        CK_ENSURE_IF_NOT(ck::IsValid(WorldActor),
            TEXT("Failed to spawn EcsScheduler Actor for tick group [{}]"), TickGroup)
        { continue; }

        _WorldActors.Add(TickGroup, TStrongObjectPtr{WorldActor});

        ck::ecs::Verbose(TEXT("Spawned EcsScheduler Actor [{}] for tick group [{}]"),
            ActorName, TickGroup);
    }

    CK_ENSURE_IF_NOT(NOT _WorldActors.IsEmpty(),
        TEXT("Failed to spawn ANY EcsScheduler Actors. ECS Pipeline will NOT work."))
    { return; }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EcsWorld_Subsystem_UE::
    Get_TransientEntity(
        const UWorld* InWorld)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorld),
        TEXT("Unable to get the EcsSubsystem to get the TransientEntity as the World is [{}]"), InWorld)
    { return {}; }

    if (const auto& RuntimeSubsystem = InWorld->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
        ck::IsValid(RuntimeSubsystem))
    {
        return RuntimeSubsystem->Get_TransientEntity();
    }

    if (const auto& EditorSubsystem = InWorld->GetSubsystem<UCk_EditorEcsWorld_Subsystem_UE>();
        ck::IsValid(EditorSubsystem))
    {
        return EditorSubsystem->Get_TransientEntity();
    }

    CK_TRIGGER_ENSURE(
        TEXT("No Ecs subsystem on World [{}]. Get_TransientEntity called too early or for an unsupported world type."),
        InWorld);
    return {};
}

auto
    UCk_Utils_EcsWorld_Subsystem_UE::
    Get_TransientEntity_FromContextObject(
        const UObject* InWorldContextObject)
    -> FCk_Handle
{
    if (ck::Is_NOT_Valid(InWorldContextObject))
    { return {}; }

    return Get_TransientEntity(InWorldContextObject->GetWorld());
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_ecs_world_subsystem
{
#if !UE_BUILD_SHIPPING
    auto
        DoVisitSchedulerFrameSnapshots(
            const UObject* InWorldContextObject,
            int64 InFrameNumber,
            const TFunctionRef<void(const ck::FSchedulerDebug_FrameSnapshot&)>& InVisitor)
        -> void
    {
        if (ck::Is_NOT_Valid(InWorldContextObject))
        { return; }

        const auto* World = InWorldContextObject->GetWorld();
        if (ck::Is_NOT_Valid(World))
        { return; }

        const auto* Subsystem = World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
        if (ck::Is_NOT_Valid(Subsystem))
        { return; }

        for (const auto& [TickGroup, Actor] : Subsystem->Get_WorldActors())
        {
            if (NOT Actor.IsValid())
            { continue; }

            const auto& SchedulerOpt = Actor->Get_Scheduler();
            if (NOT SchedulerOpt.IsSet())
            { continue; }

            for (const auto& Snapshot : SchedulerOpt.GetValue().Get_DebugFrameHistory())
            {
                if (Snapshot.FrameNumber != static_cast<uint64>(InFrameNumber))
                { continue; }

                InVisitor(Snapshot);
            }
        }
    }
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EcsWorld_Subsystem_UE::
    Get_Debug_ProcessorPumpCountForFrame(
        const UObject* InWorldContextObject,
        FName InProcessorName,
        int64 InFrameNumber)
    -> int32
{
    auto Result = int32{-1};

#if !UE_BUILD_SHIPPING
    ck_ecs_world_subsystem::DoVisitSchedulerFrameSnapshots(InWorldContextObject, InFrameNumber,
        [&](const ck::FSchedulerDebug_FrameSnapshot& InSnapshot) -> void
        {
            for (const auto& Timing : InSnapshot.ProcessorTimings)
            {
                if (Timing.ProcessorName != InProcessorName)
                { continue; }

                Result = FMath::Max(Result, Timing.PumpCountThisFrame);
            }
        });
#endif

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EcsWorld_Subsystem_UE::
    Get_Debug_ProcessorMainPassEntityCountForFrame(
        const UObject* InWorldContextObject,
        FName InProcessorName,
        int64 InFrameNumber)
    -> int32
{
    auto Result = int32{-1};

#if !UE_BUILD_SHIPPING
    ck_ecs_world_subsystem::DoVisitSchedulerFrameSnapshots(InWorldContextObject, InFrameNumber,
        [&](const ck::FSchedulerDebug_FrameSnapshot& InSnapshot) -> void
        {
            for (const auto& Timing : InSnapshot.ProcessorTimings)
            {
                if (Timing.ProcessorName != InProcessorName)
                { continue; }

                Result = FMath::Max(Result, Timing.MainPassEntityCount);
            }
        });
#endif

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EcsWorld_Subsystem_UE::
    Get_Debug_FramePumpIterationCount(
        const UObject* InWorldContextObject,
        int64 InFrameNumber)
    -> int32
{
    auto Result = int32{-1};

#if !UE_BUILD_SHIPPING
    ck_ecs_world_subsystem::DoVisitSchedulerFrameSnapshots(InWorldContextObject, InFrameNumber,
        [&](const ck::FSchedulerDebug_FrameSnapshot& InSnapshot) -> void
        {
            Result = FMath::Max(Result, InSnapshot.PumpIterationCount);
        });
#endif

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EcsWorld_Subsystem_UE::
    Get_Debug_FrameLocalSettlePassCount(
        const UObject* InWorldContextObject,
        int64 InFrameNumber)
    -> int32
{
    auto Result = int32{-1};

#if !UE_BUILD_SHIPPING
    ck_ecs_world_subsystem::DoVisitSchedulerFrameSnapshots(InWorldContextObject, InFrameNumber,
        [&](const ck::FSchedulerDebug_FrameSnapshot& InSnapshot) -> void
        {
            Result = FMath::Max(Result, InSnapshot.LocalSettlePassCount);
        });
#endif

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

#if !UE_BUILD_SHIPPING

// Render the output with: `dot -Tsvg SchedulerGraph.dot -o SchedulerGraph.svg`
static auto
DoHandleExportSchedulerGraphCommand(
    const TArray<FString>& InArgs,
    UWorld* InWorld)
    -> void
{
    if (ck::Is_NOT_Valid(InWorld))
    {
        ck::ecs::Warning(TEXT("Ck.Ecs.Scheduler.ExportGraph: no valid world context"));
        return;
    }

    const auto* Subsystem = InWorld->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
    if (ck::Is_NOT_Valid(Subsystem))
    {
        ck::ecs::Warning(TEXT("Ck.Ecs.Scheduler.ExportGraph: UCk_EcsWorld_Subsystem_UE not available"));
        return;
    }

    // The serializer expects the container shape FProcessorGraph uses internally, so copy each live
    // partition out of its owning scheduler.
    auto Partitions = TMap<TEnumAsByte<ETickingGroup>, ck::FProcessorGraphPartition>{};

    for (const auto& [TickGroup, ActorPtr] : Subsystem->Get_WorldActors())
    {
        if (NOT ActorPtr.IsValid())
        { continue; }

        const auto& SchedulerOpt = ActorPtr->Get_Scheduler();
        if (NOT SchedulerOpt.IsSet())
        { continue; }

        Partitions.Add(TickGroup, SchedulerOpt.GetValue().Get_Partition());
    }

    if (Partitions.IsEmpty())
    {
        ck::ecs::Warning(TEXT("Ck.Ecs.Scheduler.ExportGraph: no live scheduler partitions"));
        return;
    }

    const auto DotContent = ck::DoSerializeProcessorGraphToDot(Partitions);

    auto OutputPath = InArgs.Num() > 0
        ? InArgs[0]
        : FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CkEcs"), TEXT("SchedulerGraph.dot"));

    if (FPaths::IsRelative(OutputPath))
    {
        OutputPath = FPaths::ConvertRelativePathToFull(OutputPath);
    }

    if (const auto& OutputDir = FPaths::GetPath(OutputPath);
        NOT OutputDir.IsEmpty())
    {
        constexpr auto Tree = true;
        IFileManager::Get().MakeDirectory(*OutputDir, Tree);
    }

    if (FFileHelper::SaveStringToFile(DotContent, *OutputPath))
    {
        ck::ecs::Display(TEXT("Ck.Ecs.Scheduler.ExportGraph: wrote [{}] bytes to [{}]"),
            DotContent.Len(), OutputPath);
    }
    else
    {
        ck::ecs::Warning(TEXT("Ck.Ecs.Scheduler.ExportGraph: failed to write [{}]"), OutputPath);
    }
}

static FAutoConsoleCommandWithWorldAndArgs GCk_ExportSchedulerGraphCommand(
    TEXT("Ck.Ecs.Scheduler.ExportGraph"),
    TEXT("Dumps the current ECS processor graph as Graphviz DOT. ")
    TEXT("Usage: Ck.Ecs.Scheduler.ExportGraph [optional path]. ")
    TEXT("Default path is <ProjectSaved>/CkEcs/SchedulerGraph.dot."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&DoHandleExportSchedulerGraphCommand));

// --------------------------------------------------------------------------------------------------------------------

static auto
DoHandleExportSchedulerOrderCommand(
    const TArray<FString>& InArgs,
    UWorld* InWorld)
    -> void
{
    if (ck::Is_NOT_Valid(InWorld))
    {
        ck::ecs::Warning(TEXT("Ck.Ecs.Scheduler.ExportOrder: no valid world context"));
        return;
    }

    const auto* Subsystem = InWorld->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
    if (ck::Is_NOT_Valid(Subsystem))
    {
        ck::ecs::Warning(TEXT("Ck.Ecs.Scheduler.ExportOrder: UCk_EcsWorld_Subsystem_UE not available"));
        return;
    }

    auto Partitions = TMap<TEnumAsByte<ETickingGroup>, ck::FProcessorGraphPartition>{};

    for (const auto& [TickGroup, ActorPtr] : Subsystem->Get_WorldActors())
    {
        if (NOT ActorPtr.IsValid())
        { continue; }

        const auto& SchedulerOpt = ActorPtr->Get_Scheduler();
        if (NOT SchedulerOpt.IsSet())
        { continue; }

        Partitions.Add(TickGroup, SchedulerOpt.GetValue().Get_Partition());
    }

    if (Partitions.IsEmpty())
    {
        ck::ecs::Warning(TEXT("Ck.Ecs.Scheduler.ExportOrder: no live scheduler partitions"));
        return;
    }

    const auto OrderContent = ck::DoSerializeProcessorExecutionOrder(Partitions);

    auto OutputPath = InArgs.Num() > 0
        ? InArgs[0]
        : FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CkEcs"), TEXT("SchedulerOrder.txt"));

    if (FPaths::IsRelative(OutputPath))
    {
        OutputPath = FPaths::ConvertRelativePathToFull(OutputPath);
    }

    if (const auto& OutputDir = FPaths::GetPath(OutputPath);
        NOT OutputDir.IsEmpty())
    {
        constexpr auto Tree = true;
        IFileManager::Get().MakeDirectory(*OutputDir, Tree);
    }

    if (FFileHelper::SaveStringToFile(OrderContent, *OutputPath))
    {
        ck::ecs::Display(TEXT("Ck.Ecs.Scheduler.ExportOrder: wrote [{}] bytes to [{}]"),
            OrderContent.Len(), OutputPath);
    }
    else
    {
        ck::ecs::Warning(TEXT("Ck.Ecs.Scheduler.ExportOrder: failed to write [{}]"), OutputPath);
    }
}

static FAutoConsoleCommandWithWorldAndArgs GCk_ExportSchedulerOrderCommand(
    TEXT("Ck.Ecs.Scheduler.ExportOrder"),
    TEXT("Dumps the final processor execution order (post-topological-sort) as plain text. ")
    TEXT("Usage: Ck.Ecs.Scheduler.ExportOrder [optional path]. ")
    TEXT("Default path is <ProjectSaved>/CkEcs/SchedulerOrder.txt."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&DoHandleExportSchedulerOrderCommand));

#endif // !UE_BUILD_SHIPPING

// --------------------------------------------------------------------------------------------------------------------
