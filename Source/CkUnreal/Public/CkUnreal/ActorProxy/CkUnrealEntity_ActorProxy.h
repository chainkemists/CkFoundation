#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkUnreal/Entity/CkUnrealEntity_Fragment_Params.h"

#include "CkUnrealEntity_ActorProxy.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKUNREAL_API UCk_ChildActorComponent : public UChildActorComponent
{
    GENERATED_BODY()
};


UCLASS(Abstract, Blueprintable, BlueprintType)
class CKUNREAL_API ACk_UnrealEntity_ActorProxy_UE : public AActor
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_UnrealEntity_ActorProxy_UE);

public:
    ACk_UnrealEntity_ActorProxy_UE();

protected:
    UFUNCTION(BlueprintImplementableEvent)
    FCk_Handle
    Get_TransientHandle() const;

    UFUNCTION(BlueprintImplementableEvent)
    void
    OnEntityCreated(const FCk_Handle& InCreatedEntity) const;

public:
    void OnConstruction(
        const FTransform& Transform) override;

protected:
#if WITH_EDITOR
    auto PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent) -> void override;
    void PostLoad() override;

public:
    void PreSave(
        FObjectPreSaveContext ObjectSaveContext) override;
	void PostSaveRoot(FObjectPostSaveRootContext ObjectSaveContext) override;

protected:
#endif

public:
    auto BeginPlay() -> void override;

public:
    auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const -> void override;

public:
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TSubclassOf<AActor> _ActorToSpawn;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Replicated)
    TObjectPtr<AActor> _SpawnedActor;

#if WITH_EDITORONLY_DATA
    UPROPERTY(Transient)
    TObjectPtr<UChildActorComponent> _ChildActorComponent;

    UPROPERTY(Transient)
    TObjectPtr<AActor> _TransientActor;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
