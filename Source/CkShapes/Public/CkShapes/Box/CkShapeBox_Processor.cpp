#include "CkShapeBox_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkNet/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ShapeBox_Setup::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_ShapeBox_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeBox_Params& InParams,
            FFragment_ShapeBox_Current& InCurrent) const
        -> void
    {
        InCurrent._CurrentShape = InParams.Get_Params().Get_Shape();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ShapeBox_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _TransientEntity.Clear<FTag_ShapeBox_Updated>();

        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_ShapeBox_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeBox_Params& InParams,
            FFragment_ShapeBox_Current& InCurrent,
            FFragment_ShapeBox_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_ShapeBox_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InCurrent, InRequest);
            }));
        });
    }

    auto
        FProcessor_ShapeBox_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_ShapeBox_Params& InParams,
            FFragment_ShapeBox_Current& InCurrent,
            const FCk_Request_ShapeBox_UpdateShape& InRequest)
        -> void
    {
        InCurrent._CurrentShape = InRequest.Get_NewShape();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ShapeBox_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeBox_Params& InParams,
            FFragment_ShapeBox_Current& InCurrent) const
        -> void
    {
        // Add teardown logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ShapeBox_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ShapeBox_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_ShapeBox_Rep>& InRepComp) const
        -> void
    {
        UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_ShapeBox_Rep>(InHandle, [&](UCk_Fragment_ShapeBox_Rep* InLocalRepComp)
        {
            // Add replication logic here
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------