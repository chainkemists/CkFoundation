#include "CkEcsTemplate_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcsTemplate/CkEcsTemplate_Log.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_EcsTemplate_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_EcsTemplate_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_EcsTemplate_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_EcsTemplate_Setup::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_EcsTemplate_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EcsTemplate_Params& InParams,
            FFragment_EcsTemplate_Current& InCurrent) const
        -> void
    {
        // Add setup logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EcsTemplate_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _TransientEntity.Clear<FTag_EcsTemplate_Updated>();

        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_EcsTemplate_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EcsTemplate_Params& InParams,
            FFragment_EcsTemplate_Current& InCurrent,
            FFragment_EcsTemplate_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_EcsTemplate_Requests& InRequests)
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
        FProcessor_EcsTemplate_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_EcsTemplate_Params& InParams,
            FFragment_EcsTemplate_Current& InCurrent,
            const FCk_Request_EcsTemplate_ExampleRequest& InRequest)
        -> void
    {
        // Add request handling logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EcsTemplate_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EcsTemplate_Params& InParams,
            FFragment_EcsTemplate_Current& InCurrent) const
        -> void
    {
        // Add teardown logic here
    }

}

// --------------------------------------------------------------------------------------------------------------------