#include "CkSmCondition_Polled.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_Polled::
    Evaluate(
        FCk_Handle_SmCondition InHandle,
        FCk_Time InDeltaT) const
    -> bool
{
    const auto Result = DoEvaluate(InHandle, InDeltaT);
    return _NegateResult ? NOT Result : Result;
}

// --------------------------------------------------------------------------------------------------------------------
