#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEntityBridge/CkEntityBridge_Fragment_Data.h"

#include "CkActorProxy_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKACTORPROXY_API ACk_ActorProxy_UE : public AActor
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_ActorProxy_UE);

public:
    ACk_ActorProxy_UE();

protected:
    UFUNCTION(BlueprintImplementableEvent)
    FCk_Handle
    Get_TransientHandle() const;

    UFUNCTION(BlueprintImplementableEvent)
    void
    OnEntityCreated(const FCk_Handle& InCreatedEntity) const;

protected:
#if WITH_EDITOR
    virtual auto PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent) -> void override;
#endif

public:
    virtual auto BeginPlay() -> void override;

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
#endif
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKACTORPROXY_API UCk_ActorProxy_Subsystem_UE : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ActorProxy_Subsystem_UE);

    void PostInitialize() override;
    void Initialize(
        FSubsystemCollectionBase& Collection) override;

    /** Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors */
    auto OnWorldBeginPlay(UWorld& InWorld) -> void override;

private:
    auto DoDestroyAllActorProxyChildActors() const -> void;
};

// --------------------------------------------------------------------------------------------------------------------
