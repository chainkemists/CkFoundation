#include "CkShapeCapsule_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkNet/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
	auto
		FProcessor_ShapeCapsule_Setup::
		DoTick(
			TimeType InDeltaT)
		-> void
	{
		TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
	}

	auto
		FProcessor_ShapeCapsule_Setup::
		ForEachEntity(
			TimeType InDeltaT,
			HandleType InHandle,
			const FFragment_ShapeCapsule_Params& InParams,
			FFragment_ShapeCapsule_Current& InCurrent) const
		-> void
	{
		// Add setup logic here
	}

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ShapeCapsule_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _TransientEntity.Clear<FTag_ShapeCapsule_Updated>();

        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_ShapeCapsule_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeCapsule_Params& InParams,
            FFragment_ShapeCapsule_Current& InCurrent,
            FFragment_ShapeCapsule_Requests& InRequestsComp) const
        -> void
    {
    	InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_ShapeCapsule_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InCurrent, InRequest);
            }));
        });
    }

    auto
	    FProcessor_ShapeCapsule_HandleRequests::
		DoHandleRequest(
		    HandleType InHandle,
		    const FFragment_ShapeCapsule_Params& InParams,
            FFragment_ShapeCapsule_Current& InCurrent,
		    const FCk_Request_ShapeCapsule_ExampleRequest& InRequest)
		-> void
    {
        // Add request handling logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
	    FProcessor_ShapeCapsule_Teardown::
		ForEachEntity(
		    TimeType InDeltaT,
		    HandleType InHandle,
		    const FFragment_ShapeCapsule_Params& InParams,
		    FFragment_ShapeCapsule_Current& InCurrent) const
		-> void
    {
		// Add teardown logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ShapeCapsule_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ShapeCapsule_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_ShapeCapsule_Rep>& InRepComp) const
        -> void
    {
        UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_ShapeCapsule_Rep>(InHandle, [&](UCk_Fragment_ShapeCapsule_Rep* InLocalRepComp)
        {
            // Add replication logic here
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------