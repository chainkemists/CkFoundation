#include "CkEcsEditor_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Net/CkNet_Fragment_Data.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Scheduler/CkProcessorGraph.h"
#include "CkEcs/Scheduler/CkProcessorRegistry.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EditorEcsWorld_Subsystem_UE::
    ShouldCreateSubsystem(
        UObject* InOuter) const
    -> bool
{
    if (NOT Super::ShouldCreateSubsystem(InOuter))
    { return false; }

    if (ck::Is_NOT_Valid(InOuter))
    { return false; }

    const auto* World = InOuter->GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return false; }

    // Only create for editor-authoring worlds — never for Game/PIE/Editor Preview (meshes/thumbnails).
    // The runtime UCk_EcsWorld_Subsystem_UE is responsible for Game/PIE worlds.
    return World->WorldType == EWorldType::Editor;
}

auto
    UCk_EditorEcsWorld_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);

    _TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(_Registry);
    UCk_Utils_Handle_UE::Set_DebugName(_TransientEntity, TEXT("Transient Entity (Editor)"));

    auto* World = GetWorld();
    if (ck::IsValid(World))
    {
        _TransientEntity.Add<TWeakObjectPtr<UWorld>>(World);
    }

    // Editor entities never replicate — stamp the transient entity with a self-consistent
    // net-params set so processors that only check authority still run where they should.
    UCk_Utils_Net_UE::Add(_TransientEntity, FCk_Net_ConnectionSettings{
        ECk_Replication::DoesNotReplicate,
        ECk_Net_NetModeType::Host,
        ECk_Net_EntityNetRole::Authority});

    DoBuildGraphAndSchedulers();
}

auto
    UCk_EditorEcsWorld_Subsystem_UE::
    Deinitialize()
    -> void
{
    if (_OnEndFrameHandle.IsValid())
    {
        FCoreDelegates::OnEndFrame.Remove(_OnEndFrameHandle);
        _OnEndFrameHandle.Reset();
    }

    DoTeardownSchedulers();

    _Registry.Shutdown();

    _TransientEntity = FCk_Handle{};
    _Registry = FCk_Registry{};

    Super::Deinitialize();
}

auto
    UCk_EditorEcsWorld_Subsystem_UE::
    Tick(
        float DeltaTime)
    -> void
{
    Super::Tick(DeltaTime);

    const auto DeltaT = FCk_Time{DeltaTime};

    for (auto& SchedulerOpt : _Schedulers)
    {
        if (NOT SchedulerOpt.IsSet())
        { continue; }

        SchedulerOpt->Tick(DeltaT, _Registry);
    }
}

auto
    UCk_EditorEcsWorld_Subsystem_UE::
    GetStatId() const
    -> TStatId
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UCk_EditorEcsWorld_Subsystem_UE, STATGROUP_Tickables);
}


auto
    UCk_EditorEcsWorld_Subsystem_UE::
    Request_SpawnEditorEntity(
        UCk_EntityScript_UE* InScriptArchetype)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InScriptArchetype),
        TEXT("Cannot spawn editor entity with invalid script archetype"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(_TransientEntity),
        TEXT("Editor transient entity is invalid — subsystem not initialized?"))
    { return {}; }

    // Use the same spawn primitive as the runtime path. The pending-entity wraps the real entity
    // that will be driven through Construct() by FProcessor_EntityScript_SpawnEntity_HandleRequests
    // on the editor graph's next tick.
    auto PendingEntity = UCk_Utils_EntityScript_UE::Request_SpawnEntity_Archetype(
        _TransientEntity,
        InScriptArchetype,
        FInstancedStruct{});

    auto NewEntity = PendingEntity.Get_EntityUnderConstruction();

    // Stamp the editor-only tag synchronously so editor-only processors see the entity from the
    // very first tick — even before Construct() has finished.
    if (ck::IsValid(NewEntity))
    {
        NewEntity.Add<ck::FTag_EditorOnlyEntity>();
    }

    return NewEntity;
}

auto
    UCk_EditorEcsWorld_Subsystem_UE::
    Request_DestroyEditorEntity(
        FCk_Handle& InHandle)
    -> void
{
    if (ck::Is_NOT_Valid(InHandle))
    { return; }

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
}

auto
    UCk_EditorEcsWorld_Subsystem_UE::
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
    UCk_EditorEcsWorld_Subsystem_UE::
    OnEndFrame_DoRebuild()
    -> void
{
    FCoreDelegates::OnEndFrame.Remove(_OnEndFrameHandle);
    _OnEndFrameHandle.Reset();
    _PendingRebuildGraph = false;

    ck::ecs::Verbose(TEXT("Rebuilding editor ECS processor graph..."));

    DoTeardownSchedulers();
    DoBuildGraphAndSchedulers();

    ck::ecs::Verbose(TEXT("Editor ECS processor graph rebuilt."));
}

auto
    UCk_EditorEcsWorld_Subsystem_UE::
    DoBuildGraphAndSchedulers()
    -> void
{
    const auto& Descriptors = ck::FProcessorRegistry::Get().Get_AllDescriptors();

    if (Descriptors.IsEmpty())
    {
        ck::ecs::Warning(TEXT("No processors registered. Editor ECS pipeline will NOT work."));
        return;
    }

    // Give the same pre-build hook that runtime uses so modules like CkDynamic can inject
    // script-defined processors before the graph builder snapshots the descriptor list.
    if (auto* World = GetWorld(); ck::IsValid(World))
    {
        UCk_EcsWorld_Subsystem_UE::Get_OnPreBuildProcessorGraph().Broadcast(*World);
    }

    auto GraphBuilder = ck::FProcessorGraphBuilder{};
    auto Graph = GraphBuilder.Build(
        Descriptors,
        _Registry,
        _TransientEntity,
        ECk_UnresolvedRefPolicy::Permissive,
        ECk_ProcessorWorldTypeContext::Editor);

    ck::ecs::Verbose(TEXT("Editor processor graph built. Partitions: [{}]"), Graph._Partitions.Num());

    _Schedulers.Reset();
    _Schedulers.Reserve(Graph._Partitions.Num());

    for (auto& [TickGroup, Partition] : Graph._Partitions)
    {
        if (Partition._Nodes.IsEmpty())
        { continue; }

        _Schedulers.Emplace(ck::FProcessorScheduler{MoveTemp(Partition)});
    }
}

auto
    UCk_EditorEcsWorld_Subsystem_UE::
    DoTeardownSchedulers()
    -> void
{
    for (auto& SchedulerOpt : _Schedulers)
    {
        SchedulerOpt.Reset();
    }
    _Schedulers.Reset();
}

// --------------------------------------------------------------------------------------------------------------------
