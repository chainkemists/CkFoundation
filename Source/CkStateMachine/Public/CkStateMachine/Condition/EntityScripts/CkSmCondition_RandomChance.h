#pragma once

#include "CkSmCondition_Polled.h"

#include "CkCore/Math/ValueRange/CkValueRange.h"

#include "CkSmCondition_RandomChance.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType,
    meta = (DisplayName = "SM Condition: Random Chance"))
class CKSTATEMACHINE_API UCk_SmCondition_RandomChance : public UCk_SmCondition_Polled
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmCondition_RandomChance);

    UCk_SmCondition_RandomChance() = default;

    auto
    Evaluate(
        FCk_Handle_SmCondition InHandle,
        FCk_Time InDeltaT) const -> bool override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Condition|RandomChance",
        meta = (AllowPrivateAccess = true))
    FCk_FloatRange_0to1 _Probability = FCk_FloatRange_0to1{0.5};

public:
    CK_PROPERTY_GET(_Probability);
};

// --------------------------------------------------------------------------------------------------------------------
