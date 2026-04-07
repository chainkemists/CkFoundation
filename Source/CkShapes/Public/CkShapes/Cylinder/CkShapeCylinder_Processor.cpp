#include "CkShapeCylinder_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_ShapeCylinder_HandleRequests);

namespace ck
{
    auto
        FProcessor_ShapeCylinder_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeCylinder_Params& InParams,
            FFragment_ShapeCylinder_Current& InCurrent,
            FFragment_ShapeCylinder_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_ShapeCylinder_Requests& InRequests)
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
        FProcessor_ShapeCylinder_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_ShapeCylinder_Params& InParams,
            FFragment_ShapeCylinder_Current& InCurrent,
            const FCk_Request_ShapeCylinder_UpdateDimensions& InRequest)
        -> void
    {
        const auto& NewDimensions = InRequest.Get_NewDimensions();

        if (InCurrent.Get_Dimensions() == NewDimensions)
        { return; }

        InCurrent._Dimensions = NewDimensions;
        UUtils_Signal_OnShapeCylinderDimensionsChanged::Broadcast(InHandle, MakePayload(InHandle, NewDimensions));
    }
}

// --------------------------------------------------------------------------------------------------------------------