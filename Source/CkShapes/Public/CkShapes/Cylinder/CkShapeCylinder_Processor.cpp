#include "CkShapeCylinder_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkNet/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
	auto
		FProcessor_ShapeCylinder_Setup::
		DoTick(
			TimeType InDeltaT)
		-> void
	{
		TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
	}

	auto
		FProcessor_ShapeCylinder_Setup::
		ForEachEntity(
			TimeType InDeltaT,
			HandleType InHandle,
			const FFragment_ShapeCylinder_Params& InParams,
			FFragment_ShapeCylinder_Current& InCurrent) const
		-> void
	{
		// Add setup logic here
	}

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ShapeCylinder_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _TransientEntity.Clear<FTag_ShapeCylinder_Updated>();

        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_ShapeCylinder_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeCylinder_Params& InParams,
            FFragment_ShapeCylinder_Current& InCurrent,
            FFragment_ShapeCylinder_Requests& InRequestsComp) const
        -> void
    {
    	InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_ShapeCylinder_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InCurrent, InRequest);
            }));
        });
    }

    auto
	    FProcessor_ShapeCylinder_HandleRequests::
		DoHandleRequest(
		    HandleType InHandle,
		    const FFragment_ShapeCylinder_Params& InParams,
            FFragment_ShapeCylinder_Current& InCurrent,
		    const FCk_Request_ShapeCylinder_ExampleRequest& InRequest)
		-> void
    {
        // Add request handling logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
	    FProcessor_ShapeCylinder_Teardown::
		ForEachEntity(
		    TimeType InDeltaT,
		    HandleType InHandle,
		    const FFragment_ShapeCylinder_Params& InParams,
		    FFragment_ShapeCylinder_Current& InCurrent) const
		-> void
    {
		// Add teardown logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ShapeCylinder_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ShapeCylinder_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_ShapeCylinder_Rep>& InRepComp) const
        -> void
    {
        UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_ShapeCylinder_Rep>(InHandle, [&](UCk_Fragment_ShapeCylinder_Rep* InLocalRepComp)
        {
            // Add replication logic here
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------