#include "CkInteractTarget_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkInteraction/CkInteraction_Log.h"

#include "CkEcs/Net/CkNet_Utils.h"

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
            FFragment_InteractTarget_Current& InComp)
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

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
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

        const auto CanInteractResult = UCk_Utils_InteractTarget_UE::Get_CanInteractWith(InHandle, InteractSourceRawHandle);
        if (CanInteractResult != ECk_CanInteractWithResult::CanInteractWith)
        {
            ck::interaction::VeryVerbose(TEXT("InteractTarget [{}] rejected StartInteraction from source [{}]. Channel: [{}]. Result: [{}]"),
                InHandle, InteractSourceRawHandle, InParams.Get_Params().Get_InteractionChannel(), CanInteractResult);
            return;
        }

        ck::interaction::VeryVerbose(TEXT("InteractTarget [{}] creating interaction. Channel: [{}], Policy: [{}], Duration: {}s, Source: [{}]"),
            InHandle, InParams.Get_Params().Get_InteractionChannel(), InParams.Get_Params().Get_CompletionPolicy(),
            InParams.Get_Params().Get_InteractionDuration().Get_Seconds(), InteractSourceRawHandle);

        auto InteractionEntity = UCk_Utils_Interaction_UE::Add(InHandle,
            FCk_Fragment_Interaction_ParamsData(
                InParams.Get_Params().Get_InteractionChannel(),
                InteractSourceRawHandle,
                InteractInstigatorRawHandle,
                InHandle,
                InParams.Get_Params().Get_CompletionPolicy(),
                InParams.Get_Params().Get_InteractionDuration()));

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
        if (auto InteractSource = UCk_Utils_InteractSource_UE::Cast(InteractSourceRawHandle);
            ck::IsValid(InteractSource))
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
        // Collect all interactions from this source first to avoid mutating the record while iterating
        auto InteractionsToCancel = TArray<FCk_Handle_Interaction>{};
        UCk_Utils_Interaction_UE::ForEach(InHandle, [&](const FCk_Handle_Interaction& InInteraction)
        {
            if (UCk_Utils_Interaction_UE::Get_InteractionSource(InInteraction) == InRequest.Get_InteractSource())
            {
                InteractionsToCancel.Add(InInteraction);
            }
        });

        if (InteractionsToCancel.IsEmpty())
        {
            ck::interaction::VeryVerbose(TEXT("InteractTarget [{}] CancelInteraction: no matching interactions found for source [{}]"),
                InHandle, InRequest.Get_InteractSource());
        }

        for (auto& MatchingInteraction : InteractionsToCancel)
        {
            ck::interaction::VeryVerbose(TEXT("InteractTarget [{}] cancelling interaction [{}] from source [{}]. Channel: [{}]"),
                InHandle, MatchingInteraction, InRequest.Get_InteractSource(), UCk_Utils_InteractTarget_UE::Get_InteractionChannel(InHandle));
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

        ck::interaction::VeryVerbose(TEXT("InteractTarget [{}] OnInteractionFinished: interaction [{}] finished with [{}]. Channel: [{}]"),
            InteractTarget, InteractionHandle, SucceededFailed, UCk_Utils_InteractTarget_UE::Get_InteractionChannel(InteractTarget));

        UUtils_Signal_InteractTarget_OnInteractionFinished::Broadcast(InteractTarget, ck::MakePayload(InteractTarget, InteractionHandle, SucceededFailed));

        auto& Current = InteractTarget.Get<FFragment_InteractTarget_Current>();

        if (auto InteractionFinishedSignal = Current._InteractionFinishedSignals.Find(InteractionHandle);
            ck::IsValid(InteractionFinishedSignal, IsValid_Policy_NullptrOnly{}))
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
        FProcessor_InteractTarget_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_InteractTarget_Params& InParams,
            FFragment_InteractTarget_Current& InComp)
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

}

// --------------------------------------------------------------------------------------------------------------------