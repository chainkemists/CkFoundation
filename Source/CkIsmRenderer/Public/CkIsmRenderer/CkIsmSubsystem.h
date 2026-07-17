#pragma once

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkIsmRenderer/Renderer/CkIsmRenderer_Fragment_Data.h"

#include <UObject/ObjectKey.h>

#include "CkIsmSubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCkUsf_OutlinePreset;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType)
class CKISMRENDERER_API ACk_IsmRenderer_Actor_UE final : public AActor
{
    GENERATED_BODY()

public:
    friend class UCk_IsmRenderer_Subsystem_UE;

public:
    CK_GENERATED_BODY(ACk_IsmRenderer_Actor_UE);

public:
    ACk_IsmRenderer_Actor_UE();

    // Idempotent. Called by UCk_IsmRenderer_Subsystem_UE::GetOrCreate_IsmRenderer post-spawn so
    // the EntityScript is created in both runtime (BeginPlay) and editor (no BeginPlay) worlds.
    auto
    DoInitialize() -> void;

protected:
    auto
    BeginPlay() -> void override;

#if WITH_EDITOR
public:
    // Editor-world per-owner renderers redirect viewport clicks on their instances to the placed
    // actor whose preview they render (see ck::FFragment_EditorSelectionOwner) — clicking the
    // preview mesh then selects/moves/deletes that actor like clicking its billboard.
    auto
    IsSelectionChild() const -> bool override;

    auto
    GetSelectionParent() const -> AActor* override;
#endif

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta=(AllowPrivateAccess))
    TObjectPtr<USceneComponent> _RootNode;

#if WITH_EDITORONLY_DATA
private:
    // Non-UPROPERTY on purpose: this actor is transient and must never round-trip through the
    // transaction buffer; the owner is a level actor whose lifetime the weak ptr observes.
    TWeakObjectPtr<AActor> _EditorSelectionOwner;
#endif

private:

    UPROPERTY()
    TObjectPtr<const UCk_IsmRenderer_Data> _RenderData;

    bool _Initialized = false;

public:
    CK_PROPERTY_GET(_RenderData);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_IsmRenderer")
class CKISMRENDERER_API UCk_IsmRenderer_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_IsmRenderer_Subsystem_UE);

public:
    auto Initialize(FSubsystemCollectionBase& Collection) -> void override;
    auto Deinitialize() -> void override;

protected:
    auto DoesSupportWorldType(const EWorldType::Type WorldType) const -> bool override;

private:
    // Destroys any ACk_IsmRenderer_Actor_UE instances that were previously baked into the level
    // package by a prior version that didn't mark them transient. Runs once on subsystem init in
    // both editor and runtime worlds. In the editor, marks the level dirty so the user can save
    // out a clean copy. The cache is empty at this point, so subsequent GetOrCreate calls from
    // the entity-spawner rebuild path will spawn fresh (now-transient) replacements.
    auto DoSweepLeakedRenderers() -> void;

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|IsmRenderer")
    ACk_IsmRenderer_Actor_UE*
    GetOrCreate_IsmRenderer(
        const UCk_IsmRenderer_Data* InDataAsset);

#if WITH_EDITOR
    // Editor-world variant: one renderer per (data asset, selection owner), so a viewport click
    // on any of its instances can redirect selection to the owner. A shared renderer would make
    // every preview mesh select the same meaningless transient actor — per-instance mapping is
    // impossible at the actor level, so the split IS the mapping. Editor previews are small, so
    // the lost batching is irrelevant; runtime worlds always use the shared renderer above.
    auto
    GetOrCreate_IsmRenderer_ForEditorSelectionOwner(
        const UCk_IsmRenderer_Data* InDataAsset,
        AActor* InSelectionOwner) -> ACk_IsmRenderer_Actor_UE*;
#endif

    // Finds or caches the ISM component for a given renderer data asset.
    // Used by the ISM proxy processors to resolve data assets to their ISM components.
    // InEditorSelectionOwnerKey (editor worlds only, invalid otherwise) selects the per-owner
    // renderer's component; a stored FObjectKey keeps resolving the SAME component even after
    // the owner actor is destroyed, so teardown removes instances from the component that
    // actually holds them (never silently falls back to the shared component).
    auto
    FindOrCache_IsmComponent(
        const UCk_IsmRenderer_Data* InRendererData,
        const FObjectKey& InEditorSelectionOwnerKey = FObjectKey{}) -> TWeakObjectPtr<UInstancedStaticMeshComponent>;

    // Finds or creates the "shadow ISM" for (renderer data, outline preset): a custom-depth-only twin of
    // the renderer's ISM (same mesh/mobility/cull distances, no main pass, no shadows, no collision) whose
    // Custom-Stencil value is the preset's allocated value. The outline processors mirror outlined proxies'
    // instances into it — custom depth is per-component, so per-instance outlines need a second component.
    auto
    FindOrCreate_OutlineIsmComponent(
        const UCk_IsmRenderer_Data* InRendererData,
        const UCkUsf_OutlinePreset* InPreset,
        uint8 InStencilValue,
        const FObjectKey& InEditorSelectionOwnerKey = FObjectKey{}) -> TWeakObjectPtr<UInstancedStaticMeshComponent>;

private:
    auto
    DoSpawn_IsmRendererActor(
        const UCk_IsmRenderer_Data* InDataAsset,
        const FString& InNameSuffix) -> ACk_IsmRenderer_Actor_UE*;

    static auto
    DoFind_IsmComponentOnRenderer(
        const ACk_IsmRenderer_Actor_UE* InRenderer,
        const UCk_IsmRenderer_Data* InRendererData) -> UInstancedStaticMeshComponent*;

private:
    UPROPERTY()
    TMap<const UCk_IsmRenderer_Data*, TObjectPtr<ACk_IsmRenderer_Actor_UE>> _IsmRenderers;

    UPROPERTY()
    TMap<const UCk_IsmRenderer_Data*, TWeakObjectPtr<UInstancedStaticMeshComponent>> _IsmComponentCache;

    // {DataAsset, Preset, EditorSelectionOwner (invalid outside editor previews)}
    using FOutlineIsmKey = TTuple<TWeakObjectPtr<const UCk_IsmRenderer_Data>, TWeakObjectPtr<const UCkUsf_OutlinePreset>, FObjectKey>;
    TMap<FOutlineIsmKey, TWeakObjectPtr<UInstancedStaticMeshComponent>> _OutlineIsmComponentCache;

#if WITH_EDITORONLY_DATA
private:
    // Weak values: the actors/components are owned by the world; entries self-heal via validity
    // checks. Keyed by FObjectKey so lookups keep working after the owner actor is destroyed.
    using FPerOwnerRendererKey = TPair<FObjectKey, FObjectKey>; // {DataAsset, SelectionOwner}
    TMap<FPerOwnerRendererKey, TWeakObjectPtr<ACk_IsmRenderer_Actor_UE>> _PerOwnerIsmRenderers;
    TMap<FPerOwnerRendererKey, TWeakObjectPtr<UInstancedStaticMeshComponent>> _PerOwnerIsmComponentCache;
#endif
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKISMRENDERER_API UCk_Utils_IsmRenderer_Subsystem_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    using SubsystemType = UCk_Utils_IsmRenderer_Subsystem_UE;

public:
    static auto
    GetOrCreate_IsmRenderer(
        const UWorld* InWorld,
        const UCk_IsmRenderer_Data* InDataAsset) -> ACk_IsmRenderer_Actor_UE*;
};

// --------------------------------------------------------------------------------------------------------------------
