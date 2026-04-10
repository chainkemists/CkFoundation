#include "CkEcsWorld_Subsystem.h"

#include "CkCore/Actor/CkActor_Utils.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Scheduler/CkProcessorGraph.h"
#include "CkEcs/Scheduler/CkProcessorRegistry.h"

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

    CK_ENSURE_IF_NOT(_Scheduler.IsSet() and _Registry != nullptr,
        TEXT("EcsWorld Actor [{}] ticking without a valid scheduler or registry"), GetName())
    { return; }

    const auto TickStatCounter = FScopeCycleCounter{_TickStatId};

    _Scheduler->Tick(FCk_Time{DeltaSeconds}, *_Registry);
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
    _Registry = &InRegistry;
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

    _TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(_Registry);
    UCk_Utils_Handle_UE::Set_DebugName(_TransientEntity, TEXT("Transient Entity"));
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Deinitialize()
        -> void
{
    UE_LOG(LogTemp, Warning, TEXT("[CK_DEINIT] Deinitialize START. RefCount=[%d]"), _Registry.Debug_GetSharedRefCount());

    for (auto& [TickGroup, Actor] : _WorldActors)
    {
        if (Actor.IsValid())
        {
            Actor->_Scheduler.Reset();
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("[CK_DEINIT] After scheduler reset. RefCount=[%d]"), _Registry.Debug_GetSharedRefCount());

    // Break self-referential cycle: entities store FCk_Handle in fragments,
    // each handle holds a TSharedPtr back to this registry. Clear all entity
    // data first so the shared_ptr refcount can reach zero.
    _Registry.Shutdown();
    UE_LOG(LogTemp, Warning, TEXT("[CK_DEINIT] After registry Shutdown(). RefCount=[%d]"), _Registry.Debug_GetSharedRefCount());

    _WorldActors.Reset();
    _TransientEntity = FCk_Handle{};
    _Registry = FCk_Registry{};
    UE_LOG(LogTemp, Warning, TEXT("[CK_DEINIT] Deinitialize END"));

    Super::Deinitialize();
}

auto
    UCk_EcsWorld_Subsystem_UE::
    OnWorldBeginPlay(
        UWorld& InWorld) -> void
{
    Super::OnWorldBeginPlay(InWorld);

    DoBuildGraphAndSpawnActors(InWorld);
}

auto
    UCk_EcsWorld_Subsystem_UE::
    DoBuildGraphAndSpawnActors(
        UWorld& InWorld)
    -> void
{
    _TransientEntity.Add<TWeakObjectPtr<UWorld>>(&InWorld);

    const auto& Descriptors = ck::FProcessorRegistry::Get().Get_AllDescriptors();

    CK_ENSURE_IF_NOT(NOT Descriptors.IsEmpty(),
        TEXT("No processors registered in FProcessorRegistry. ECS Pipeline will NOT work."))
    { return; }

    auto GraphBuilder = ck::FProcessorGraphBuilder{};
    auto Graph = GraphBuilder.Build(
        Descriptors,
        _Registry,
        _TransientEntity);

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

    const auto& Subsystem = InWorld->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();

    CK_ENSURE_IF_NOT(ck::IsValid(Subsystem),
        TEXT("Unable to get the EcsSubsystem from the World [{}]. It's possible Get_TransientEntity is being called too early"),
        InWorld)
    { return {}; }

    return Subsystem->Get_TransientEntity();
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

// Console command: Ck.Ecs.Scheduler.ExportGraph [path]
//
// Dumps the current ECS processor graph (one subgraph cluster per tick group) as Graphviz DOT.
// If a path argument is supplied it's written relative to the project root (or treated as absolute
// if already absolute). When omitted, the file is written to <ProjectSaved>/CkEcs/SchedulerGraph.dot.
//
// Render with: `dot -Tsvg SchedulerGraph.dot -o SchedulerGraph.svg`
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

    // Reconstruct a partition map keyed by tick group from each world actor's scheduler. The
    // serializer expects the same container shape FProcessorGraph uses internally, so we copy
    // each live partition out of its owning scheduler into a temporary map.
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

    const auto OutputDir = FPaths::GetPath(OutputPath);
    if (NOT OutputDir.IsEmpty())
    {
        IFileManager::Get().MakeDirectory(*OutputDir, /*Tree=*/true);
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

#endif // !UE_BUILD_SHIPPING

// --------------------------------------------------------------------------------------------------------------------
