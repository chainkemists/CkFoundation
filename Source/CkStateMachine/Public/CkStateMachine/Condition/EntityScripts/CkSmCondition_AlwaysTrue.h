#pragma once

#include "CkSmCondition_EventDriven.h"

#include "CkSmCondition_AlwaysTrue.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType,
    meta = (DisplayName = "SM Condition: Always True"))
class CKSTATEMACHINE_API UCk_SmCondition_AlwaysTrue : public UCk_SmCondition_EventDriven
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmCondition_AlwaysTrue);

    UCk_SmCondition_AlwaysTrue() = default;

protected:
    auto
    BeginPlay() -> void override;
};

// --------------------------------------------------------------------------------------------------------------------
