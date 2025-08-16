#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

#include <GameplayTagContainer.h>

#include "CkCueBase_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKCUE_API UCk_CueBase_EntityScript : public UCk_EntityScript_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_CueBase_EntityScript);

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Cue",
        meta = (AllowPrivateAccess = true))
    FGameplayTag _CueName;

public:
    CK_PROPERTY_GET(_CueName);
};

// --------------------------------------------------------------------------------------------------------------------