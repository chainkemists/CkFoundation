#include "CkInteractionResolver_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkInteraction/CkInteraction_Log.h"

#include "CkInteraction/InteractionResolver/CkInteractionResolver_Utils.h"
#include "CkInteraction/InteractTarget/CkInteractTarget_Utils.h"

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_InteractionResolver_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_InteractionResolver_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_InteractionResolver_Persistent);
CK_REGISTER_PROCESSOR(ck::FProcessor_InteractionResolver_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_InteractionResolver_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _TransientEntity.Clear<FTag_InteractionResolver_Updated>();

        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_InteractionResolver_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver& InInteractionResolver,
            const FFragment_InteractionResolver_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_InteractionResolver_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                auto Result = ECk_Request_OperationResult::Failed;
                const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

                DoHandleRequest(InHandle, InParams, InInteractionResolver, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }

                Result = ECk_Request_OperationResult::Succeeded;
            }));
        });
    }

    auto
        FProcessor_InteractionResolver_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver& InInteractionResolver,
            const FCk_Request_InteractionResolver_StartIntent& InRequest)
        -> void
    {
        const auto& Intent = InRequest.Get_Intent();

        CK_ENSURE_IF_NOT(ck::IsValid(Intent),
            TEXT("Cannot start invalid intent for resolver [{}]"), InHandle)
        { return; }

        if (InInteractionResolver._ActiveIntents.Contains(Intent))
        {
            ck::interaction::VeryVerbose(TEXT("Intent [{}] already active for resolver [{}]"), Intent, InHandle);
            return;
        }

        InHandle.AddOrGet<FTag_InteractionResolver_ResolveDirty>();
        InInteractionResolver._ActiveIntents.Add(Intent);

        ck::interaction::VeryVerbose(TEXT("Started intent [{}] for resolver [{}]. Active intents: {}"), Intent, InHandle, InInteractionResolver._ActiveIntents.Num());
    }

    auto
        FProcessor_InteractionResolver_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver& InInteractionResolver,
            const FCk_Request_InteractionResolver_StopIntent& InRequest)
        -> void
    {
        const auto& Intent = InRequest.Get_Intent();

        CK_ENSURE_IF_NOT(ck::IsValid(Intent),
            TEXT("Cannot stop invalid intent for resolver [{}]"), InHandle)
        { return; }

        if (NOT InInteractionResolver._ActiveIntents.Contains(Intent))
        {
            ck::interaction::VeryVerbose(TEXT("Intent [{}] was not active for resolver [{}]"), Intent, InHandle);
            return;
        }

        InHandle.AddOrGet<FTag_InteractionResolver_ResolveDirty>();

        const auto PreviousTargets = InInteractionResolver.Get_CachedBestTargets().Find(Intent);
        const auto PreviousTargetsArray = PreviousTargets ? *PreviousTargets : TArray<FCk_Handle_InteractTarget>{};

        InInteractionResolver._ActiveIntents.Remove(Intent);
        InInteractionResolver._CachedBestTargets.Remove(Intent);

        ck::interaction::VeryVerbose(TEXT("Stopped intent [{}] for resolver [{}]. Remaining active intents: {}. Removing {} cached targets"),
            Intent, InHandle, InInteractionResolver._ActiveIntents.Num(), PreviousTargetsArray.Num());

        auto EmptyTargets = TArray<FCk_Handle_InteractTarget>{};
        UUtils_Signal_InteractionResolver_OnBestTargetsChanged::Broadcast(InHandle,
            ck::MakePayload(InHandle, Intent, PreviousTargetsArray, EmptyTargets, PreviousTargetsArray));

        ck::interaction::VeryVerbose(TEXT("Stopped intent [{}] for resolver [{}] and broadcasted transition from {} to 0 targets ({} removed)"),
            Intent, InHandle, PreviousTargetsArray.Num(), PreviousTargetsArray.Num());
    }

    auto
        FProcessor_InteractionResolver_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver& InInteractionResolver,
            const FCk_Request_InteractionResolver_AddInteractTarget& InRequest)
        -> void
    {
        const auto& Target = InRequest.Get_Target();

        if (ck::Is_NOT_Valid(Target))
        {
            ck::interaction::Warning(TEXT("Cannot add invalid InteractTarget to resolver [{}]"), InHandle);
            return;
        }

        if (InInteractionResolver._AvailableTargets.Contains(Target))
        {
            ck::interaction::VeryVerbose(TEXT("InteractTarget [{}] already available for resolver [{}]"), Target, InHandle);
            return;
        }

        InInteractionResolver._AvailableTargets.Add(Target);
        InHandle.AddOrGet<FTag_InteractionResolver_ResolveDirty>();

        ck::interaction::VeryVerbose(TEXT("Added InteractTarget [{}] to resolver [{}]"), Target, InHandle);
    }

    auto
        FProcessor_InteractionResolver_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver& InInteractionResolver,
            const FCk_Request_InteractionResolver_RemoveInteractTarget& InRequest)
        -> void
    {
        const auto& Target = InRequest.Get_Target();

        if (NOT InInteractionResolver._AvailableTargets.Contains(Target))
        {
            ck::interaction::VeryVerbose(TEXT("InteractTarget [{}] was not available for resolver [{}]"), Target, InHandle);
            return;
        }

        InInteractionResolver._AvailableTargets.Remove(Target);
        InHandle.AddOrGet<FTag_InteractionResolver_ResolveDirty>();

        ck::interaction::VeryVerbose(TEXT("Removed InteractTarget [{}] from resolver [{}]"), Target, InHandle);
    }

    auto
        FProcessor_InteractionResolver_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver& InInteractionResolver,
            const FCk_Request_InteractionResolver_RemoveAllTargetsByChannel& InRequest)
        -> void
    {
        const auto& Channel = InRequest.Get_Channel();

        CK_ENSURE_IF_NOT(ck::IsValid(Channel),
            TEXT("Cannot remove targets with invalid channel for resolver [{}]"), InHandle)
        { return; }

        auto TargetsToRemove = TArray<FCk_Handle_InteractTarget>{};

        for (const auto& Target : InInteractionResolver.Get_AvailableTargets())
        {
            if (ck::IsValid(Target) &&
                UCk_Utils_InteractTarget_UE::Get_InteractionChannel(Target).MatchesTagExact(Channel))
            {
                TargetsToRemove.Add(Target);
            }
        }

        for (const auto& TargetToRemove : TargetsToRemove)
        {
            InInteractionResolver._AvailableTargets.Remove(TargetToRemove);
        }

        if (NOT TargetsToRemove.IsEmpty())
        { InHandle.AddOrGet<FTag_InteractionResolver_ResolveDirty>(); }

        ck::interaction::VeryVerbose(TEXT("Removed {} targets with channel [{}] from resolver [{}]"),
            TargetsToRemove.Num(), Channel, InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_InteractionResolver_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_InteractionResolver_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_InteractionResolver_Persistent::
        ForEachEntity(
            TimeType InDeltaT,
            const HandleType& InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver& InInteractionResolver)
            -> void
    {
        DoUpdateCachedTargets(InHandle, InParams, InInteractionResolver);
    }

    auto
        FProcessor_InteractionResolver_Persistent::
        DoUpdateCachedTargets(
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver& InInteractionResolver)
        -> void
    {
        InHandle.Remove<FTag_InteractionResolver_ResolveDirty>();

        auto InvalidTargets = TArray<FCk_Handle_InteractTarget>{};
        for (const auto& Target : InInteractionResolver.Get_AvailableTargets())
        {
            if (ck::Is_NOT_Valid(Target))
            {
                InvalidTargets.Add(Target);
            }
        }

        for (const auto& InvalidTarget : InvalidTargets)
        {
            InInteractionResolver._AvailableTargets.Remove(InvalidTarget);
        }

        if (InvalidTargets.Num() > 0)
        {
            ck::interaction::VeryVerbose(TEXT("Cleaned up {} invalid targets from resolver [{}]"),
                InvalidTargets.Num(), InHandle);
        }

        auto AvailableTargetsArray = InInteractionResolver.Get_AvailableTargets().Array();

        for (const auto& Intent : InInteractionResolver.Get_ActiveIntents())
        {
            const auto NewTargets = UCk_Utils_InteractionResolver_UE::DoResolveTargets_Internal(
                InHandle,
                Intent,
                AvailableTargetsArray
            );

            const auto PreviousTargets = InInteractionResolver.Get_CachedBestTargets().Find(Intent);
            const auto PreviousTargetsArray = PreviousTargets ? *PreviousTargets : TArray<FCk_Handle_InteractTarget>{};

            const auto TargetsChanged = [&]() -> bool
            {
                if (NewTargets.Num() != PreviousTargetsArray.Num())
                { return true; }

                for (auto Index = 0; Index < NewTargets.Num(); ++Index)
                {
                    if (NewTargets[Index] != PreviousTargetsArray[Index])
                    { return true; }
                }

                return false;
            }();

            if (TargetsChanged)
            {
                auto RemovedTargets = TArray<FCk_Handle_InteractTarget>{};
                for (const auto& PrevTarget : PreviousTargetsArray)
                {
                    if (NOT NewTargets.Contains(PrevTarget))
                    {
                        RemovedTargets.Add(PrevTarget);
                    }
                }

                InInteractionResolver._CachedBestTargets.Add(Intent, NewTargets);

                UUtils_Signal_InteractionResolver_OnBestTargetsChanged::Broadcast(InHandle,
                    ck::MakePayload(InHandle, Intent, PreviousTargetsArray, NewTargets, RemovedTargets));

                ck::interaction::VeryVerbose(TEXT("Best targets changed for resolver [{}], intent [{}]: {} -> {} targets ({} removed)"),
                    InHandle, Intent, PreviousTargetsArray.Num(), NewTargets.Num(), RemovedTargets.Num());
            }
        }

        auto IntentsToRemove = TArray<FGameplayTag>{};
        for (const auto& [CachedIntent, CachedTargets] : InInteractionResolver.Get_CachedBestTargets())
        {
            if (NOT InInteractionResolver.Get_ActiveIntents().Contains(CachedIntent))
            {
                IntentsToRemove.Add(CachedIntent);
            }
        }

        for (const auto& IntentToRemove : IntentsToRemove)
        {
            InInteractionResolver._CachedBestTargets.Remove(IntentToRemove);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_InteractionResolver_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver& InInteractionResolver)
        -> void
    {
        InInteractionResolver._ActiveIntents.Empty();
        InInteractionResolver._CachedBestTargets.Empty();
        InInteractionResolver._AvailableTargets.Empty();
    }

}

// --------------------------------------------------------------------------------------------------------------------
