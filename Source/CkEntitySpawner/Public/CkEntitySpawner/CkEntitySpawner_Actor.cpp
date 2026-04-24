#include "CkEntitySpawner_Actor.h"

#include "CkEntitySpawner/CkEntitySpawner_Log.h"

#include "CkActorRelay/CkActorRelay_Fragment_Data.h"
#include "CkActorRelay/CkActorRelay_Utils.h"

#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
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
        bool bFinished)
    -> void
{
    Super::PostEditMove(bFinished);
    EditorOnly_RebuildEntity();
}

auto
    ACk_EntitySpawner_UE::
    PostEditUndo()
    -> void
{
    Super::PostEditUndo();
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

    EditorOnly_DestroyEntity();

    if (ck::Is_NOT_Valid(_EntityScript))
    { return; }

    auto* EditorSubsystem = World->GetSubsystem<UCk_EditorEcsWorld_Subsystem_UE>();
    if (ck::Is_NOT_Valid(EditorSubsystem))
    { return; }

    DoInjectActorTransform();

    _EditorEntityHandle = EditorSubsystem->Request_SpawnEditorEntity(_EntityScript);
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

    _EditorEntityHandle = FCk_Handle{};
}
#endif

auto
    ACk_EntitySpawner_UE::
    DoInjectActorTransform()
    -> void
{
    if (ck::Is_NOT_Valid(_EntityScript))
    { return; }

    const auto PropertyName = _InjectActorTransformToScriptProperty.Get_PropertyName();
    if (PropertyName.IsNone())
    { return; }

    auto* Property = _EntityScript->GetClass()->FindPropertyByName(PropertyName);
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
        auto PendingEntity = UCk_Utils_EntityScript_UE::Request_SpawnEntity_Archetype(LifetimeOwner, _EntityScript, FInstancedStruct{});
        _RuntimeEntityHandle = PendingEntity.Get_EntityUnderConstruction();
        return;
    }

    auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(GetWorld());

    CK_ENSURE_IF_NOT(ck::IsValid(TransientEntity),
        TEXT("EntitySpawner [{}] could not resolve the TransientEntity for the current world."), this)
    { return; }

    auto PendingEntity = UCk_Utils_EntityScript_UE::Request_SpawnEntity_Archetype(TransientEntity, _EntityScript, FInstancedStruct{});
    _RuntimeEntityHandle = PendingEntity.Get_EntityUnderConstruction();
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
