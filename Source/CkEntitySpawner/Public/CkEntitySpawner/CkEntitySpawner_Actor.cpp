#include "CkEntitySpawner_Actor.h"

#include "CkEntitySpawner/CkEntitySpawner_Fragment.h"
#include "CkEntitySpawner/CkEntitySpawner_Log.h"

#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"
#include "CkCore/Reflection/CkReflection_Utils.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"
#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Data.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Subsystem/CkEcsEditor_Subsystem.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include <Components/BillboardComponent.h>
#include <Components/SceneComponent.h>
#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::entityspawner
{
    auto
        TryResolveDefaultTransformProperty(
            const UClass* InEntityScriptClass)
        -> FProperty*
    {
        if (ck::Is_NOT_Valid(InEntityScriptClass))
        { return nullptr; }

        const auto TryFind = [InEntityScriptClass](FName InName) -> FProperty*
        {
            auto* Property = InEntityScriptClass->FindPropertyByName(InName);
            if (ck::Is_NOT_Valid(Property, ck::IsValid_Policy_NullptrOnly{}))
            { return nullptr; }

            const auto* StructProp = CastField<FStructProperty>(Property);
            if (ck::Is_NOT_Valid(StructProp, ck::IsValid_Policy_NullptrOnly{}))
            { return nullptr; }

            if (StructProp->Struct != TBaseStructure<FTransform>::Get())
            { return nullptr; }

            return Property;
        };

        if (auto* Public = TryFind(FName{TEXT("SpawnTransform")}); ck::IsValid(Public, ck::IsValid_Policy_NullptrOnly{}))
        { return Public; }

        return TryFind(FName{TEXT("_SpawnTransform")});
    }
}

// --------------------------------------------------------------------------------------------------------------------

ACk_EntitySpawner_UE::
    ACk_EntitySpawner_UE()
{
    PrimaryActorTick.bCanEverTick = false;
    PrimaryActorTick.bStartWithTickEnabled = false;
    bReplicates = false;
    bAlwaysRelevant = false;
    SetCanBeDamaged(false);

    _ReplicatedChannelGroup = UCk_Utils_GameplayTag_UE::ResolveGameplayTag(TEXT("ActorRelay.Generic"));

    auto SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

#if WITH_EDITORONLY_DATA
    if (auto Sprite = GetSpriteComponent();
        ck::IsValid(Sprite))
    {
        Sprite->SetupAttachment(RootComponent);
    }
#endif
}

auto
    ACk_EntitySpawner_UE::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    DoSpawnEntity();
}

auto
    ACk_EntitySpawner_UE::
    EndPlay(
        const EEndPlayReason::Type EndPlayReason)
    -> void
{
    DoDestroyRuntimeEntity();
#if WITH_EDITOR
    EditorOnly_DestroyEntity();
#endif
    Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
auto
    ACk_EntitySpawner_UE::
    PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent)
    -> void
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    EditorOnly_RebuildEntity();
}

auto
    ACk_EntitySpawner_UE::
    PostEditMove(
        bool InIsFinished)
    -> void
{
    Super::PostEditMove(InIsFinished);

    // Transform-only edit (drag in viewport): push the new transform onto the existing editor
    // entity instead of rebuilding. Full rebuild is reserved for PostEditChangeProperty (non-
    // transform field changes) and PostEditUndo. The in-drag (InIsFinished=false) and on-drop
    // (InIsFinished=true) paths share the in-place writer.
    EditorOnly_PushActorTransformToEntity();
}

auto
    ACk_EntitySpawner_UE::
    PreEditUndo()
    -> void
{
    // Ctrl+Z of a placement runs here BEFORE the transaction system invalidates the actor's
    // subobject chain (the EntityScript and any UStaticMeshComponents created with the
    // EntityScript as Outer). If we wait until Destroyed() or PostEditUndo() to enqueue the
    // editor-entity destroy, the components are already marked-pending-kill — the cascade's
    // UnrealComponent_EndPlay processor finds an INVALID UObject and UnregisterComponent()
    // never runs, leaving a dead render proxy in the world (the visual remnant).
    //
    // Enqueuing the destroy here gives the EndPlay processors a window where components are
    // still valid. The Delete path doesn't need this hook because Delete fires Destroyed()
    // while components are still valid.
    EditorOnly_DestroyEntity();

    Super::PreEditUndo();
}

