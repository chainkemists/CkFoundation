#include "CkEntitySpawner_Actor.h"

#include "CkEntitySpawner/CkEntitySpawner_Fragment.h"
#include "CkEntitySpawner/CkEntitySpawner_Log.h"

#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"
#include "CkCore/Reflection/CkReflection_Utils.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"
#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Data.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Snapshot/CkSaveKey_Fragment.h"
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
            if (ck::Is_NOT_Valid(Property))
            { return nullptr; }

            const auto* StructProp = CastField<FStructProperty>(Property);
            if (ck::Is_NOT_Valid(StructProp))
            { return nullptr; }

            if (StructProp->Struct != TBaseStructure<FTransform>::Get())
            { return nullptr; }

            return Property;
        };

        if (auto* Public = TryFind(FName{TEXT("SpawnTransform")}); ck::IsValid(Public))
        { return Public; }

        if (auto* Private = TryFind(FName{TEXT("_SpawnTransform")}); ck::IsValid(Private))
        { return Private; }

        // Gym-authored scripts name it InitialTransform; unresolved, editor previews compose at world origin.
        if (auto* PublicInitial = TryFind(FName{TEXT("InitialTransform")}); ck::IsValid(PublicInitial))
        { return PublicInitial; }

        return TryFind(FName{TEXT("_InitialTransform")});
    }

    auto
        Get_IsDestroyInFlight(
            const FCk_Handle& InEntity)
        -> bool
    {
        return UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InEntity, ECk_EntityLifetime_DestructionPhase::BeginDestroy) ||
               UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InEntity, ECk_EntityLifetime_DestructionPhase::Teardown) ||
               UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InEntity, ECk_EntityLifetime_DestructionPhase::Destroyed);
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

    if (InIsFinished)
    {
        // Scripts that compose child entities at construct time have no scene-node link to the root,
        // so only a re-construct at the new injected transform re-anchors them.
        EditorOnly_RebuildEntity();
        return;
    }

    EditorOnly_PushActorTransformToEntity();
}

auto
    ACk_EntitySpawner_UE::
    PreEditUndo()
    -> void
{
    // Runs BEFORE the transaction system invalidates the actor's subobject chain, so the EndPlay
    // processors still see valid components. Destroyed()/PostEditUndo() would leave a dead render proxy.
    EditorOnly_DestroyEntity();

    Super::PreEditUndo();
}

auto
    ACk_EntitySpawner_UE::
    PostEditUndo()
    -> void
{
    Super::PostEditUndo();

    // Fires on the pending-kill actor after a placement-undo AND on the restored actor after the redo;
    // the first leaves _EditorRebuildPending stuck true, which would early-out the redo's rebuild.
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
    // Delete / level-unload: the EntityScript and its components are still valid here, so the EndPlay
    // processors can clean them up on later scheduler ticks. Placement-undo cannot — see PreEditUndo.
    EditorOnly_DestroyEntity();
    Super::Destroyed();
}

auto
    ACk_EntitySpawner_UE::
    PushSelectionToProxies()
    -> void
{
    Super::PushSelectionToProxies();

    UCk_Utils_EditorSelectionOwner_UE::PushOwnerSelectionToProxies(this);
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
    // Deferred to end-of-frame so rapid event chains (drag PostEditMove, OnLevelActorAdded, PECP on inline
    // Instanced edits) coalesce into one pass; running synchronously races the subsystem's scheduler tick.
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

    // PIE start/stop briefly tears down the editor ECS registry; touching it now resolves stale handles
    // against a freed registry. Re-arm for the next safe end-of-frame.
    if (NOT EditorSubsystem->Get_IsEditorEcsMutationSafe())
    {
        EditorOnly_RebuildEntity();
        return;
    }

    // IncludePendingKill so the guard still trips during the Finalize tick, where the default policy reports false.
    if (ck::IsValid(_EditorEntityHandle, ck::IsValid_Policy_IncludePendingKill{}) &&
        ck::entityspawner::Get_IsDestroyInFlight(_EditorEntityHandle))
    {
        EditorOnly_RebuildEntity();
        return;
    }

    EditorOnly_DestroyEntity();

    if (ck::Is_NOT_Valid(_EntityScript))
    { return; }

    // During drag-drop and reinstancing the class is briefly a UE placeholder and spawning from one ensures
    // on an INVALID archetype. Skipping is safe — the next end-of-frame pass runs with the real class.
    if (UCk_Utils_Reflection_UE::Is_PlaceholderClass(_EntityScript->GetClass()))
    { return; }

    DoInjectActorTransform();

    _EditorEntityHandle = EditorSubsystem->Request_SpawnEditorEntity(_EntityScript);

    // Editor-world visuals live on per-owner proxy actors; owning their selection makes a viewport
    // click on the preview mesh act on this spawner.
    if (ck::IsValid(_EditorEntityHandle))
    {
        UCk_Utils_EditorSelectionOwner_UE::Request_SetupEntityWithEditorSelectionOwner(_EditorEntityHandle, this);
        UCk_Utils_EditorSelectionOwner_UE::PushOwnerSelectionToProxies(this);
    }
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

    // Deliberately NOT clearing _EditorEntityHandle: the entity survives its destruction-phase chain
    // (~4 ticks) and EditorOnly_DoRebuildEntity relies on the still-set handle to detect that.
}

