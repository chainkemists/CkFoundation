#include "CkAcceleration_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FCk_Fragment_Acceleration_Params::
        FCk_Fragment_Acceleration_Params(
            ParamsType InParams)
        : _Params(InParams)
    {
    }

    // --------------------------------------------------------------------------------------------------------------------

    FCk_Fragment_Acceleration_Current::
        FCk_Fragment_Acceleration_Current(
            FVector InAcceleration)
        : _CurrentAcceleration(InAcceleration)
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------
