#include "CkMarker_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FCk_Fragment_Marker_Current::
        FCk_Fragment_Marker_Current(
            ECk_EnableDisable InEnableDisable)
        : _AttachedEntityAndActor()
        , _EnableDisable(InEnableDisable)
    {
    }

    // --------------------------------------------------------------------------------------------------------------------

    FCk_Fragment_Marker_Params::
        FCk_Fragment_Marker_Params(
            ParamsType InParams)
        : _Params(InParams)
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------


