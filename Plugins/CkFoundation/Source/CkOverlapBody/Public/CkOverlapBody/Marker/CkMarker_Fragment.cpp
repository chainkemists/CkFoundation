#include "CkMarker_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FFragment_Marker_Current::
        FFragment_Marker_Current(
            ECk_EnableDisable InEnableDisable)
        : _AttachedEntityAndActor()
        , _EnableDisable(InEnableDisable)
    {
    }

    // --------------------------------------------------------------------------------------------------------------------

    FFragment_Marker_Params::
        FFragment_Marker_Params(
            ParamsType InParams)
        : _Params(InParams)
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------


