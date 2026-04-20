#include "CkSmCondition_RandomChance.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_RandomChance::
    Evaluate(
        FCk_Handle_SmCondition InHandle,
        FCk_Time InDeltaT) const
    -> bool
{
    return FMath::FRand() < _Probability.Get_Value();
}

// --------------------------------------------------------------------------------------------------------------------