auto
    ACk_EntitySpawner_UE::
    EditorOnly_PushActorTransformToEntity()
    -> void
{
    if (ck::Is_NOT_Valid(_EditorEntityHandle))
    { return; }

    // Mid-destruction entities are excluded from the Transform processor's view; the InIsFinished=true call rebuilds.
    if (ck::entityspawner::Get_IsDestroyInFlight(_EditorEntityHandle))
    { return; }

    if (NOT UCk_Utils_Transform_UE::Has(_EditorEntityHandle))
    { return; }

    auto TransformHandle = UCk_Utils_Transform_UE::Cast(_EditorEntityHandle);
    if (ck::Is_NOT_Valid(TransformHandle))
    { return; }

    UCk_Utils_Transform_UE::Request_SetTransform(
        TransformHandle,
        FCk_Request_Transform_SetTransform{GetActorTransform()}, {});
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

    if (ck::Is_NOT_Valid(Property))
    { return; }

    const auto* StructProp = CastField<FStructProperty>(Property);
    if (ck::Is_NOT_Valid(StructProp))
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

    // A level-placed spawner re-creates the same entity on every boot, so keying that entity makes a load RENDEZVOUS
    // the saved state onto the fresh world's copy instead of rebuilding a second one beside it. The identity is the
    // SPAWNER's, not the script's: one spawner owns exactly one entity, and the spawner is what the level re-creates.
    // A runtime-spawned spawner gets no key — nothing about it is stable across boots.
    const auto SaveKeyIdentity = ck::save_key::Get_LevelPlacedIdentity(this);

    if (const auto Replication = _EntityScript->Get_EffectiveReplication();
        Replication == ECk_Replication::Replicates && HasAuthority())
    {
        CK_ENSURE_IF_NOT(_ReplicatedChannelGroup.IsValid(),
            TEXT("EntitySpawner [{}] has an invalid ReplicatedChannelGroup tag."), this)
        { return; }

        // FProcessor_EntitySpawner_Spawn retries each tick until the group pool has a ready channel; the
        // synchronous acquire path ensures on a same-frame race with the channel actor's BeginPlay.
        auto PendingEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(this);

        CK_ENSURE_IF_NOT(ck::IsValid(PendingEntity),
            TEXT("EntitySpawner [{}] could not create a pending-spawn entity."), this)
        { return; }

        PendingEntity.Add<ck::FFragment_EntitySpawner_PendingSpawn>(_EntityScript, _ReplicatedChannelGroup, SaveKeyIdentity);

        // Track the queue entity, not the payload: destroying this spawner beforehand cancels the pending
        // spawn, and once the payload is spawned under the channel's lifetime this handle goes invalid.
        _RuntimeEntityHandle = PendingEntity;
        return;
    }

    auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(GetWorld());

    CK_ENSURE_IF_NOT(ck::IsValid(TransientEntity),
        TEXT("EntitySpawner [{}] could not resolve the TransientEntity for the current world."), this)
    { return; }

    // A KEYED spawn is a rendezvous target — during a load the saved row waits to ADOPT this very entity, so
    // it must pass the load-gate spawn suppression. An unkeyed spawn stays suppressible: its saved row is
    // recipe-respawned by the loader, and admitting the world's copy would double it.
    auto EcsWorld = static_cast<UCk_EcsWorld_Subsystem_UE*>(nullptr);
    if (NOT SaveKeyIdentity.IsEmpty())
    { EcsWorld = GetWorld()->GetSubsystem<UCk_EcsWorld_Subsystem_UE>(); }
    const auto RendezvousWindow = FCk_ScopedRendezvousSpawnWindow{EcsWorld};

    const auto PendingEntity = UCk_Utils_EntityScript_UE::Request_SpawnEntity_Archetype(TransientEntity, _EntityScript, FInstancedStruct{}, {});
    _RuntimeEntityHandle = PendingEntity.Get_EntityUnderConstruction();
    if (ck::Is_NOT_Valid(_RuntimeEntityHandle))
    { return; }

    UCk_Utils_ContextOwner_UE::Request_OverrideToSelf(_RuntimeEntityHandle, {});

    if (NOT SaveKeyIdentity.IsEmpty())
    { ck::save_key::AssignLevelPlaced(_RuntimeEntityHandle, SaveKeyIdentity); }
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
