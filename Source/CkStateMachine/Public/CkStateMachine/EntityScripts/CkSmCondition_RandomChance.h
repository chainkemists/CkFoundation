#pragma once

#include "CkSmCondition_EntityScript.h"

#include "CkSmCondition_RandomChance.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType,
    meta = (DisplayName = "SM Condition: Random Chance"))
class CKSTATEMACHINE_API UCk_SmCondition_RandomChance : public UCk_SmCondition_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmCondition_RandomChance);

    UCk_SmCondition_RandomChance()
    {
        _ConditionMode = ECk_SmConditionMode::Polled;
        _ResetBehavior = ECk_SmConditionResetBehavior::ResetEveryFrame;
    }

    auto
    Evaluate() const -> bool override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Condition|RandomChance",
        meta = (AllowPrivateAccess = true, ClampMin = "0.0", ClampMax = "1.0"))
    float _Probability = 0.5f;

public:
    CK_PROPERTY_GET(_Probability);
};

// --------------------------------------------------------------------------------------------------------------------
