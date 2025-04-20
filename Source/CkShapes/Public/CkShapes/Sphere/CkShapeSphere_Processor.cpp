#include "CkShapeSphere_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Net/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ShapeSphere_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeSphere_Params& InParams,
            FFragment_ShapeSphere_Current& InCurrent,
            FFragment_ShapeSphere_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_ShapeSphere_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InCurrent, InRequest);
            }));
        });
    }

    auto
        FProcessor_ShapeSphere_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_ShapeSphere_Params& InParams,
            FFragment_ShapeSphere_Current& InCurrent,
            const FCk_Request_ShapeSphere_UpdateDimensions& InRequest)
        -> void
    {
        // TODO:
        InCurrent._Dimensions = InRequest.Get_NewDimensions();
    }
}

// --------------------------------------------------------------------------------------------------------------------