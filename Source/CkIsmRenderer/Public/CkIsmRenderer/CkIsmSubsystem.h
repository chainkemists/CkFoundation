#pragma once

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkIsmRenderer/Renderer/CkIsmRenderer_Fragment_Data.h"

#include "CkIsmSubsystem.generated.h"

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

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta=(AllowPrivateAccess))
    TObjectPtr<USceneComponent> _RootNode;

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

    // Finds or caches the ISM component for a given renderer data asset.
    // Used by the ISM proxy processors to resolve data assets to their ISM components.
    auto
    FindOrCache_IsmComponent(
        const UCk_IsmRenderer_Data* InRendererData) -> TWeakObjectPtr<UInstancedStaticMeshComponent>;

private:
    UPROPERTY()
    TMap<const UCk_IsmRenderer_Data*, TObjectPtr<ACk_IsmRenderer_Actor_UE>> _IsmRenderers;

    UPROPERTY()
    TMap<const UCk_IsmRenderer_Data*, TWeakObjectPtr<UInstancedStaticMeshComponent>> _IsmComponentCache;
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
