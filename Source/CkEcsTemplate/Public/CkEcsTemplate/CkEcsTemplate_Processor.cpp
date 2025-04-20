#include "CkEcsTemplate_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcsTemplate/CkEcsTemplate_Log.h"

#include "CkEcs/Net/CkNet_Utils.h"

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
        FProcessor_EcsTemplate_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EcsTemplate_Params& InParams,
            FFragment_EcsTemplate_Current& InCurrent) const
        -> void
    {
        // Add teardown logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EcsTemplate_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_EcsTemplate_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_EcsTemplate_Rep>& InRepComp) const
        -> void
    {
        UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_EcsTemplate_Rep>(InHandle, [&](UCk_Fragment_EcsTemplate_Rep* InLocalRepComp)
        {
            // Add replication logic here
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------