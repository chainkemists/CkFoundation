// Copyright Saad Taame. All Rights Reserved.

#pragma once

#include "StateTreeConditionBase.h"
#include "CkStateTree_NativeCondition.generated.h"

// --------------------------------------------------------------------------------------------------------------------

struct FStateTreeExecutionContext;

/**
 * Base struct for native C++ State Tree conditions with invert support.
 * This is the USTRUCT counterpart to UCk_StateTree_Condition (UCLASS/Blueprint version).
 *
 * Subclasses override EvaluateCondition() instead of TestCondition().
 * The base handles the Invert flag automatically.
 *
 * Usage:
 *   USTRUCT(meta = (DisplayName = "My Condition", Category = "MyCategory"))
 *   struct FMyCondition : public FCk_StateTree_NativeCondition
 *   {
 *       GENERATED_BODY()
 *       virtual bool EvaluateCondition(FStateTreeExecutionContext& Context) const override;
 *   };
 */
USTRUCT(meta = (Hidden))
struct CKSTATETREE_API FCk_StateTree_NativeCondition : public FStateTreeConditionCommonBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Condition")
    bool Invert = false;

    virtual bool TestCondition(FStateTreeExecutionContext& Context) const override final;

protected:
    virtual bool EvaluateCondition(FStateTreeExecutionContext& Context) const;
};

// --------------------------------------------------------------------------------------------------------------------
