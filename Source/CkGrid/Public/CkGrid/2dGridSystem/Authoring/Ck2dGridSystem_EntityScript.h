#pragma once

#include "CkEcs/EntityScript/CkGenericEntityScript.h"

#include "Ck2dGridSystem_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_2dGridSystem_Spec;

// --------------------------------------------------------------------------------------------------------------------

// Runtime authoring glue: spawns a single 2d grid from a UCk_2dGridSystem_Spec DataAsset, then
// applies the Spec's per-cell tags and blockers on top. Driven by the CkEntitySpawner actor.
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

    // Injected by ACk_EntitySpawner_UE; the name is the spawner's default-resolution target and
    // must not be changed.
    UPROPERTY(EditAnywhere, Category = "Ck|2dGridSystem")
    FTransform SpawnTransform = FTransform::Identity;
};

// --------------------------------------------------------------------------------------------------------------------
