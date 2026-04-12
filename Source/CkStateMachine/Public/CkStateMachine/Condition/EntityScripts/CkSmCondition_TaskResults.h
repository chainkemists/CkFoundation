#pragma once

#include "CkSmCondition_EventDriven.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkSmCondition_TaskResults.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType,
    meta = (DisplayName = "SM Condition: Task Results (Aggregate)"))
class CKSTATEMACHINE_API UCk_SmCondition_TaskResults : public UCk_SmCondition_EventDriven
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmCondition_TaskResults);

    UCk_SmCondition_TaskResults() = default;

protected:
    auto
    BeginPlay() -> void override;

    auto
    EndPlay() -> void override;

    UFUNCTION()
    void
    OnTaskFinished(
        FCk_Handle_SmTask InTaskHandle,
        ECk_SmTaskResult InResult);

    void
    DoEvaluateThreshold();

private:
    TArray<FCk_Handle_SmTask> _BoundTasks;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Condition|TaskResults",
        meta = (AllowPrivateAccess = true))
    ECk_SmCondition_TaskResultsCheck _Check = ECk_SmCondition_TaskResultsCheck::AllSucceeded;

public:
    CK_PROPERTY_GET(_Check);

private:
    int32 _TickTaskCount = 0;
    int32 _SucceededCount = 0;
    int32 _FailedCount = 0;
};

// --------------------------------------------------------------------------------------------------------------------
