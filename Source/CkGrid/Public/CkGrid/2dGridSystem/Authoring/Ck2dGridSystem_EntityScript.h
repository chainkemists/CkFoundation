#pragma once

#include "CkEcs/EntityScript/CkGenericEntityScript.h"

#include "Ck2dGridSystem_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_2dGridSystem_Spec;

// --------------------------------------------------------------------------------------------------------------------

// Runtime authoring glue: spawns a single 2d grid from a UCk_2dGridSystem_Spec DataAsset, then
// applies the Spec's per-cell tags and blockers on top. Driven by the CkEntitySpawner actor — the
// spawner injects its actor transform into the `SpawnTransform` UPROPERTY below (resolved by name
// via ck::entityspawner::TryResolveDefaultTransformProperty), and the grid is added to the bridged
// entity's Transform so it inherits that placement.
UCLASS(Blueprintable, BlueprintType)
class CKGRID_API UCk_2dGridSystem_EntityScript : public UCk_GenericEntityScript_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_2dGridSystem_EntityScript);

protected:
    auto
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams) -> ECk_EntityScript_ConstructionFlow override;

private:
    // The grid definition to instantiate. Edited by the paint tool.
    UPROPERTY(EditAnywhere, Category = "Ck|2dGridSystem")
    TObjectPtr<UCk_2dGridSystem_Spec> Spec;

    // Injected by ACk_EntitySpawner_UE via FCk_EntitySpawner_ScriptPropertyBinding. The name
    // "SpawnTransform" is the spawner's default-resolution target (see
    // ck::entityspawner::TryResolveDefaultTransformProperty). Currently informational — the grid
    // is added to the bridged entity Transform set up by Super::Construct.
    UPROPERTY(EditAnywhere, Category = "Ck|2dGridSystem")
    FTransform SpawnTransform = FTransform::Identity;
};

// --------------------------------------------------------------------------------------------------------------------
