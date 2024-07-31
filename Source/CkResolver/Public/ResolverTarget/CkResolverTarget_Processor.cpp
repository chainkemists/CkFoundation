#include "CkResolverTarget_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkNet/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ResolverTarget_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_ResolverTarget_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ResolverTarget_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_ResolverTarget_Requests& InRequests)
        {
            ck::algo::ForEachRequest(InRequests._ResolverRequests,
            [&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InRequest);
            });
        });
    }

    auto
        FProcessor_ResolverTarget_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_ResolverTarget_InitiateNewResolution& InNewResolution)
        -> void
    {
        UUtils_Signal_ResolverTarget_OnNewResolverDataBundle::Broadcast(InNewResolution.GetAndDestroyRequestHandle(),
            MakePayload(InHandle, InNewResolution.Get_DataBundle()));

        UUtils_Signal_ResolverTarget_OnNewResolverDataBundle::Broadcast(InHandle,
            MakePayload(InHandle, InNewResolution.Get_DataBundle()));
    }
}

// --------------------------------------------------------------------------------------------------------------------
