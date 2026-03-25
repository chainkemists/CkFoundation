#pragma once

#include "CkSmCondition_EntityScript.h"

#include "CkSmCondition_SubSmFinished.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType,
    meta = (DisplayName = "SM Condition: Sub-SM Finished"))
class CKSTATEMACHINE_API UCk_SmCondition_SubSmFinished : public UCk_SmCondition_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmCondition_SubSmFinished);

    UCk_SmCondition_SubSmFinished()
    {
        _ConditionMode = ECk_SmConditionMode::Polled;
        _ResetBehavior = ECk_SmConditionResetBehavior::ResetEveryFrame;
    }

    auto
    Evaluate() const -> bool override;
};

// --------------------------------------------------------------------------------------------------------------------