auto
    ACk_EntitySpawner_UE::
    PostEditUndo()
    -> void
{
    Super::PostEditUndo();

    // PostEditUndo fires twice per Ctrl+Z/Ctrl+Y round trip:
    //   (a) on the now-pending-kill actor right after a placement-undo, and
    //   (b) on the restored actor after a placement-redo.
    //
    // Case (a) queues an OnEndFrame weak lambda that becomes a no-op when it fires
    // (because the actor is pending-kill). The lambda expires itself, but our
    // _EditorRebuildPending flag stays true with no one to clear it. Then on Ctrl+Y
    // case (b) fires, but RebuildEntity's "is rebuild already pending?" early-out
    // sees the stale true and bails — the redo never spawns a fresh entity.
    //
    // Reset the pending state here so case (b) starts clean.
    if (_EditorRebuildEndFrameHandle.IsValid())
    {
        FCoreDelegates::OnEndFrame.Remove(_EditorRebuildEndFrameHandle);
        _EditorRebuildEndFrameHandle.Reset();
    }
    _EditorRebuildPending = false;

    EditorOnly_RebuildEntity();
}

auto
    ACk_EntitySpawner_UE::
    Destroyed()
    -> void
{
    // Delete / level-unload path. At this point the EntityScript and its created components
    // are still valid, so enqueuing the destroy lets the EndPlay processors clean them up on
    // subsequent scheduler ticks. The Ctrl+Z path goes through PreEditUndo() instead because
    // by the time Destroyed() fires on a placement-undo the transaction has already
    // invalidated the actor's subobjects.
    EditorOnly_DestroyEntity();
    Super::Destroyed();
}

auto
    ACk_EntitySpawner_UE::
    PushSelectionToProxies()
    -> void
{
    Super::PushSelectionToProxies();

    ck::editor_selection_owner::PushOwnerSelectionToProxies(this);
}

auto
    ACk_EntitySpawner_UE::
    EditorOnly_InitializeEntityScript(
        TSubclassOf<UCk_EntityScript_UE> InEntityScriptClass)
    -> void
{
    if (ck::Is_NOT_Valid(InEntityScriptClass))
    { return; }

    _EntityScript = NewObject<UCk_EntityScript_UE>(this, InEntityScriptClass, NAME_None, RF_Transactional);
}

auto
    ACk_EntitySpawner_UE::
    EditorOnly_RebuildEntity()
    -> void
{
    // Defer to end-of-frame so rapid event chains (drag-preview PostEditMove, factory
    // OnLevelActorAdded, PECP on inline Instanced subobject edits) coalesce into a single
    // destroy+respawn pass. Running synchronously was racing the subsystem's scheduler tick
    // and corrupting registry component pools mid-iteration.
    if (_EditorRebuildPending)
    { return; }

    auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return; }

    if (World->WorldType != EWorldType::Editor)
    { return; }

    _EditorRebuildPending = true;
    _EditorRebuildEndFrameHandle = FCoreDelegates::OnEndFrame.AddWeakLambda(this, [this]()
    {
        EditorOnly_DoRebuildEntity();
    });
}

