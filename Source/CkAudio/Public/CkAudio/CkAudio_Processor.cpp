#include "CkAudio_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkAudio/CkAudio_Log.h"

#include "CkEcs/Net/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Audio_Setup::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Audio_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Audio_Params& InParams,
            FFragment_Audio_Current& InCurrent) const
        -> void
    {
        // Add setup logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Audio_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _TransientEntity.Clear<FTag_Audio_Updated>();

        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Audio_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Audio_Params& InParams,
            FFragment_Audio_Current& InCurrent,
            FFragment_Audio_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_Audio_Requests& InRequests)
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
        FProcessor_Audio_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Audio_Params& InParams,
            FFragment_Audio_Current& InCurrent,
            const FCk_Request_Audio_ExampleRequest& InRequest)
        -> void
    {
        // Add request handling logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Audio_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Audio_Params& InParams,
            FFragment_Audio_Current& InCurrent) const
        -> void
    {
        // Add teardown logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Audio_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Audio_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Audio_Rep>& InRepComp) const
        -> void
    {
        UCk_Utils_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_Audio_Rep>(InHandle, [&](UCk_Fragment_Audio_Rep* InLocalRepComp)
        {
            // Add replication logic here
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------