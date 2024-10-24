#include "CkInteractTarget_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkInteraction/CkInteraction_Log.h"

#include "CkNet/CkNet_Utils.h"

#include "CkInteraction/InteractSource/CkInteractSource_Utils.h"
#include "CkInteraction/Interaction/CkInteraction_Fragment.h"
#include "CkInteraction/Interaction/CkInteraction_Utils.h"
#include "CkInteraction/InteractTarget/CkInteractTarget_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
	auto
		FProcessor_InteractTarget_Setup::
		DoTick(
			TimeType InDeltaT)
		-> void
	{
		TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
	}

	auto
		FProcessor_InteractTarget_Setup::
		ForEachEntity(
			TimeType InDeltaT,
			HandleType InHandle,
			const FFragment_InteractTarget_Params& InParams,
			FFragment_InteractTarget_Current& InComp) const
		-> void
	{
	}

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_InteractTarget_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _TransientEntity.Clear<FTag_InteractTarget_Updated>();

        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_InteractTarget_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_InteractTarget_Params& InParams,
            FFragment_InteractTarget_Current& InComp,
            FFragment_InteractTarget_Requests& InRequestsComp) const
        -> void
    {
    	InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_InteractTarget_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InComp, InRequest);
            }));
        });
    }

    auto
	    FProcessor_InteractTarget_HandleRequests::
		DoHandleRequest(
		    HandleType InHandle,
		    const FFragment_InteractTarget_Params& InParams,
            FFragment_InteractTarget_Current& InCurrent,
		    const FCk_Try_InteractTarget_StartInteraction& InRequest) const
		-> void
    {
		const auto& InteractSourceRawHandle = InRequest.Get_InteractSource();
		const auto& InteractInstigatorRawHandle = InRequest.Get_InteractInstigator();

		if (UCk_Utils_InteractTarget_UE::Get_CanInteractWith(InHandle, InteractSourceRawHandle) != ECk_CanInteractWithResult::CanInteractWith)
		{ return; }

        auto InteractionEntity = UCk_Utils_Interaction_UE::Add(InHandle,
        	FCk_Fragment_Interaction_ParamsData(
        		InParams.Get_Params().Get_InteractionChannel(),
        		InteractSourceRawHandle,
        		InteractInstigatorRawHandle,
        		InHandle,
        		InParams.Get_Params().Get_CompletionPolicy(),
        		InParams.Get_Params().Get_InteractionDuration(),
        		InParams.Get_Params().Get_InteractionConstructionScript()));

		UUtils_Signal_InteractTarget_OnNewInteraction::Broadcast(InHandle, ck::MakePayload(InHandle, InteractionEntity));

		const auto& OnInteractionFinishedConnection = UUtils_Signal_Interaction_OnInteractionFinished::Bind<&FProcessor_InteractTarget_HandleRequests::OnInteractionFinished>
	    (
	        this,
	        InteractionEntity,
	        ECk_Signal_BindingPolicy::FireIfPayloadInFlight,
	        ECk_Signal_PostFireBehavior::DoNothing
	    );
		InCurrent._InteractionFinishedSignals.Add(InteractionEntity, OnInteractionFinishedConnection);

		// This does cause InteractTarget to rely on InteractSource which is not ideal, but works for now and matches the pattern in Resolver
		auto InteractSource = UCk_Utils_InteractSource_UE::Cast(InteractSourceRawHandle);
		if (ck::IsValid(InteractSource))
		{
			UCk_Utils_InteractSource_UE::Request_StartInteraction(InteractSource, FCk_Request_InteractSource_StartInteraction{InteractionEntity});
		}
    }

    auto
	    FProcessor_InteractTarget_HandleRequests::
		DoHandleRequest(
		    HandleType InHandle,
		    const FFragment_InteractTarget_Params& InParams,
		    FFragment_InteractTarget_Current& InCurrent,
		    const FCk_Request_InteractTarget_CancelInteraction& InRequest) const
		-> void
    {
		auto MatchingInteraction = UCk_Utils_InteractTarget_UE::TryGet_Interaction(InHandle, InRequest.Get_InteractSource());
		if (ck::IsValid(MatchingInteraction))
		{
			UCk_Utils_Interaction_UE::Request_EndInteraction(MatchingInteraction, FCk_Request_Interaction_EndInteraction{ECk_SucceededFailed::Failed});
		}
    }

    auto
	    FProcessor_InteractTarget_HandleRequests::
		OnInteractionFinished(
		    FCk_Handle_Interaction InteractionHandle,
		    ECk_SucceededFailed SucceededFailed) const
		-> void
    {
		CK_ENSURE_IF_NOT(ck::IsValid(InteractionHandle),
			TEXT("Interaction [{}] is NOT valid!"), InteractionHandle)
		{ return; }

		auto InteractTargetRawHandle = UCk_Utils_Interaction_UE::Get_InteractionTarget(InteractionHandle);
		auto InteractTarget = UCk_Utils_InteractTarget_UE::Cast(InteractTargetRawHandle);

		CK_ENSURE_IF_NOT(ck::IsValid(InteractTarget),
			TEXT("Interaction Target of Interaction [{}] is NOT valid when listening from the Interaction Target processor!"), InteractionHandle)
		{ return; }

		UUtils_Signal_InteractTarget_OnInteractionFinished::Broadcast(InteractTarget, ck::MakePayload(InteractTarget, InteractionHandle, SucceededFailed));

		auto& Current = InteractTarget.Get<FFragment_InteractTarget_Current>();

		auto InteractionFinishedSignal = Current._InteractionFinishedSignals.Find(InteractionHandle);
		if (IsValid(InteractionFinishedSignal, IsValid_Policy_NullptrOnly{}))
		{
			InteractionFinishedSignal->release();
		}

		Current._InteractionFinishedSignals.Remove(InteractionHandle);

		UCk_Utils_Interaction_UE::RecordOfInteractions_Utils::Request_Disconnect(InteractTarget, InteractionHandle);

		// Since InteractTarget creates interaction entities, it's also responsible for destroying them
		UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InteractionHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
	    FProcessor_InteractTarget_Teardown::
		ForEachEntity(
		    TimeType InDeltaT,
		    HandleType InHandle,
		    const FFragment_InteractTarget_Params& InParams,
		    FFragment_InteractTarget_Current& InComp) const
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
        FProcessor_InteractTarget_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_InteractTarget_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_InteractTarget_Rep>& InComp) const
        -> void
    {
        UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_InteractTarget_Rep>(InHandle, [&](UCk_Fragment_InteractTarget_Rep* InRepComp)
        {
            // Add replication logic here
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------