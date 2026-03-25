#pragma once

#include "CkSmCondition_EntityScript.h"

#include "CkSmCondition_Timer.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType,
    meta = (DisplayName = "SM Condition: Timer"))
class CKSTATEMACHINE_API UCk_SmCondition_Timer : public UCk_SmCondition_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmCondition_Timer);

    UCk_SmCondition_Timer()
    {
        _ConditionMode = ECk_SmConditionMode::Polled;
        _ResetBehavior = ECk_SmConditionResetBehavior::ResetEveryFrame;
    }

protected:
    auto
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams) -> ECk_EntityScript_ConstructionFlow override;

public:
    auto
    Evaluate() const -> bool override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Condition|Timer",
        meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _DurationSeconds = 1.0f;

public:
    CK_PROPERTY_GET(_DurationSeconds);

private:
    double _StartTimeSeconds = 0.0;
};

// --------------------------------------------------------------------------------------------------------------------
