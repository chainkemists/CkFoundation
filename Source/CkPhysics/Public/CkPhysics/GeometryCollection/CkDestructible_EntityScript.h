#pragma once

#include "CkEcsExt/EntityScript/CkEntityScript_WithActor.h"

#include "CkDestructible_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKPHYSICS_API UCk_EntityScript_Destructible_UE : public UCk_EntityScript_WithActor_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityScript_Destructible_UE);

    UCk_EntityScript_Destructible_UE();

protected:
    [[nodiscard]]
    auto
    ConstructWithActor(
        FCk_Handle& InHandle,
        AActor* InOwningActor) -> ECk_EntityScript_ConstructionFlow override;
};

// --------------------------------------------------------------------------------------------------------------------