auto
    ACk_EntitySpawner_UE::
    EditorOnly_DoRebuildEntity()
    -> void
{
    FCoreDelegates::OnEndFrame.Remove(_EditorRebuildEndFrameHandle);
    _EditorRebuildEndFrameHandle.Reset();
    _EditorRebuildPending = false;

    auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return; }

    if (World->WorldType != EWorldType::Editor)
    { return; }

    auto* EditorSubsystem = World->GetSubsystem<UCk_EditorEcsWorld_Subsystem_UE>();
    if (ck::Is_NOT_Valid(EditorSubsystem))
    { return; }

    // PIE start/stop briefly tears down the editor ECS registry. Touching it now — the destroy
    // below (a deferred lifetime request), the in-flight-destroy handle queries, or the spawn —
    // resolves stale handles against a freed registry and floods the MessageLog with INVALID
    // REGISTRY DEBUG_NAME has-queries and INVALID archetype spawn ensures. Re-arm for the next
    // safe end-of-frame; mirrors the scheduler-tick guard in UCk_EditorEcsWorld_Subsystem_UE::Tick.
    if (NOT EditorSubsystem->Get_IsEditorEcsMutationSafe())
    {
        EditorOnly_RebuildEntity();
        return;
    }

    // If a previous rebuild's destroy hasn't finished walking the entity-lifetime phase
    // chain (Initiate -> EndPlay -> Teardown -> Await -> Finalize), don't pile a synchronous
    // spawn on top of it — re-arm for the next end-of-frame and let the destroy complete.
    // IncludePendingKill so the guard still trips during the Finalize tick (default IsValid
    // returns false there).
    if (ck::IsValid(_EditorEntityHandle, ck::IsValid_Policy_IncludePendingKill{}) &&
        (UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(_EditorEntityHandle, ECk_EntityLifetime_DestructionPhase::BeginDestroy) ||
         UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(_EditorEntityHandle, ECk_EntityLifetime_DestructionPhase::Teardown) ||
         UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(_EditorEntityHandle, ECk_EntityLifetime_DestructionPhase::Destroyed)))
    {
        EditorOnly_RebuildEntity();
        return;
    }

    EditorOnly_DestroyEntity();

    if (ck::Is_NOT_Valid(_EntityScript))
    { return; }

    // During drag-drop and reinstancing the script's class can briefly be a UE placeholder
    // (SKEL_/REINST_/TRASHCLASS_/HOTRELOADED_) before the real class is rebound. Spawning
    // from a placeholder yields an INVALID archetype and spams
    // "EntityScriptArchetype is INVALID. Cannot Spawn Entity". Silently skip — the next
    // end-of-frame pass runs with the real class.
    if (UCk_Utils_Reflection_UE::Is_PlaceholderClass(_EntityScript->GetClass()))
    { return; }

    DoInjectActorTransform();

    _EditorEntityHandle = EditorSubsystem->Request_SpawnEditorEntity(_EntityScript);

    // Stamp this spawner as the preview entity's selection owner: editor-world visuals created for
    // the entity (ISM instances, hosted scene components) host themselves on per-owner proxy actors
    // whose viewport clicks redirect selection to this actor — clicking the preview mesh then
    // selects/moves/deletes the spawner exactly like clicking its billboard.
    if (ck::IsValid(_EditorEntityHandle))
    { ck::editor_selection_owner::Set(_EditorEntityHandle, this); }
}

auto
    ACk_EntitySpawner_UE::
    EditorOnly_DestroyEntity()
    -> void
{
    if (_EditorRebuildEndFrameHandle.IsValid())
    {
        FCoreDelegates::OnEndFrame.Remove(_EditorRebuildEndFrameHandle);
        _EditorRebuildEndFrameHandle.Reset();
        _EditorRebuildPending = false;
    }

    if (ck::Is_NOT_Valid(_EditorEntityHandle))
    { return; }

    auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    {
        _EditorEntityHandle = FCk_Handle{};
        return;
    }

    if (auto* EditorSubsystem = World->GetSubsystem<UCk_EditorEcsWorld_Subsystem_UE>();
        ck::IsValid(EditorSubsystem))
    {
        EditorSubsystem->Request_DestroyEditorEntity(_EditorEntityHandle);
    }

    // Intentionally do NOT clear _EditorEntityHandle — the entity isn't actually erased from the
    // registry until its destruction phase chain completes (~4 ticks). EditorOnly_DoRebuildEntity
    // relies on this to detect "previous destroy still in flight" and re-arm rather than piling
    // a synchronous spawn on top. The handle becomes ck::Is_NOT_Valid naturally once the registry
    // erases the entity, and the next spawn overwrites it.
}

auto
    ACk_EntitySpawner_UE::
    EditorOnly_PushActorTransformToEntity()
    -> void
{
    if (ck::Is_NOT_Valid(_EditorEntityHandle))
    { return; }

    // Mid-destruction entities are excluded from the Transform processor's view; pushing a
    // transform request onto one would be wasted work. The InIsFinished=true call will rebuild.
    if (UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(_EditorEntityHandle, ECk_EntityLifetime_DestructionPhase::BeginDestroy) ||
        UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(_EditorEntityHandle, ECk_EntityLifetime_DestructionPhase::Teardown) ||
        UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(_EditorEntityHandle, ECk_EntityLifetime_DestructionPhase::Destroyed))
    { return; }

    if (NOT UCk_Utils_Transform_UE::Has(_EditorEntityHandle))
    { return; }

    auto TransformHandle = UCk_Utils_Transform_UE::Cast(_EditorEntityHandle);
    if (ck::Is_NOT_Valid(TransformHandle))
    { return; }

    UCk_Utils_Transform_UE::Request_SetTransform(
        TransformHandle,
        FCk_Request_Transform_SetTransform{GetActorTransform()});
}
#endif

