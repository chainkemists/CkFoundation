#pragma once

#include "CkCore/Macros/CkMacros.h"

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

public:
#if WITH_EDITOR
    auto
    EditorOnly_InitializeEntityScript(
        TSubclassOf<UCk_EntityScript_UE> InEntityScriptClass) -> void;
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
};

// --------------------------------------------------------------------------------------------------------------------
