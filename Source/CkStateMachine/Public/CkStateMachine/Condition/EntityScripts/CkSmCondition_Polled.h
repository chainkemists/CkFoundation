#pragma once

#include "CkSmCondition_EntityScript.h"

#include "CkSmCondition_Polled.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKSTATEMACHINE_API UCk_SmCondition_Polled : public UCk_SmCondition_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmCondition_Polled);

    UCk_SmCondition_Polled()
    {
        _ResetBehavior = ECk_SmConditionResetBehavior::ResetEveryFrame;
    }

    // ================================================================================================================
    // POLLED EVALUATION
    // ================================================================================================================

public:
    virtual auto
    Evaluate() const -> bool;

protected:
    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|SM|Condition",
        DisplayName = "Evaluate")
    bool
    DoEvaluate(
        FCk_Handle InHandle) const;
};

// --------------------------------------------------------------------------------------------------------------------
