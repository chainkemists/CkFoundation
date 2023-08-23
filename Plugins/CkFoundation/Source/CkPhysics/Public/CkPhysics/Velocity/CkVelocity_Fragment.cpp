#include "CkVelocity_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FCk_Fragment_Velocity_Params::
        FCk_Fragment_Velocity_Params(
            ParamsType InParams)
        : _Params(InParams)
    {
    }

    // --------------------------------------------------------------------------------------------------------------------

    FCk_Fragment_Velocity_Current::
        FCk_Fragment_Velocity_Current(
            FVector InVelocity)
        : _CurrentVelocity(InVelocity)
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------
