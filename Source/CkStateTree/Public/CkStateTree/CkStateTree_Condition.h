#pragma once

#include "Blueprint/StateTreeConditionBlueprintBase.h"
#include "CkStateTree_Condition.generated.h"

// --------------------------------------------------------------------------------------------------------------------

struct FStateTreeExecutionContext;

/**
 * Base class for Blueprint based Conditions with invert support.
 */
UCLASS(Abstract, Blueprintable)
class CKSTATETREE_API UCk_StateTree_Condition : public UStateTreeConditionBlueprintBase
{
    GENERATED_BODY()

protected:
    virtual auto TestCondition(FStateTreeExecutionContext& Context) const -> bool override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition")
    bool Invert = false;
};

// --------------------------------------------------------------------------------------------------------------------
