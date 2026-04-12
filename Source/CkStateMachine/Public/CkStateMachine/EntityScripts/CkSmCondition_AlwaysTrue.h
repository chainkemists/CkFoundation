#pragma once

#include "CkSmCondition_EntityScript.h"

#include "CkSmCondition_AlwaysTrue.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType,
    meta = (DisplayName = "SM Condition: Always True"))
class CKSTATEMACHINE_API UCk_SmCondition_AlwaysTrue : public UCk_SmCondition_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmCondition_AlwaysTrue);

    UCk_SmCondition_AlwaysTrue()
    {
        _ConditionMode = ECk_SmConditionMode::EventDriven;
        _ResetBehavior = ECk_SmConditionResetBehavior::Manual;
    }

protected:
    auto
    BeginPlay() -> void override;
};

// --------------------------------------------------------------------------------------------------------------------
