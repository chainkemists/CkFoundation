#include "CkSmCondition_Polled.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_Polled::
    Evaluate(
        FCk_Time InDeltaT) const
    -> bool
{
    const auto Result = DoEvaluate(DoGet_ScriptEntity(), InDeltaT);
    return _NegateResult ? NOT Result : Result;
}

// --------------------------------------------------------------------------------------------------------------------
