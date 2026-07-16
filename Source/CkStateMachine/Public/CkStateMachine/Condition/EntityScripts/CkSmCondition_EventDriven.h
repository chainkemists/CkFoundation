#pragma once

#include "CkSmCondition_EntityScript.h"

#include "CkSmCondition_EventDriven.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKSTATEMACHINE_API UCk_SmCondition_EventDriven : public UCk_SmCondition_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmCondition_EventDriven);

    UCk_SmCondition_EventDriven() = default;

    // --------------------------------------------------------------------------------------------------------------------

public:
    auto
    EnterCondition(
        FCk_Handle_SmCondition InHandle,
        ECk_Sm_NetContext InNetContext) -> void override;

    // --------------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|SM|Condition",
        DisplayName = "[Ck][SM] Mark Satisfied")
    void
    MarkSatisfied();

    UFUNCTION(BlueprintCallable,
        Category = "Ck|SM|Condition",
        DisplayName = "[Ck][SM] Mark Unsatisfied")
    void
    MarkUnsatisfied();
};

// --------------------------------------------------------------------------------------------------------------------
