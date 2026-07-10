#pragma once

#include "CkSmCondition_Polled.h"

#include "CkCore/Math/ValueRange/CkValueRange.h"

#include "CkSmCondition_RandomChance.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::statemachine
{
    // Converts the per-SECOND fire probability into the equivalent per-evaluation probability
    // for a frame of InDeltaSeconds: 1 - (1-p)^dt. Compounding across frames is tick-rate
    // invariant — P(not fired after 1s of evaluation) = (1-p)^Σdt = (1-p)^1 at any frame rate.
    // Exposed for unit testing (Ck.StateMachine.UnitTests.RandomChance.*).
    CKSTATEMACHINE_API auto
    Get_RandomChance_FrameProbability(
        float InProbabilityPerSecond,
        float InDeltaSeconds) -> float;
}

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
    // Probability that the condition fires within ONE SECOND of continuous evaluation,
    // frame-rate independent (the per-frame roll is normalized by DeltaT — see
    // ck::statemachine::Get_RandomChance_FrameProbability). 1.0 fires on the first evaluation;
    // 0.0 never fires. A raw per-evaluation roll would converge to "always fires" at a rate
    // that scales with frame rate, making the knob meaningless.
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Condition|RandomChance",
        meta = (AllowPrivateAccess = true))
    FCk_FloatRange_0to1 _Probability = FCk_FloatRange_0to1{0.5};

public:
    CK_PROPERTY_GET(_Probability);
};

// --------------------------------------------------------------------------------------------------------------------
