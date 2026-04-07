#include "CkSfx_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkFx/CkFx_Log.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Sfx_HandleRequests);

namespace ck
{
    auto
        FProcessor_Sfx_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Sfx_Current& InComp,
            FFragment_Sfx_Requests& InRequestsComp) const
        -> void
    {
        // Add request handling logic here
    }
}

// --------------------------------------------------------------------------------------------------------------------
