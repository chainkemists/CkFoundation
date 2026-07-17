#pragma once

#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"

#include "CkIskmSubsystem.generated.h"

class UCk_IskmAnimCollection_Data;
class UCk_IskmRenderer_Data;
class ACk_Iskm_BatchedCrowd_Actor;

// ---- Manager Actor ----

UCLASS(Blueprintable, BlueprintType)
class CKISKMRENDERER_API ACk_IskmRenderer_Actor_UE final : public AActor
{
    GENERATED_BODY()

public:
    friend class UCk_IskmRenderer_Subsystem_UE;

public:
    CK_GENERATED_BODY(ACk_IskmRenderer_Actor_UE);

public:
    ACk_IskmRenderer_Actor_UE();

    auto
    DoInitialize(UCk_IskmRenderer_Data* InRendererData) -> void;

    // Allocate a SKMC for a new proxy entity. Returns either a freshly created or pooled-reusable SKMC.
    auto
    Acquire_BaseSKMC() -> USkeletalMeshComponent*;

#if WITH_EDITOR
    // Editor worlds freeze SKMC poses behind the editor-only bUpdateAnimationInEditor flag —
    // enable it so previews animate. No-op outside Editor/EditorPreview worlds. Applied to every
    // acquired base SKMC and every submesh child created in editor worlds.
    static auto
    EditorOnly_EnableAnimationTicking(USkeletalMeshComponent* InComp) -> void;
#endif

    // Release a SKMC back to the pool. SKMC is hidden, detached, anim cleared.
    auto
    Release_BaseSKMC(USkeletalMeshComponent* InComp) -> void;

protected:
    auto BeginPlay() -> void override;

#if WITH_EDITOR
public:
    // Editor-world per-owner renderers redirect viewport clicks on their SKMCs to the placed
    // actor whose preview they render (see ck::FFragment_EditorSelectionOwner) — clicking the
    // preview mesh then selects/moves/deletes that actor like clicking its billboard.
    auto
    IsSelectionChild() const -> bool override;

    auto
    GetSelectionParent() const -> AActor* override;
#endif

#if WITH_EDITORONLY_DATA
private:
    // Non-UPROPERTY on purpose: this actor is transient and must never round-trip through the
    // transaction buffer; the owner is a level actor whose lifetime the weak ptr observes.
    TWeakObjectPtr<AActor> _EditorSelectionOwner;
#endif

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
    TObjectPtr<USceneComponent> _RootNode;

    UPROPERTY(Transient)
    TObjectPtr<UCk_IskmRenderer_Data> _RendererData;

    UPROPERTY(Transient)
    TArray<TObjectPtr<USkeletalMeshComponent>> _Pool_FreeSKMCs;

    UPROPERTY(Transient)
    TArray<TObjectPtr<USkeletalMeshComponent>> _LiveSKMCs;

    bool _Initialized = false;

public:
    CK_PROPERTY_GET(_RendererData);
    CK_PROPERTY_GET(_LiveSKMCs);
};

// ---- Subsystem ----

UCLASS(DisplayName = "CkSubsystem_IskmRenderer")
class CKISKMRENDERER_API UCk_IskmRenderer_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_IskmRenderer_Subsystem_UE);

public:
#if WITH_EDITOR
    auto Initialize(FSubsystemCollectionBase& Collection) -> void override;
#endif
    auto Deinitialize() -> void override;

protected:
    auto DoesSupportWorldType(const EWorldType::Type WorldType) const -> bool override;

#if WITH_EDITOR
private:
    // Backstop reclaim for per-owner renderers whose SKMC pool is already quiescent when their
    // selection owner is deleted (no future Release_BaseSKMC → the actor-side self-reclaim never
    // runs). The in-flight case is left to that self-reclaim — see Release_BaseSKMC.
    auto
    EditorOnly_OnLevelActorDeleted(AActor* InActor) -> void;
#endif

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|IskmRenderer")
    ACk_IskmRenderer_Actor_UE*
    GetOrCreate_RendererActor(UCk_IskmRenderer_Data* InRendererData);

#if WITH_EDITOR
    // Editor-world variant: one renderer per (data asset, selection owner), so a viewport click
    // on any of its SKMCs can redirect selection to the owner. Selection redirect is actor-level,
    // so the split IS the instance-to-spawner mapping; editor previews are a handful of meshes,
    // and runtime worlds always use the shared renderer above. SKMC release stays owner-derived
    // (SKMC->GetOwner()), so proxies need no extra teardown bookkeeping.
    auto
    GetOrCreate_RendererActor_ForEditorSelectionOwner(
        UCk_IskmRenderer_Data* InRendererData,
        AActor* InSelectionOwner) -> ACk_IskmRenderer_Actor_UE*;

    // Batched-crowd sibling of the per-owner renderer above: one preview crowd per
    // (anim collection, selection owner), so a viewport click on any of its tile instances
    // redirects selection to the owner — the split IS the member-to-spawner mapping (a shared
    // crowd cannot attribute instances to owners at actor level). The crowd is Initialize()d
    // and ready for AddInstance/Finalize; reclaimed when the owner is deleted.
    auto
    GetOrCreate_PreviewCrowd_ForEditorSelectionOwner(
        UCk_IskmAnimCollection_Data* InCollection,
        float InTileSize,
        AActor* InSelectionOwner) -> ACk_Iskm_BatchedCrowd_Actor*;
#endif

private:
    auto
    DoSpawn_RendererActor(
        UCk_IskmRenderer_Data* InRendererData) -> ACk_IskmRenderer_Actor_UE*;

private:
    UPROPERTY()
    TMap<TObjectPtr<UCk_IskmRenderer_Data>, TObjectPtr<ACk_IskmRenderer_Actor_UE>> _RendererActors;

#if WITH_EDITORONLY_DATA
private:
    // Weak on both sides: the actors are owned by the world; entries self-heal via validity
    // checks. TWeakObjectPtr keys compare by object index + serial, so entries stay addressable
    // even while an owner is mid-undo or already destroyed.
    using FPerOwnerRendererKey = TPair<TWeakObjectPtr<const UCk_IskmRenderer_Data>, TWeakObjectPtr<AActor>>; // {DataAsset, SelectionOwner}
    TMap<FPerOwnerRendererKey, TWeakObjectPtr<ACk_IskmRenderer_Actor_UE>> _PerOwnerRendererActors;

    using FPerOwnerCrowdKey = TPair<TWeakObjectPtr<const UCk_IskmAnimCollection_Data>, TWeakObjectPtr<AActor>>; // {AnimCollection, SelectionOwner}
    TMap<FPerOwnerCrowdKey, TWeakObjectPtr<ACk_Iskm_BatchedCrowd_Actor>> _PerOwnerPreviewCrowds;

    FDelegateHandle _OnLevelActorDeletedHandle;
#endif
};

// ---- Subsystem accessor utility (matching ISM) ----

UCLASS(NotBlueprintable)
class CKISKMRENDERER_API UCk_Utils_IskmRenderer_Subsystem_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmRenderer|Subsystem",
        DisplayName="[Ck][IskmRenderer] Get Or Create Renderer Actor")
    static ACk_IskmRenderer_Actor_UE*
    GetOrCreate_RendererActor(
        const UWorld* InWorld,
        UCk_IskmRenderer_Data* InRendererData);
};
