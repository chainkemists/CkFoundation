#include "CkShapeSphere_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkNet/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ShapeSphere_Setup::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_ShapeSphere_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeSphere_Params& InParams,
            FFragment_ShapeSphere_Current& InCurrent) const
        -> void
    {
        InCurrent._CurrentShape = InParams.Get_Params().Get_Shape();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ShapeSphere_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _TransientEntity.Clear<FTag_ShapeSphere_Updated>();

        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

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
                DoHandleRequest(InHandle, InParams, InCurrent, InRequest);
            }));
        });
    }

    auto
        FProcessor_ShapeSphere_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_ShapeSphere_Params& InParams,
            FFragment_ShapeSphere_Current& InCurrent,
            const FCk_Request_ShapeSphere_UpdateShape& InRequest)
        -> void
    {
        InCurrent._CurrentShape = InRequest.Get_NewShape();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ShapeSphere_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeSphere_Params& InParams,
            FFragment_ShapeSphere_Current& InCurrent) const
        -> void
    {
        // Add teardown logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ShapeSphere_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ShapeSphere_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_ShapeSphere_Rep>& InRepComp) const
        -> void
    {
        UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_ShapeSphere_Rep>(InHandle, [&](UCk_Fragment_ShapeSphere_Rep* InLocalRepComp)
        {
            // Add replication logic here
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------
