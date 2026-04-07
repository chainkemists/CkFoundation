#include "CkShapeCapsule_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_ShapeCapsule_HandleRequests);

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

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
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
        const auto& NewDimensions = InRequest.Get_NewDimensions();

        if (InCurrent.Get_Dimensions() == NewDimensions)
        { return; }

        InCurrent._Dimensions = NewDimensions;
        UUtils_Signal_OnShapeCapsuleDimensionsChanged::Broadcast(InHandle, MakePayload(InHandle, NewDimensions));
    }
}

// --------------------------------------------------------------------------------------------------------------------