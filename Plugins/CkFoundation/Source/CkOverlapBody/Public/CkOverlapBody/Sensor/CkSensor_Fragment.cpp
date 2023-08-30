#include "CkSensor_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FCk_Fragment_Sensor_Current::
        FCk_Fragment_Sensor_Current(
            ECk_EnableDisable InEnableDisable)
        : _AttachedEntityAndActor(),
        _EnableDisable(InEnableDisable)
    {
    }

    // --------------------------------------------------------------------------------------------------------------------

    FCk_Fragment_Sensor_Params::
        FCk_Fragment_Sensor_Params(
            ParamsType InParams)
        : _Params(InParams)
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------