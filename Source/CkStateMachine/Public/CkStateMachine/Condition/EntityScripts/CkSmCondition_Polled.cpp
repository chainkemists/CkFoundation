#include "CkSmCondition_Polled.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_Polled::
    Evaluate() const
    -> bool
{
    auto Result = DoEvaluate(DoGet_ScriptEntity());
    return _NegateResult ? NOT Result : Result;
}

// --------------------------------------------------------------------------------------------------------------------
