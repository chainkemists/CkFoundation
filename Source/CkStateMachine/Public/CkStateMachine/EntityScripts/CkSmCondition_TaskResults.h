#pragma once

#include "CkSmCondition_EntityScript.h"

#include "CkStateMachine/CkStateMachine_Fragment_Data.h"

#include "CkSmCondition_TaskResults.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType,
    meta = (DisplayName = "SM Condition: Task Results (Aggregate)"))
class CKSTATEMACHINE_API UCk_SmCondition_TaskResults : public UCk_SmCondition_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmCondition_TaskResults);

    UCk_SmCondition_TaskResults()
    {
        _ConditionMode = ECk_SmConditionMode::Polled;
        _ResetBehavior = ECk_SmConditionResetBehavior::ResetEveryFrame;
    }

    auto
    Evaluate() const -> bool override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Condition|TaskResults",
        meta = (AllowPrivateAccess = true))
    ECk_SmCondition_TaskResultsCheck _Check = ECk_SmCondition_TaskResultsCheck::AllSucceeded;

public:
    CK_PROPERTY_GET(_Check);
};

// --------------------------------------------------------------------------------------------------------------------
