#include "CkStateTree_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkStateTree/CkStateTree_Log.h"

#include "CkEcs/Net/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_StateTree_Setup::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_StateTree_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_StateTree_Params& InParams,
            FFragment_StateTree_Current& InCurrent) const
        -> void
    {
        // Add setup logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_StateTree_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _TransientEntity.Clear<FTag_StateTree_Updated>();

        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_StateTree_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_StateTree_Params& InParams,
            FFragment_StateTree_Current& InCurrent,
            FFragment_StateTree_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_StateTree_Requests& InRequests)
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
        FProcessor_StateTree_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_StateTree_Params& InParams,
            FFragment_StateTree_Current& InCurrent,
            const FCk_Request_StateTree_ExampleRequest& InRequest)
        -> void
    {
        // Add request handling logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_StateTree_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_StateTree_Params& InParams,
            FFragment_StateTree_Current& InCurrent) const
        -> void
    {
        // Add teardown logic here
    }

}

// --------------------------------------------------------------------------------------------------------------------