auto
    ACk_EntitySpawner_UE::
    DoInjectActorTransform()
    -> void
{
    if (ck::Is_NOT_Valid(_EntityScript))
    { return; }

    const auto* ScriptClass = _EntityScript->GetClass();

    auto* Property = [&]() -> FProperty*
    {
        const auto PropertyName = _InjectActorTransformToScriptProperty.Get_PropertyName();
        if (PropertyName.IsNone())
        { return ck::entityspawner::TryResolveDefaultTransformProperty(ScriptClass); }

        return ScriptClass->FindPropertyByName(PropertyName);
    }();

    if (ck::Is_NOT_Valid(Property, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    const auto* StructProp = CastField<FStructProperty>(Property);
    if (ck::Is_NOT_Valid(StructProp, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    if (StructProp->Struct != TBaseStructure<FTransform>::Get())
    { return; }

    auto* Dest = StructProp->ContainerPtrToValuePtr<FTransform>(_EntityScript);
    *Dest = GetActorTransform();
}

auto
    ACk_EntitySpawner_UE::
    DoSpawnEntity()
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(_EntityScript),
        TEXT("EntitySpawner [{}] has no EntityScript assigned."), this)
    { return; }

    DoInjectActorTransform();

    if (const auto Replication = _EntityScript->Get_EffectiveReplication();
        Replication == ECk_Replication::Replicates && HasAuthority())
    {
        CK_ENSURE_IF_NOT(_ReplicatedChannelGroup.IsValid(),
            TEXT("EntitySpawner [{}] has an invalid ReplicatedChannelGroup tag."), this)
        { return; }

        // Defer the acquire-and-spawn via FProcessor_EntitySpawner_Spawn. The processor retries
        // each tick until the group pool has a ready channel (pool populated AND its companion
        // entity is ECS-ready). Keeps us off the synchronous acquire path that would ensure on
        // a same-frame race between the channel actor's BeginPlay and this one.
        auto PendingEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(this);

        CK_ENSURE_IF_NOT(ck::IsValid(PendingEntity),
            TEXT("EntitySpawner [{}] could not create a pending-spawn entity."), this)
        { return; }

        PendingEntity.Add<ck::FFragment_EntitySpawner_PendingSpawn>(_EntityScript, _ReplicatedChannelGroup);

        // Track the queue entity, not the payload: if this spawner is destroyed before the
        // processor runs, destroying the queue cancels the pending spawn. Once the processor
        // has spawned the payload (under the channel's lifetime) and destroyed the queue, the
        // handle becomes invalid and DoDestroyRuntimeEntity is a no-op — the payload correctly
        // outlives this spawner because it belongs to the channel's lifetime chain.
        _RuntimeEntityHandle = PendingEntity;
        return;
    }

    auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(GetWorld());

    CK_ENSURE_IF_NOT(ck::IsValid(TransientEntity),
        TEXT("EntitySpawner [{}] could not resolve the TransientEntity for the current world."), this)
    { return; }

    const auto PendingEntity = UCk_Utils_EntityScript_UE::Request_SpawnEntity_Archetype(TransientEntity, _EntityScript, FInstancedStruct{});
    _RuntimeEntityHandle = PendingEntity.Get_EntityUnderConstruction();
    if (ck::IsValid(_RuntimeEntityHandle))
    { UCk_Utils_ContextOwner_UE::Request_OverrideToSelf(_RuntimeEntityHandle); }
}

auto
    ACk_EntitySpawner_UE::
    DoDestroyRuntimeEntity()
    -> void
{
    if (ck::Is_NOT_Valid(_RuntimeEntityHandle))
    { return; }

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(_RuntimeEntityHandle);
    _RuntimeEntityHandle = FCk_Handle{};
}

// --------------------------------------------------------------------------------------------------------------------
