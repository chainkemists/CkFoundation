#include "CkShapeCapsule_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkNet/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ShapeCapsule_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeCapsule_Params& InParams,
            FFragment_ShapeCapsule_Current& InCurrent,
            FFragment_ShapeCapsule_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_ShapeCapsule_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InCurrent, InRequest);
            }));
        });
    }

    auto
        FProcessor_ShapeCapsule_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_ShapeCapsule_Params& InParams,
            FFragment_ShapeCapsule_Current& InCurrent,
            const FCk_Request_ShapeCapsule_UpdateDimensions& InRequest)
        -> void
    {
        // TODO:
        InCurrent._Dimensions = InRequest.Get_NewDimensions();
    }
}

// --------------------------------------------------------------------------------------------------------------------