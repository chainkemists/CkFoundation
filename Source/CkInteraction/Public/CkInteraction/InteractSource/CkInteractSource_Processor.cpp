#include "CkInteractSource_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkInteraction/CkInteraction_Log.h"

#include "CkNet/CkNet_Utils.h"

#include "CkInteraction/InteractSource/CkInteractSource_Utils.h"
#include "CkInteraction/Interaction/CkInteraction_Utils.h"
#include "CkInteraction/InteractTarget/CkInteractTarget_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
	auto
		FProcessor_InteractSource_Setup::
		DoTick(
			TimeType InDeltaT)
		-> void
	{
		TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
	}

	auto
		FProcessor_InteractSource_Setup::
		ForEachEntity(
			TimeType InDeltaT,
			HandleType InHandle,
			const FFragment_InteractSource_Params& InParams,
			FFragment_InteractSource_Current& InComp) const
		-> void
	{
	}

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_InteractSource_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _TransientEntity.Clear<FTag_InteractSource_Updated>();

        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_InteractSource_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_InteractSource_Params& InParams,
            FFragment_InteractSource_Current& InComp,
            FFragment_InteractSource_Requests& InRequestsComp) const
        -> void
    {
    	InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_InteractSource_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InComp, InRequest);
            }));
        });
    }

    auto
	    FProcessor_InteractSource_HandleRequests::
		DoHandleRequest(
		    HandleType InHandle,
		    const FFragment_InteractSource_Params& InParams,
            FFragment_InteractSource_Current& InCurrent,
		    const FCk_Request_InteractSource_StartInteraction& InRequest) const
		-> void
    {
        auto InteractionEntity = InRequest.Get_Interaction();

		InCurrent._InteractionsPendingAdd.Remove(InRequest.Get_Interaction());

		UUtils_Signal_InteractSource_OnNewInteraction::Broadcast(InHandle, ck::MakePayload(InHandle, InteractionEntity));

		const auto& OnInteractionFinishedConnection = UUtils_Signal_Interaction_OnInteractionFinished::Bind<&FProcessor_InteractSource_HandleRequests::OnInteractionFinished>
	    (
	        this,
	        InteractionEntity,
	        ECk_Signal_BindingPolicy::FireIfPayloadInFlight,
	        ECk_Signal_PostFireBehavior::DoNothing
	    );
		InCurrent._InteractionFinishedSignals.Add(InteractionEntity, OnInteractionFinishedConnection);
    }

    auto
	    FProcessor_InteractSource_HandleRequests::
		DoHandleRequest(
		    HandleType InHandle,
		    const FFragment_InteractSource_Params& InParams,
		    FFragment_InteractSource_Current& InCurrent,
		    const FCk_Request_InteractSource_CancelInteraction& InRequest) const
		-> void
    {
		if (auto Interaction = UCk_Utils_InteractSource_UE::TryGet_CurrentInteractionsByTarget(InHandle, InRequest.Get_InteractTarget());
			ck::IsValid(Interaction))
		{
			UCk_Utils_Interaction_UE::Request_EndInteraction(Interaction, FCk_Request_Interaction_EndInteraction{ECk_SucceededFailed::Failed});
		}
    }

    auto
	    FProcessor_InteractSource_HandleRequests::
		OnInteractionFinished(
		    FCk_Handle_Interaction InteractionHandle,
		    ECk_SucceededFailed SucceededFailed) const
		-> void
    {
		CK_ENSURE_IF_NOT(ck::IsValid(InteractionHandle),
			TEXT("Interaction [{}] is NOT valid!"), InteractionHandle)
		{ return; }

		auto InteractSourceRawHandle = UCk_Utils_Interaction_UE::Get_InteractionSource(InteractionHandle);
		auto InteractSource = UCk_Utils_InteractSource_UE::Cast(InteractSourceRawHandle);

		CK_ENSURE_IF_NOT(ck::IsValid(InteractSource),
			TEXT("Interaction Source of Interaction [{}] is NOT valid when listening from the Interaction Source processor!"), InteractionHandle)
		{ return; }

		UUtils_Signal_InteractSource_OnInteractionFinished::Broadcast(InteractSource, ck::MakePayload(InteractSource, InteractionHandle, SucceededFailed));

		auto& Current = InteractSource.Get<FFragment_InteractSource_Current>();

		auto InteractionFinishedSignal = Current._InteractionFinishedSignals.Find(InteractionHandle);
		if (IsValid(InteractionFinishedSignal, IsValid_Policy_NullptrOnly{}))
		{
			InteractionFinishedSignal->release();
		}

		Current._InteractionFinishedSignals.Remove(InteractionHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
	    FProcessor_InteractSource_Teardown::
		ForEachEntity(
		    TimeType InDeltaT,
		    HandleType InHandle,
		    const FFragment_InteractSource_Params& InParams,
		    FFragment_InteractSource_Current& InComp) const
		-> void
    {
		// TODO: This processor doesn't get called, can cause issues if teardown is mid interaction!!!
		// Will need to investigate later
		for (auto& InteractionFinishedSignal : InComp._InteractionFinishedSignals)
		{
			InteractionFinishedSignal.Value.release();
			UCk_Utils_Interaction_UE::Request_EndInteraction(InteractionFinishedSignal.Key, FCk_Request_Interaction_EndInteraction{ECk_SucceededFailed::Failed});
		}
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_InteractSource_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_InteractSource_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_InteractSource_Rep>& InComp) const
        -> void
    {
        UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_InteractSource_Rep>(InHandle, [&](UCk_Fragment_InteractSource_Rep* InRepComp)
        {
            // Add replication logic here
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------