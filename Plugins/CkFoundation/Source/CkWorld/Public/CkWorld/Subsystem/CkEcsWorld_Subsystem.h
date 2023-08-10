#pragma once

#include "CkMacros/CkMacros.h"

#include "CkWorld/Public/CkWorld/Ecs/CkEcsWorld.h"

#include "CkEcsWorld_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKWORLD_API UCk_EcsWorld_Subsystem_UE : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EcsWorld_Subsystem_UE);

public:
    using FEcsWorldType = ck::FEcsWorld;

public:
    virtual auto Deinitialize() -> void override;
    virtual auto Initialize(FSubsystemCollectionBase& Collection) -> void override;

public:
    UFUNCTION(BlueprintImplementableEvent)
    void OnInitialize();

    UFUNCTION(BlueprintImplementableEvent)
    void OnDeInitialize();

private:
    auto DoSpawnWorldActor() -> void;

private:
    UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = true))
    TEnumAsByte<ETickingGroup> _TickingGroup = TG_PrePhysics;

    UPROPERTY(Transient)
    TObjectPtr<class ACk_World_Actor_UE> _WorldActor = nullptr;

private:
    static FCk_Handle _TransientEntity;

public:
    CK_PROPERTY_GET_STATIC(_TransientEntity);
};