#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include <GameFramework/Info.h>

#include "CkEntitySpawner_Actor.generated.h"

class UCk_EntityScript_UE;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType,
    DisplayName = "Ck Entity Spawner",
    HideCategories = (Replication, Physics, Networking, Actor, Rendering, Collision, Input, LOD, HLOD, WorldPartition, DataLayers, Cooking, "Level Instance", Advanced, Tags, ComponentReplication, ComponentTick, Events),
    meta = (DisplayName = "Ck Entity Spawner"))
class CKENTITYSPAWNER_API ACk_EntitySpawner_UE : public AInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_EntitySpawner_UE);

public:
    ACk_EntitySpawner_UE();

protected:
    auto
    BeginPlay() -> void override;

    auto
    EndPlay(
        const EEndPlayReason::Type EndPlayReason) -> void override;

#if WITH_EDITOR
    auto
    PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent) -> void override;

    auto
    PostEditUndo() -> void override;

    auto
    Destroyed() -> void override;
#endif

public:
#if WITH_EDITOR
    auto
    EditorOnly_InitializeEntityScript(
        TSubclassOf<UCk_EntityScript_UE> InEntityScriptClass) -> void;

    // Spawns (or respawns) this spawner's editor-only ECS entity in the owning UWorld's
    // UCk_EditorEcsWorld_Subsystem_UE. Idempotent — if a cached handle is still valid, the old
    // entity is destroyed first. Safe to call before, during, or after the factory's PostSpawnActor.
    auto
    EditorOnly_RebuildEntity() -> void;

    // Destroys this spawner's editor-only ECS entity, if one exists. Idempotent.
    auto
    EditorOnly_DestroyEntity() -> void;
#endif

private:
    auto
    DoSpawnEntity() -> void;

private:
    UPROPERTY(EditAnywhere, Instanced,
        Category = "Ck|EntitySpawner",
        meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_EntityScript_UE> _EntityScript;

    UPROPERTY(EditAnywhere,
        Category = "Ck|EntitySpawner",
        meta = (AllowPrivateAccess = true, Categories = "ActorRelay"))
    FGameplayTag _ReplicatedChannelGroup;

#if WITH_EDITORONLY_DATA
    // Transient handle to the editor-only ECS entity backing this spawner. NOT persisted with the
    // level — the entity is rebuilt from _EntityScript on map open, factory placement, and after
    // any property change. Declared WITH_EDITORONLY_DATA so it is stripped from cooked builds.
    FCk_Handle _EditorEntityHandle;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
