#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include <GameFramework/Info.h>
#include <Misc/Optional.h>

#include "CkEntitySpawner_Actor.generated.h"

class UCk_EntityScript_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::entityspawner
{
    // Fallback for an empty details-panel picker: the first FTransform property named
    // SpawnTransform / _SpawnTransform / InitialTransform / _InitialTransform, else nullptr.
    CKENTITYSPAWNER_API auto
    TryResolveDefaultTransformProperty(
        const UClass* InEntityScriptClass) -> FProperty*;
}

// --------------------------------------------------------------------------------------------------------------------

// Names one FTransform property on the EntityScript to be driven by the spawner's actor transform.
// Rendered in the details panel as a dropdown populated via reflection over the assigned script.
USTRUCT(BlueprintType)
struct CKENTITYSPAWNER_API FCk_EntitySpawner_ScriptPropertyBinding
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EntitySpawner_ScriptPropertyBinding);

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Ck|EntitySpawner",
        meta = (AllowPrivateAccess = true))
    FName _PropertyName;

public:
    CK_PROPERTY(_PropertyName);
};

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
    PostEditMove(
        bool InIsFinished) -> void override;

    auto
    PreEditUndo() -> void override;

    auto
    PostEditUndo() -> void override;

    auto
    Destroyed() -> void override;

    // The proxy actors hosting the preview visuals are not attached, so the engine's attached-actor
    // selection propagation cannot reach them.
    auto
    PushSelectionToProxies() -> void override;
#endif

public:
#if WITH_EDITOR
    auto
    EditorOnly_InitializeEntityScript(
        TSubclassOf<UCk_EntityScript_UE> InEntityScriptClass) -> void;

    auto
    EditorOnly_RebuildEntity() -> void;

    auto
    EditorOnly_DestroyEntity() -> void;

private:
    auto
    EditorOnly_DoRebuildEntity() -> void;

    auto
    EditorOnly_PushActorTransformToEntity() -> void;
public:
#endif

private:
    auto
    DoSpawnEntity() -> void;

    auto
    DoDestroyRuntimeEntity() -> void;

    auto
    DoInjectActorTransform() -> void;

private:
    UPROPERTY(EditAnywhere, Instanced,
        Category = "Ck|EntitySpawner",
        meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_EntityScript_UE> _EntityScript;

    UPROPERTY(EditAnywhere,
        Category = "Ck|EntitySpawner",
        meta = (AllowPrivateAccess = true, Categories = "ActorRelay"))
    FGameplayTag _ReplicatedChannelGroup;

    UPROPERTY(EditAnywhere,
        Category = "Ck|EntitySpawner",
        DisplayName = "Inject Actor Transform Into",
        meta = (AllowPrivateAccess = true))
    FCk_EntitySpawner_ScriptPropertyBinding _InjectActorTransformToScriptProperty;

public:
    CK_PROPERTY_GET(_InjectActorTransformToScriptProperty);
    CK_PROPERTY_GET(_EntityScript);

private:
    FCk_Handle _RuntimeEntityHandle;

#if WITH_EDITORONLY_DATA
private:
    // Non-UPROPERTY: must not round-trip through the transaction buffer — a restored handle
    // would point at an already-dead registry entry. PostEditUndo rebuilds fresh.
    FCk_Handle _EditorEntityHandle;

    // PostEditMove observes the actor after it moved, so the construction frame is required to
    // recover and preserve any transform delta authored by the EntityScript.
    FTransform _EditorActorTransformAtPreviewConstruction = FTransform::Identity;
    TOptional<FTransform> _EditorEntityTransformRelativeToActor;

    bool _EditorRebuildPending = false;
    FDelegateHandle _EditorRebuildEndFrameHandle;

public:
    CK_PROPERTY_GET(_EditorEntityHandle);
#endif
};

// --------------------------------------------------------------------------------------------------------------------
