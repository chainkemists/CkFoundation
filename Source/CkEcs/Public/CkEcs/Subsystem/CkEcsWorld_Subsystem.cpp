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

// --------------------------------------------------------------------------------------------------------------------

DECLARE_STATS_GROUP(TEXT("CkEcsWorldActor_Tick"), STATGROUP_CkEcsWorldActor_Tick, STATCAT_Advanced);

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

    // The load kernel keeps feature processors frozen against the half-rebuilt world. An ESCALATED gate
    // runs the full scope while the loader still owns completion — multi-stage constructions (EntityScript
    // `Continue` fulfilled by game processors) are unfinishable under the kernel, and the rebuild may be
    // waiting on the identity they stamp on completion.
    auto Scope = ck::ECk_SchedulerTickScope::Full;
    if (const auto* Subsystem = DoGet_OwningSubsystem();
        Subsystem != nullptr and Subsystem->Get_IsLoadGateActive() and NOT Subsystem->Get_IsLoadGateEscalated())
    { Scope = ck::ECk_SchedulerTickScope::LoadKernel; }

    _Scheduler->Tick(FCk_Time{DeltaSeconds}, _Registry, Scope);
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
    Request_AdoptRestoredTransient(
        FCk_Entity InRestoredTransient)
    -> void
{
    CK_ENSURE_IF_NOT(_OwnedRegistry.IsValid(),
        TEXT("Request_AdoptRestoredTransient: no owned registry"))
    { return; }

    // entt's ctx().emplace (what FCk_Registry::SetContext uses) is try_emplace — it would NOT overwrite the stale
    // FCtx_TransientEntity left behind by registry.clear(). insert_or_assign overwrites unconditionally.
    _OwnedRegistry->ctx().insert_or_assign(ck::FCtx_TransientEntity{InRestoredTransient});

    _TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(_Registry);
    UCk_Utils_Handle_UE::Set_DebugName(_TransientEntity, TEXT("Transient Entity (restored)"));

    // Guarded because EnTT emplace asserts if the fragment is already present.
    if (NOT _TransientEntity.Has<TWeakObjectPtr<UWorld>>())
    {
        _TransientEntity.Add<TWeakObjectPtr<UWorld>>(GetWorld());
    }
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Get_IsLoadGateActive() const
    -> bool
{
    return _IsLoadGateActive;
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Set_IsLoadGateActive(
        bool InActive)
    -> void
{
    _IsLoadGateActive = InActive;

    if (NOT InActive)
    { _IsLoadGateEscalated = false; }
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Get_IsLoadGateEscalated() const
    -> bool
{
    return _IsLoadGateEscalated;
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Set_IsLoadGateEscalated(
        bool InEscalated)
    -> void
{
    _IsLoadGateEscalated = InEscalated;
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

    for (auto& [TickGroup, Actor] : _WorldActors)
    {
        if (NOT Actor.IsValid())
        { continue; }

        if (NOT Actor->_Scheduler.IsSet())
        { continue; }

        if (Actor->_Scheduler->Get_IsTickInProgress())
        {
            ck::ecs::Warning(TEXT("Request_PumpToQuiescence skipping tick group [{}]: a Tick is already in progress (re-entrant pump)"),
                TickGroup);
            continue;
        }

        Actor->_Scheduler->Tick(FCk_Time{0.0}, _Registry, InScope);
        TotalPumpCount += Actor->_Scheduler->Get_LastFramePumpCount();
    }

    return TotalPumpCount;
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
