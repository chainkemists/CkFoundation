#include "CkInteraction_Processor.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkInteraction/CkInteraction_Log.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkInteraction/Interaction/CkInteraction_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Interaction_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Interaction_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Interaction_Params& InParams,
            FFragment_Interaction_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_Interaction_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InRequest);
            }));
        });
    }

    auto
        FProcessor_Interaction_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Interaction_Params& InParams,
            const FCk_Request_Interaction_EndInteraction& InRequest)
        -> void
    {
        UUtils_Signal_Interaction_OnInteractionFinished::Broadcast(InHandle, ck::MakePayload(InHandle, InRequest.Get_SuccessFail()));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Interaction_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Interaction_Params& InParams)
        -> void
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------