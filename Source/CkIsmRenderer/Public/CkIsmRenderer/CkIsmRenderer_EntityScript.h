#pragma once

#include "CkEcsExt/EntityScript/CkEntityScript_WithActor.h"

#include "CkIsmRenderer_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_IsmRenderer_Data;

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKISMRENDERER_API UCk_EntityScript_IsmRenderer_UE : public UCk_EntityScript_WithActor_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityScript_IsmRenderer_UE);

    UCk_EntityScript_IsmRenderer_UE();

protected:
    [[nodiscard]]
    auto
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams) -> ECk_EntityScript_ConstructionFlow override;
};

// --------------------------------------------------------------------------------------------------------------------
