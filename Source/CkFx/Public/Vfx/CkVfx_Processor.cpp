#include "CkVfx_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkFx/CkFx_Log.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Vfx_HandleRequests);

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
