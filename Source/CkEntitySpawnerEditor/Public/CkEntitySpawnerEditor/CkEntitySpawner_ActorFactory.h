#pragma once

#include "ActorFactories/ActorFactory.h"

#include "CkEntitySpawner_ActorFactory.generated.h"

class UCk_EntityScript_UE;

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKENTITYSPAWNEREDITOR_API UCk_EntitySpawner_ActorFactory_UE : public UActorFactory
{
    GENERATED_UCLASS_BODY()

public:
    virtual auto
    CanCreateActorFrom(
        const FAssetData& InAssetData,
        FText& OutErrorMsg) -> bool override;

    virtual auto
    PostSpawnActor(
        UObject* InAsset,
        AActor* InNewActor) -> void override;

private:
    static auto
    ResolveEntityScriptClass(
        const FAssetData& InAssetData) -> TSubclassOf<UCk_EntityScript_UE>;
};

// --------------------------------------------------------------------------------------------------------------------
