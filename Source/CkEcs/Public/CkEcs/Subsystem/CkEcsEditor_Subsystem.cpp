#include "CkEcsEditor_Subsystem.h"

#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Utils.h"
#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_HostActor.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Net/CkNet_Fragment_Data.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Scheduler/CkProcessorGraph.h"
#include "CkEcs/Scheduler/CkProcessorRegistry.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkEcs/Tag/CkTag_EditorOnly.h"

#include <Engine/World.h>

#if WITH_EDITOR
#include <Editor.h>
#endif

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

    return World->WorldType == EWorldType::Editor;
}

auto
    UCk_EditorEcsWorld_Subsystem_UE::
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
    UCk_Utils_Handle_UE::Set_DebugName(_TransientEntity, TEXT("Transient Entity (Editor)"));

    if (auto* World = GetWorld();
        ck::IsValid(World))
    {
        _TransientEntity.Add<TWeakObjectPtr<UWorld>>(World);
    }

    // ClientAndHost so both AuthorityOnly and CosmeticOnly net-mode gates pass — standalone
    // semantics, which match editor-time single-machine authoring.
    UCk_Utils_Net_UE::Add(_TransientEntity, FCk_Net_ConnectionSettings{
        ECk_Replication::DoesNotReplicate,
        ECk_Net_NetModeType::ClientAndHost,
        ECk_Net_EntityNetRole::Authority});

    // Cascaded by Request_SetupEntityWithLifetimeOwner so every descendant inherits the tag.
    _TransientEntity.Add<ck::FTag_EditorOnlyEntity>();

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

    // Free the slot BEFORE destroying the registry: in between, a ghost handle resolves to null
    // rather than reaching freed memory.
    ck::registry_table::Free(_Registry.Get_RegistryHandle());

    _Registry        = FCk_Registry{};
    _TransientEntity = FCk_Handle{};
    _OwnedRegistry.Reset();

    Super::Deinitialize();
}

auto
    UCk_EditorEcsWorld_Subsystem_UE::
    Tick(
        float DeltaTime)
    -> void
{
    Super::Tick(DeltaTime);

#if WITH_EDITOR
    // Skip scheduler work while PIE is active: dynamic-fragment iteration here can dereference handles
    // whose registry a previous PIE teardown destroyed, which EnTT turns into a hard crash. The redraw
    // request below is still serviced so editor viewports refresh normally.
    if (ck::IsValid(GEditor, ck::IsValid_Policy_NullptrOnly{}) && ck::IsValid(GEditor->PlayWorld))
    {
        if (_PendingRedraw)
        {
            UCk_Utils_EditorOnly_UE::Request_RedrawLevelEditingViewports();
            _PendingRedraw = false;
        }
        return;
    }
#endif

    const auto DeltaT = FCk_Time{DeltaTime};

    for (auto& SchedulerOpt : _Schedulers)
    {
        if (NOT SchedulerOpt.IsSet())
        { continue; }

        SchedulerOpt->Tick(DeltaT, _Registry);
    }

#if WITH_EDITOR
    if (_PendingRedraw)
    {
        UCk_Utils_EditorOnly_UE::Request_RedrawLevelEditingViewports();
        _PendingRedraw = false;
    }
#endif
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
    Get_IsEditorEcsMutationSafe() const
    -> bool
{
#if WITH_EDITOR
    // PIE active: editor-world handles can briefly outlive their registry across the PIE start/stop
    // transition. Same condition the scheduler-tick guard checks in Tick().
    if (ck::IsValid(GEditor, ck::IsValid_Policy_NullptrOnly{}) && ck::IsValid(GEditor->PlayWorld))
    { return false; }
#endif

    // Invalid whether the registry was freed or never built. A processor-graph rebuild does NOT free
    // the registry, so the transient stays valid there and spawning is not gated.
    if (ck::Is_NOT_Valid(_TransientEntity))
    { return false; }

    return true;
}

#if WITH_EDITOR
auto
    UCk_EditorEcsWorld_Subsystem_UE::
    Get_SelectionProxyHostActor(
        AActor* InSelectionOwner)
    -> AActor*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSelectionOwner),
        TEXT("Cannot get a SelectionProxyHost for an INVALID SelectionOwner"))
    { return nullptr; }

    const auto Key = TWeakObjectPtr<const AActor>{InSelectionOwner};

    if (const auto* MaybeFound = _SelectionProxyHostActors.Find(Key);
        MaybeFound != nullptr && MaybeFound->IsValid())
    { return MaybeFound->Get(); }

    auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return nullptr; }

    auto SpawnParams = FActorSpawnParameters{};
    SpawnParams.ObjectFlags = RF_Transient;
    SpawnParams.bHideFromSceneOutliner = true;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    auto* HostActor = World->SpawnActor<ACk_EditorSelectionProxyHost_Actor_UE>(
        ACk_EditorSelectionProxyHost_Actor_UE::StaticClass(), SpawnParams);

    if (ck::Is_NOT_Valid(HostActor))
    { return nullptr; }

    HostActor->_EditorSelectionOwner = InSelectionOwner;

    UCk_Utils_EditorSelectionOwner_UE::RegisterProxyActor(InSelectionOwner, HostActor);

    _SelectionProxyHostActors.Add(Key, HostActor);

    return HostActor;
}
#endif

auto
    UCk_EditorEcsWorld_Subsystem_UE::
    Request_SpawnEditorEntity(
        UCk_EntityScript_UE* InScriptArchetype)
    -> FCk_Handle
{
    // Never enqueue a spawn during the PIE-transition window — it would later resolve a stale
    // archetype against a torn-down registry. The spawner's rebuild path re-arms, so nothing is lost.
    if (NOT Get_IsEditorEcsMutationSafe())
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InScriptArchetype),
        TEXT("Cannot spawn editor entity with invalid script archetype"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(_TransientEntity),
        TEXT("Editor transient entity is invalid — subsystem not initialized?"))
    { return {}; }

    const auto PendingEntity = UCk_Utils_EntityScript_UE::Request_SpawnEntity_Archetype(
        _TransientEntity,
        InScriptArchetype,
        FInstancedStruct{});

    const auto NewEntity = PendingEntity.Get_EntityUnderConstruction();

    Request_Redraw();

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

    Request_Redraw();
}

auto
    UCk_EditorEcsWorld_Subsystem_UE::
    Request_Redraw()
    -> void
{
    _PendingRedraw = true;
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

    Request_Redraw();

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

    // Same pre-build hook the runtime uses, so CkDynamic can inject script-defined processors first.
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
