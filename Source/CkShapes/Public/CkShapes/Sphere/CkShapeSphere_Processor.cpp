#include "CkShapeSphere_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkShapes/CkShapes_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_ShapeSphere_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_ShapeSphere_CancelPendingRequests);

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
                // DoHandleRequest is void and has no rejection path, so reaching the line after
                // the call IS the success condition.
                auto Result = ECk_Request_OperationResult::Failed;
                const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

                DoHandleRequest(InHandle, InParams, InCurrent, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }

                Result = ECk_Request_OperationResult::Succeeded;
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
        const auto& NewDimensions = InRequest.Get_NewDimensions();

        if (InCurrent.Get_Dimensions() == NewDimensions)
        { return; }

        InCurrent._Dimensions = NewDimensions;
        UUtils_Signal_OnShapeSphereDimensionsChanged::Broadcast(InHandle, MakePayload(InHandle, NewDimensions));
#if WITH_EDITOR
        UCk_Utils_Shapes_UE::Notify_DimensionsChanged(InHandle.ConvertToHandle());
#endif
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ShapeSphere_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeSphere_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
