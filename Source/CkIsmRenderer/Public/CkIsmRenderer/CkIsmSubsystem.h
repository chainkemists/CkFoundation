#pragma once

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkIsmRenderer/Renderer/CkIsmRenderer_Fragment_Data.h"

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

    // Idempotent — also called post-spawn so editor worlds (no BeginPlay) get the EntityScript.
    auto
    DoInitialize() -> void;

protected:
    auto
    BeginPlay() -> void override;

#if WITH_EDITOR
public:
    // Redirects viewport clicks on this renderer's instances to the placed actor whose preview it
    // renders (see ck::FFragment_EditorSelectionOwner).
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
    // Destroys renderer actors baked into the level package by a pre-RF_Transient version, and
    // dirties editor levels so a clean copy can be saved. See CkIsmRenderer/CLAUDE.md.
    auto DoSweepLeakedRenderers() -> void;

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|IsmRenderer")
    ACk_IsmRenderer_Actor_UE*
    GetOrCreate_IsmRenderer(
        const UCk_IsmRenderer_Data* InDataAsset);

#if WITH_EDITOR
    // Editor-world variant: one renderer per (data asset, selection owner) — the split IS the
    // per-instance selection mapping. See CkIsmRenderer/CLAUDE.md.
    auto
    GetOrCreate_IsmRenderer_ForEditorSelectionOwner(
        const UCk_IsmRenderer_Data* InDataAsset,
        AActor* InSelectionOwner) -> ACk_IsmRenderer_Actor_UE*;
#endif

    // InEditorSelectionOwner (editor previews only, explicitly-null otherwise) selects the per-owner
    // renderer's component. Weak keys compare by index + serial, so the lookup keeps resolving the
    // SAME component after the owner is destroyed — teardown never falls back to the shared one.
    auto
    FindOrCache_IsmComponent(
        const UCk_IsmRenderer_Data* InRendererData,
        const TWeakObjectPtr<AActor>& InEditorSelectionOwner = {}) -> TWeakObjectPtr<UInstancedStaticMeshComponent>;

    // The "shadow ISM" for (renderer data, outline preset): a custom-depth-only twin of the renderer's
    // ISM carrying the preset's stencil value. Custom depth is per-component, so per-instance outlines
    // need this second component. See CkIsmRenderer/CLAUDE.md.
    auto
    FindOrCreate_OutlineIsmComponent(
        const UCk_IsmRenderer_Data* InRendererData,
        const UCkUsf_OutlinePreset* InPreset,
        uint8 InStencilValue,
        const TWeakObjectPtr<AActor>& InEditorSelectionOwner = {}) -> TWeakObjectPtr<UInstancedStaticMeshComponent>;

    // The cel-pattern twin of the above, keyed on the stencil VALUE: the cel contract is a direct value
    // rather than a refcounted preset allocation, so two patterns sharing a renderer need two shadows and
    // nothing needs releasing. See CkIsmRenderer/CLAUDE.md.
    auto
    FindOrCreate_CelPatternIsmComponent(
        const UCk_IsmRenderer_Data* InRendererData,
        uint8 InStencilValue,
        const TWeakObjectPtr<AActor>& InEditorSelectionOwner = {}) -> TWeakObjectPtr<UInstancedStaticMeshComponent>;

    // The stylize-effect-mask twin, keyed the same way: the mask is a direct stencil value too, and there is
    // exactly one of them per project, so a crowd of masked proxies on one renderer shares one shadow and
    // nothing needs releasing. See CkIsmRenderer/CLAUDE.md.
    auto
    FindOrCreate_StylizeMaskIsmComponent(
        const UCk_IsmRenderer_Data* InRendererData,
        uint8 InStencilValue,
        const TWeakObjectPtr<AActor>& InEditorSelectionOwner = {}) -> TWeakObjectPtr<UInstancedStaticMeshComponent>;

private:
    // Shared body of every shadow factory: a custom-depth-only twin of InSourceIsm carrying InStencilValue.
    static auto
    DoCreate_CustomDepthShadowIsm(
        UInstancedStaticMeshComponent* InSourceIsm,
        uint8 InStencilValue,
        FName InNameBase) -> UInstancedStaticMeshComponent*;

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

    using FOutlineIsmKey = TTuple<TWeakObjectPtr<const UCk_IsmRenderer_Data>, TWeakObjectPtr<const UCkUsf_OutlinePreset>, TWeakObjectPtr<AActor>>;
    TMap<FOutlineIsmKey, TWeakObjectPtr<UInstancedStaticMeshComponent>> _OutlineIsmComponentCache;

    using FCelPatternIsmKey = TTuple<TWeakObjectPtr<const UCk_IsmRenderer_Data>, uint8, TWeakObjectPtr<AActor>>;
    TMap<FCelPatternIsmKey, TWeakObjectPtr<UInstancedStaticMeshComponent>> _CelPatternIsmComponentCache;

    using FStylizeMaskIsmKey = TTuple<TWeakObjectPtr<const UCk_IsmRenderer_Data>, uint8, TWeakObjectPtr<AActor>>;
    TMap<FStylizeMaskIsmKey, TWeakObjectPtr<UInstancedStaticMeshComponent>> _StylizeMaskIsmComponentCache;

#if WITH_EDITORONLY_DATA
private:
    // Weak on both sides: the world owns the actors/components and entries self-heal on lookup;
    // weak keys compare by index + serial, so lookups survive the owner actor's destruction.
    using FPerOwnerRendererKey = TPair<TWeakObjectPtr<const UCk_IsmRenderer_Data>, TWeakObjectPtr<AActor>>; // {DataAsset, SelectionOwner}
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
