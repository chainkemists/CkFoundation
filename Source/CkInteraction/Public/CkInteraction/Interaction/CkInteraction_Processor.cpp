#include "CkInteraction_Processor.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkInteraction/CkInteraction_Log.h"

#include "CkNet/CkNet_Utils.h"

#include "CkInteraction/Interaction/CkInteraction_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
	auto
		FProcessor_Interaction_Setup::
		DoTick(
			TimeType InDeltaT)
		-> void
	{
		TProcessor::DoTick(InDeltaT);
	}

	auto
		FProcessor_Interaction_Setup::
		ForEachEntity(
			TimeType InDeltaT,
			HandleType InHandle,
			const FFragment_Interaction_Params& InParams) const
		-> void
	{
		if (InParams.Get_Params().Get_CompletionPolicy() == ECk_InteractionCompletionPolicy::Timed)
		{
			const auto& DurationAttributeParams = FCk_Fragment_FloatAttribute_ParamsData{
				TAG_InteractionDuration_FloatAttribute_Name,
				InParams.Get_Params()._InteractionDuration.Get_Seconds()};

			UCk_Utils_FloatAttribute_UE::Add(InHandle, DurationAttributeParams, ECk_Replication::DoesNotReplicate);
		}
		InHandle.Remove<MarkedDirtyBy>();
	}

	// --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Interaction_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _TransientEntity.Clear<FTag_Interaction_Updated>();

        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Interaction_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Interaction_Params& InParams,
            FFragment_Interaction_Requests& InRequestsComp) const
        -> void
    {
    	InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_Interaction_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InRequest);
            }));
        });
    }

    auto
	    FProcessor_Interaction_HandleRequests::
		DoHandleRequest(
		    HandleType InHandle,
		    const FFragment_Interaction_Params& InParams,
		    const FCk_Request_Interaction_EndInteraction& InRequest)
		-> void
    {
		UUtils_Signal_Interaction_OnInteractionFinished::Broadcast(InHandle, ck::MakePayload(InHandle, InRequest.Get_SuccessFail()));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
	    FProcessor_Interaction_Teardown::
		ForEachEntity(
		    TimeType InDeltaT,
		    HandleType InHandle,
		    const FFragment_Interaction_Params& InParams) const
		-> void
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------