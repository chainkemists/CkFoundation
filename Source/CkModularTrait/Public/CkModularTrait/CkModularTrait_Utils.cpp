#include "CkModularTrait_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ModularTrait_Condition_UE::
    Evaluate_Implementation(
        const FCk_Handle& InContext,
        const FInstancedStruct& InCustomParameters)
    -> ECk_Condition_Result
{
    return ECk_Condition_Result::Pass;
}

// --------------------------------------------------------------------------------------------------------------------
