#include "CkSensor_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FFragment_Sensor_Current::
        FFragment_Sensor_Current(
            ECk_EnableDisable InEnableDisable)
        : _AttachedEntityAndActor(),
        _EnableDisable(InEnableDisable)
    {
    }

    // --------------------------------------------------------------------------------------------------------------------

    FFragment_Sensor_Params::
        FFragment_Sensor_Params(
            ParamsType InParams)
        : _Params(InParams)
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------