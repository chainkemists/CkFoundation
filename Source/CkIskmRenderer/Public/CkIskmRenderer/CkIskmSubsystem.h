#pragma once

#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"

#include "CkIskmSubsystem.generated.h"

class UCk_IskmAnimCollection_Data;
class UCk_IskmRenderer_Data;

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

    // Release a SKMC back to the pool. SKMC is hidden, detached, anim cleared.
    auto
    Release_BaseSKMC(USkeletalMeshComponent* InComp) -> void;

protected:
    auto BeginPlay() -> void override;

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
    auto Deinitialize() -> void override;

protected:
    auto DoesSupportWorldType(const EWorldType::Type WorldType) const -> bool override;

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|IskmRenderer")
    ACk_IskmRenderer_Actor_UE*
    GetOrCreate_RendererActor(UCk_IskmRenderer_Data* InRendererData);

private:
    UPROPERTY()
    TMap<TObjectPtr<UCk_IskmRenderer_Data>, TObjectPtr<ACk_IskmRenderer_Actor_UE>> _RendererActors;
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
