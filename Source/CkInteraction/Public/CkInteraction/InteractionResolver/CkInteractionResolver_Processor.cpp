#include "CkInteractionResolver_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkInteraction/CkInteraction_Log.h"

#include "CkInteraction/InteractionResolver/CkInteractionResolver_Utils.h"
#include "CkInteraction/InteractTarget/CkInteractTarget_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_InteractionResolver_HandleRequests);
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
            FFragment_InteractionResolver_Current& InCurrent,
            const FFragment_InteractionResolver_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_InteractionResolver_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InCurrent, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });
    }

    auto
        FProcessor_InteractionResolver_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver_Current& InCurrent,
            const FCk_Request_InteractionResolver_StartIntent& InRequest)
        -> void
    {
        const auto& Intent = InRequest.Get_Intent();

        CK_ENSURE_IF_NOT(ck::IsValid(Intent),
            TEXT("Cannot start invalid intent for resolver [{}]"), InHandle)
        { return; }

        if (InCurrent._ActiveIntents.Contains(Intent))
        {
            ck::interaction::VeryVerbose(TEXT("Intent [{}] already active for resolver [{}]"), Intent, InHandle);
            return;
        }

        InHandle.AddOrGet<FTag_InteractionResolver_IntentUpdated>();
        InCurrent._ActiveIntents.Add(Intent);

        ck::interaction::VeryVerbose(TEXT("Started intent [{}] for resolver [{}]. Active intents: {}"), Intent, InHandle, InCurrent._ActiveIntents.Num());
    }

    auto
        FProcessor_InteractionResolver_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver_Current& InCurrent,
            const FCk_Request_InteractionResolver_StopIntent& InRequest)
        -> void
    {
        const auto& Intent = InRequest.Get_Intent();

        CK_ENSURE_IF_NOT(ck::IsValid(Intent),
            TEXT("Cannot stop invalid intent for resolver [{}]"), InHandle)
        { return; }

        if (NOT InCurrent._ActiveIntents.Contains(Intent))
        {
            ck::interaction::VeryVerbose(TEXT("Intent [{}] was not active for resolver [{}]"), Intent, InHandle);
            return;
        }

        InHandle.AddOrGet<FTag_InteractionResolver_IntentUpdated>();

        // Get the current targets before clearing them
        const auto PreviousTargets = InCurrent.Get_CachedBestTargets().Find(Intent);
        const auto PreviousTargetsArray = PreviousTargets ? *PreviousTargets : TArray<FCk_Handle_InteractTarget>{};

        InCurrent._ActiveIntents.Remove(Intent);
        InCurrent._CachedBestTargets.Remove(Intent);

        ck::interaction::VeryVerbose(TEXT("Stopped intent [{}] for resolver [{}]. Remaining active intents: {}. Removing {} cached targets"),
            Intent, InHandle, InCurrent._ActiveIntents.Num(), PreviousTargetsArray.Num());

        // Broadcast with previous targets, empty new targets, and all previous targets as removed
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
            FFragment_InteractionResolver_Current& InCurrent,
            const FCk_Request_InteractionResolver_AddInteractTarget& InRequest)
        -> void
    {
        const auto& Target = InRequest.Get_Target();

        if (NOT ck::IsValid(Target))
        {
            ck::interaction::Warning(TEXT("Cannot add invalid InteractTarget to resolver [{}]"), InHandle);
            return;
        }

        if (InCurrent._AvailableTargets.Contains(Target))
        {
            ck::interaction::VeryVerbose(TEXT("InteractTarget [{}] already available for resolver [{}]"), Target, InHandle);
            return;
        }

        InCurrent._AvailableTargets.Add(Target);

        ck::interaction::VeryVerbose(TEXT("Added InteractTarget [{}] to resolver [{}]"), Target, InHandle);
    }

    auto
        FProcessor_InteractionResolver_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver_Current& InCurrent,
            const FCk_Request_InteractionResolver_RemoveInteractTarget& InRequest)
        -> void
    {
        const auto& Target = InRequest.Get_Target();

        if (NOT InCurrent._AvailableTargets.Contains(Target))
        {
            ck::interaction::VeryVerbose(TEXT("InteractTarget [{}] was not available for resolver [{}]"), Target, InHandle);
            return;
        }

        InCurrent._AvailableTargets.Remove(Target);

        ck::interaction::VeryVerbose(TEXT("Removed InteractTarget [{}] from resolver [{}]"), Target, InHandle);
    }

    auto
        FProcessor_InteractionResolver_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver_Current& InCurrent,
            const FCk_Request_InteractionResolver_RemoveAllTargetsByChannel& InRequest)
        -> void
    {
        const auto& Channel = InRequest.Get_Channel();

        CK_ENSURE_IF_NOT(ck::IsValid(Channel),
            TEXT("Cannot remove targets with invalid channel for resolver [{}]"), InHandle)
        { return; }

        auto TargetsToRemove = TArray<FCk_Handle_InteractTarget>{};

        for (const auto& Target : InCurrent.Get_AvailableTargets())
        {
            if (ck::IsValid(Target) &&
                UCk_Utils_InteractTarget_UE::Get_InteractionChannel(Target).MatchesTagExact(Channel))
            {
                TargetsToRemove.Add(Target);
            }
        }

        for (const auto& TargetToRemove : TargetsToRemove)
        {
            InCurrent._AvailableTargets.Remove(TargetToRemove);
        }

        ck::interaction::VeryVerbose(TEXT("Removed {} targets with channel [{}] from resolver [{}]"),
            TargetsToRemove.Num(), Channel, InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_InteractionResolver_Persistent::
        ForEachEntity(
            TimeType InDeltaT,
            const HandleType& InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver_Current& InCurrent)
            -> void
    {
        DoUpdateCachedTargets(InHandle, InParams, InCurrent);
    }

    auto
        FProcessor_InteractionResolver_Persistent::
        DoUpdateCachedTargets(
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver_Current& InCurrent)
        -> void
    {
        InHandle.Remove<FTag_InteractionResolver_IntentUpdated>();

        // Clean up invalid targets from available targets
        auto InvalidTargets = TArray<FCk_Handle_InteractTarget>{};
        for (const auto& Target : InCurrent.Get_AvailableTargets())
        {
            if (NOT ck::IsValid(Target))
            {
                InvalidTargets.Add(Target);
            }
        }

        for (const auto& InvalidTarget : InvalidTargets)
        {
            InCurrent._AvailableTargets.Remove(InvalidTarget);
        }

        if (InvalidTargets.Num() > 0)
        {
            ck::interaction::VeryVerbose(TEXT("Cleaned up {} invalid targets from resolver [{}]"),
                InvalidTargets.Num(), InHandle);
        }

        // Convert available targets set to array for processing
        auto AvailableTargetsArray = InCurrent.Get_AvailableTargets().Array();

        // Process each active intent
        for (const auto& Intent : InCurrent.Get_ActiveIntents())
        {
            // Get new resolved targets using immediate resolution
            const auto NewTargets = UCk_Utils_InteractionResolver_UE::DoResolveTargets_Internal(
                InHandle,
                Intent,
                AvailableTargetsArray
            );

            // Get previous cached targets
            const auto PreviousTargets = InCurrent.Get_CachedBestTargets().Find(Intent);
            const auto PreviousTargetsArray = PreviousTargets ? *PreviousTargets : TArray<FCk_Handle_InteractTarget>{};

            // Check if targets have changed (including order)
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

            // Update cache and fire delegate if changed
            if (TargetsChanged)
            {
                // Calculate removed targets
                auto RemovedTargets = TArray<FCk_Handle_InteractTarget>{};
                for (const auto& PrevTarget : PreviousTargetsArray)
                {
                    if (NOT NewTargets.Contains(PrevTarget))
                    {
                        RemovedTargets.Add(PrevTarget);
                    }
                }

                InCurrent._CachedBestTargets.Add(Intent, NewTargets);

                UUtils_Signal_InteractionResolver_OnBestTargetsChanged::Broadcast(InHandle,
                    ck::MakePayload(InHandle, Intent, PreviousTargetsArray, NewTargets, RemovedTargets));

                ck::interaction::VeryVerbose(TEXT("Best targets changed for resolver [{}], intent [{}]: {} -> {} targets ({} removed)"),
                    InHandle, Intent, PreviousTargetsArray.Num(), NewTargets.Num(), RemovedTargets.Num());
            }
        }

        // Remove cached targets for intents that are no longer active
        auto IntentsToRemove = TArray<FGameplayTag>{};
        for (const auto& [CachedIntent, CachedTargets] : InCurrent.Get_CachedBestTargets())
        {
            if (NOT InCurrent.Get_ActiveIntents().Contains(CachedIntent))
            {
                IntentsToRemove.Add(CachedIntent);
            }
        }

        for (const auto& IntentToRemove : IntentsToRemove)
        {
            InCurrent._CachedBestTargets.Remove(IntentToRemove);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_InteractionResolver_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver_Current& InCurrent)
        -> void
    {
        // Clear all active intents, cached targets, and available targets
        InCurrent._ActiveIntents.Empty();
        InCurrent._CachedBestTargets.Empty();
        InCurrent._AvailableTargets.Empty();
    }

}

// --------------------------------------------------------------------------------------------------------------------
