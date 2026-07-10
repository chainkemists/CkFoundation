#include "CkSmCondition_RandomChance.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::statemachine
{
    auto
        Get_RandomChance_FrameProbability(
            float InProbabilityPerSecond,
            float InDeltaSeconds)
        -> float
    {
        const auto Clamped = FMath::Clamp(InProbabilityPerSecond, 0.0f, 1.0f);

        if (Clamped >= 1.0f)
        { return 1.0f; }

        if (InDeltaSeconds <= 0.0f)
        { return 0.0f; }

        return 1.0f - FMath::Pow(1.0f - Clamped, InDeltaSeconds);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_RandomChance::
    Evaluate(
        FCk_Handle_SmCondition InHandle,
        FCk_Time InDeltaT) const
    -> bool
{
    return FMath::FRand() < ck::statemachine::Get_RandomChance_FrameProbability(
        _Probability.Get_Value(), InDeltaT.Get_Seconds());
}

// --------------------------------------------------------------------------------------------------------------------
