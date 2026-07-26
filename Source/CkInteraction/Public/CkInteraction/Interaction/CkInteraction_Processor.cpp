#include "CkInteraction_Processor.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkInteraction/CkInteraction_Log.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkInteraction/Interaction/CkInteraction_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Interaction_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Interaction_EndPlay);

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

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
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
        ck::interaction::VeryVerbose(TEXT("Interaction [{}] EndInteraction with [{}]. Channel: [{}], Source: [{}], Target: [{}]"),
            InHandle, InRequest.Get_SuccessFail(), InParams.Get_Params().Get_InteractionChannel(),
            InParams.Get_Params().Get_Source(), InParams.Get_Params().Get_Target());
        InHandle.AddOrGet<FTag_Interaction_FinishedBroadcastSent>();
        UUtils_Signal_Interaction_OnInteractionFinished::Broadcast(InHandle, ck::MakePayload(InHandle, InRequest.Get_SuccessFail()));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Interaction_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Interaction_Params& InParams)
        -> void
    {
        // A destroyed-mid-flight interaction never reaches the request handler (it excludes
        // pending-kill), so the Failed outcome must be broadcast from here — the one window where
        // both parties' listeners are still bound and handles still resolve.
        if (InHandle.Has<FTag_Interaction_FinishedBroadcastSent>())
        { return; }

        InHandle.AddOrGet<FTag_Interaction_FinishedBroadcastSent>();

        ck::interaction::VeryVerbose(TEXT("Interaction [{}] destroyed mid-flight — broadcasting [{}] from EndPlay. Channel: [{}], Source: [{}], Target: [{}]"),
            InHandle, ECk_SucceededFailed::Failed, InParams.Get_Params().Get_InteractionChannel(),
            InParams.Get_Params().Get_Source(), InParams.Get_Params().Get_Target());
        UUtils_Signal_Interaction_OnInteractionFinished::Broadcast(InHandle, ck::MakePayload(InHandle, ECk_SucceededFailed::Failed));
    }
}

// --------------------------------------------------------------------------------------------------------------------