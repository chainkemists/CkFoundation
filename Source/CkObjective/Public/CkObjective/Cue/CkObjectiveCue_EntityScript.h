#pragma once

#include "CkCue/CkCueBase_EntityScript.h"

#include <GameplayTagContainer.h>

#include "CkObjectiveCue_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKOBJECTIVE_API UCk_ObjectiveCue_EntityScript : public UCk_CueBase_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ObjectiveCue_EntityScript);

protected:
    auto Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams) -> ECk_EntityScript_ConstructionFlow override;
};

// --------------------------------------------------------------------------------------------------------------------