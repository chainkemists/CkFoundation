#pragma once

#include "CkSmCondition_EntityScript.h"

#include "CkSmTask_EntityScript.h"

#include "CkSmCondition_TaskResult.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType,
    meta = (DisplayName = "SM Condition: Task Result"))
class CKSTATEMACHINE_API UCk_SmCondition_TaskResult : public UCk_SmCondition_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmCondition_TaskResult);

    UCk_SmCondition_TaskResult()
    {
        _ConditionMode = ECk_SmConditionMode::Polled;
        _ResetBehavior = ECk_SmConditionResetBehavior::ResetEveryFrame;
    }

    auto
    Evaluate() const -> bool override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Condition|TaskResult",
        meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_SmTask_EntityScript> _TaskClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Condition|TaskResult",
        meta = (AllowPrivateAccess = true))
    ECk_SmTaskResult _ExpectedResult = ECk_SmTaskResult::Succeeded;

public:
    CK_PROPERTY_GET(_TaskClass);
    CK_PROPERTY_GET(_ExpectedResult);
};

// --------------------------------------------------------------------------------------------------------------------
