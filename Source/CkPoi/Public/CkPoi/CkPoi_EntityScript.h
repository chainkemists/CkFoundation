#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkEcs/EntityScript/CkGenericEntityScript.h"

#include "CkPoi/CkPoi_Fragment_Data.h"

#include "CkPoi_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Spawn params for UCk_Poi_EntityScript when spawned at runtime via UCk_Utils_EntityScript_UE::Request_SpawnEntity.
// When spawned from a level spawner (ACk_EntitySpawner_UE), the CDO-set _PoiParams + the spawner-injected
// _SpawnTransform are used instead and this struct is absent.
USTRUCT(BlueprintType)
struct CKPOI_API FCk_Poi_SpawnParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Poi_SpawnParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Fragment_Poi_ParamsData _PoiParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FTransform _Transform;

public:
    CK_PROPERTY_GET(_PoiParams);
    CK_PROPERTY_GET(_Transform);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Poi_SpawnParams, _PoiParams, _Transform);
};

// --------------------------------------------------------------------------------------------------------------------

// The rebuild recipe for durable standalone POIs. Under the v3 rebuild+hydrate save model, entities only
// round-trip a save when they have a spawn recipe — this script IS that recipe: level spawners
// (ACk_EntitySpawner_UE) reference it for designer-placed POIs, and runtime code spawns it via
// UCk_Utils_EntityScript_UE::Request_SpawnEntity with FCk_Poi_SpawnParams for runtime-placed durable POIs
// (player waypoints). Its replayed Construct re-composes Transform + Poi on load; the CkPoi persistence
// handler then hydrates enable-state and state tags.
UCLASS(Blueprintable, BlueprintType)
class CKPOI_API UCk_Poi_EntityScript : public UCk_GenericEntityScript_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Poi_EntityScript);

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poi",
              meta = (AllowPrivateAccess = true))
    FCk_Fragment_Poi_ParamsData _PoiParams;

    // Injected by ACk_EntitySpawner_UE with the spawner actor's world transform (level-placed path); overridden by
    // FCk_Poi_SpawnParams when spawned at runtime.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poi",
              meta = (AllowPrivateAccess = true))
    FTransform _SpawnTransform;

public:
    CK_PROPERTY_GET(_PoiParams);
    CK_PROPERTY_GET(_SpawnTransform);

protected:
    auto
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams) -> ECk_EntityScript_ConstructionFlow override;
};

// --------------------------------------------------------------------------------------------------------------------
