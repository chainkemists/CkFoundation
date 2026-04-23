#include "CkEntitySpawner_Actor.h"

#include "CkEntitySpawner/CkEntitySpawner_Log.h"

#include "CkActorRelay/CkActorRelay_Fragment_Data.h"
#include "CkActorRelay/CkActorRelay_Utils.h"

#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Subsystem/CkEcsEditor_Subsystem.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include <Components/BillboardComponent.h>
#include <Components/SceneComponent.h>
#include <Engine/World.h>

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

    // Any property touched — including nested Instanced EntityScript property changes — triggers a
    // rebuild so the live editor entity reflects the new authored state.
    EditorOnly_RebuildEntity();
}

auto
    ACk_EntitySpawner_UE::
    PostEditUndo()
    -> void
{
    Super::PostEditUndo();

    // The actor-side properties are transactional; the ECS entity is not. Rebuild so the entity's
    // fragments are consistent with the reverted property state.
    EditorOnly_RebuildEntity();
}

auto
    ACk_EntitySpawner_UE::
    Destroyed()
    -> void
{
    EditorOnly_DestroyEntity();
    Super::Destroyed();
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
    // Only rebuild for editor-authoring worlds — skip PIE, previews, and any other context where
    // the runtime path is responsible.
    auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return; }

    if (World->WorldType != EWorldType::Editor)
    { return; }

    EditorOnly_DestroyEntity();

    if (ck::Is_NOT_Valid(_EntityScript))
    { return; }

    auto* EditorSubsystem = World->GetSubsystem<UCk_EditorEcsWorld_Subsystem_UE>();
    if (ck::Is_NOT_Valid(EditorSubsystem))
    { return; }

    _EditorEntityHandle = EditorSubsystem->Request_SpawnEditorEntity(_EntityScript);
}

auto
    ACk_EntitySpawner_UE::
    EditorOnly_DestroyEntity()
    -> void
{
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

    _EditorEntityHandle = FCk_Handle{};
}
#endif

auto
    ACk_EntitySpawner_UE::
    DoSpawnEntity()
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(_EntityScript),
        TEXT("EntitySpawner [{}] has no EntityScript assigned."), this)
    { return; }

    const auto Replication = _EntityScript->Get_Replication();
    const auto IsReplicated = Replication == ECk_Replication::Replicates;

    if (IsReplicated)
    {
        if (NOT HasAuthority())
        { return; }

        CK_ENSURE_IF_NOT(_ReplicatedChannelGroup.IsValid(),
            TEXT("EntitySpawner [{}] has an invalid ReplicatedChannelGroup tag."), this)
        { return; }

        auto ChannelResult = UCk_Utils_ActorRelay_UE::Request_AcquireChannel(this, _ReplicatedChannelGroup);

        CK_ENSURE_IF_NOT(ck::IsValid(ChannelResult),
            TEXT("EntitySpawner [{}] failed to acquire ActorRelay channel for group [{}]."),
            this, _ReplicatedChannelGroup)
        { return; }

        auto LifetimeOwner = ChannelResult.Get_ChannelEntity();
        UCk_Utils_EntityScript_UE::Request_SpawnEntity_Archetype(LifetimeOwner, _EntityScript, FInstancedStruct{});
        return;
    }

    auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(GetWorld());

    CK_ENSURE_IF_NOT(ck::IsValid(TransientEntity),
        TEXT("EntitySpawner [{}] could not resolve the TransientEntity for the current world."), this)
    { return; }

    UCk_Utils_EntityScript_UE::Request_SpawnEntity_Archetype(TransientEntity, _EntityScript, FInstancedStruct{});
}

// --------------------------------------------------------------------------------------------------------------------
