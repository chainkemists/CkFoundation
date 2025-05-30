#include "CkShapeBox_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Net/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ShapeBox_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeBox_Params& InParams,
            FFragment_ShapeBox_Current& InCurrent,
            FFragment_ShapeBox_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_ShapeBox_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InCurrent, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });
    }

    auto
        FProcessor_ShapeBox_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_ShapeBox_Params& InParams,
            FFragment_ShapeBox_Current& InCurrent,
            const FCk_Request_ShapeBox_UpdateDimensions& InRequest)
        -> void
    {
        // TODO:
        InCurrent._Dimensions = InRequest.Get_NewDimensions();
    }
}

// --------------------------------------------------------------------------------------------------------------------