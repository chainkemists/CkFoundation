#include "CkLocator_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkNet/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
	auto
		FProcessor_Locator_Setup::
		DoTick(
			TimeType InDeltaT)
		-> void
	{
		TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
	}

	auto
		FProcessor_Locator_Setup::
		ForEachEntity(
			TimeType InDeltaT,
			HandleType InHandle,
			const FFragment_Locator_Params& InParams,
			FFragment_Locator_Current& InCurrent) const
		-> void
	{
		// Add setup logic here
	}

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Locator_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _TransientEntity.Clear<FTag_Locator_Updated>();

        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Locator_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Locator_Params& InParams,
            FFragment_Locator_Current& InCurrent,
            FFragment_Locator_Requests& InRequestsComp) const
        -> void
    {
    	InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_Locator_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InCurrent, InRequest);
            }));
        });
    }

    auto
	    FProcessor_Locator_HandleRequests::
		DoHandleRequest(
		    HandleType InHandle,
		    const FFragment_Locator_Params& InParams,
            FFragment_Locator_Current& InCurrent,
		    const FCk_Request_Locator_ExampleRequest& InRequest)
		-> void
    {
        // Add request handling logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
	    FProcessor_Locator_Teardown::
		ForEachEntity(
		    TimeType InDeltaT,
		    HandleType InHandle,
		    const FFragment_Locator_Params& InParams,
		    FFragment_Locator_Current& InCurrent) const
		-> void
    {
		// Add teardown logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Locator_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Locator_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Locator_Rep>& InRepComp) const
        -> void
    {
        UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_Locator_Rep>(InHandle, [&](UCk_Fragment_Locator_Rep* InLocalRepComp)
        {
            // Add replication logic here
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------