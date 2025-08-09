#include "CkPredictedVelocity_Utils.h"

#include "CkPredictedVelocity_Fragment.h"
#include "CkPhysics/CkPhysics_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PredictedVelocity_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_PredictedVelocity_ParamsData& InParams)
    -> void
{
    InHandle.Add<ck::FFragment_PredictedVelocity_Params>(InParams);
    InHandle.Add<ck::FFragment_PredictedVelocity_Current>();
}

auto
    UCk_Utils_PredictedVelocity_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_PredictedVelocity_Current>();
}

auto
    UCk_Utils_PredictedVelocity_UE::
    Ensure(
        const FCk_Handle& InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have PredictedVelocity"), InHandle)
    { return false; }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------