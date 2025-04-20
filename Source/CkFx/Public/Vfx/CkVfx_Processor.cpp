#include "CkVfx_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkFx/CkFx_Log.h"

#include "CkEcs/Net/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Vfx_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Vfx_Current& InComp,
            FFragment_Vfx_Requests& InRequestsComp) const
        -> void
    {
        // TODO: Add request handling logic here
    }
}

// --------------------------------------------------------------------------------------------------------------------
