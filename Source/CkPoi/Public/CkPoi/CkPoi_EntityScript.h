#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkEcs/EntityScript/CkGenericEntityScript.h"

#include "CkPoi/CkPoi_Fragment_Data.h"

#include "CkPoi_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Runtime-spawn params only. Level spawners (ACk_EntitySpawner_UE) instead use the CDO-set _PoiParams
// plus the spawner-injected _SpawnTransform, and pass no spawn params at all.
USTRUCT(BlueprintType)
struct CKPOI_API FCk_Poi_SpawnParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Poi_SpawnParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Poi_Spec _PoiParams;

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

// The save-rebuild recipe for durable standalone POIs: under the v3 rebuild+hydrate model an entity only
// round-trips a save when it has a spawn recipe. See CkPoi/CLAUDE.md § Recipes.
UCLASS(Blueprintable, BlueprintType)
class CKPOI_API UCk_Poi_EntityScript : public UCk_GenericEntityScript_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Poi_EntityScript);

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poi",
              meta = (AllowPrivateAccess = true))
    FCk_Poi_Spec _PoiParams;

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
