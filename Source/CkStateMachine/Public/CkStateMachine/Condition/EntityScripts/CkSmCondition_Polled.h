#pragma once

#include "CkSmCondition_EntityScript.h"

#include "CkCore/Time/CkTime.h"

#include "CkSmCondition_Polled.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKSTATEMACHINE_API UCk_SmCondition_Polled : public UCk_SmCondition_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmCondition_Polled);

    // ================================================================================================================
    // POLLED EVALUATION
    // ================================================================================================================

public:
    virtual auto
    Evaluate(
        FCk_Time InDeltaT) const -> bool;

protected:
    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|SM|Condition",
        DisplayName = "Evaluate")
    bool
    DoEvaluate(
        FCk_Handle InHandle,
        FCk_Time InDeltaT) const;
};

// --------------------------------------------------------------------------------------------------------------------
