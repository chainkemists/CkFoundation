#include "CkTagSet_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkTagSet/CkTagSet_Log.h"
#include "CkTagSet/CkTagSet_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_TagSet_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_TagSet& InTagSet,
            FFragment_TagSet_Requests& InRequestsComp) const
        -> void
    {
        auto TagsAdded   = FGameplayTagContainer{};
        auto TagsRemoved = FGameplayTagContainer{};

        InHandle.CopyAndRemove(InRequestsComp, [&](const FFragment_TagSet_Requests& InRequests)
        {
            ck::algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                DoHandleRequest(InHandle, InTagSet, InRequest, TagsAdded, TagsRemoved);
            }), ck::policy::DontResetContainer{});
        });

        if ([[maybe_unused]] const auto CollectionChanged = TagsAdded.Num() > 0 || TagsRemoved.Num() > 0)
        {
            UCk_Utils_TagSet_UE::Request_TryReplicateTagSet(InHandle);

            UUtils_Signal_TagSet_OnTagsChanged::Broadcast(
                InHandle,
                MakePayload(InHandle, TagsAdded, TagsRemoved));
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_TagSet_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            FFragment_TagSet& InTagSet,
            const FFragment_TagSet_Requests::AddTagsRequestType& InRequest,
            FGameplayTagContainer& OutTagsAdded,
            FGameplayTagContainer& /*OutTagsRemoved*/)
        -> void
    {
        const auto& RequestedTags = InRequest.Get_TagsToAdd();
        auto& CurrentTags = InTagSet._Tags;

        for (const auto& Tag : RequestedTags)
        {
            if (NOT CurrentTags.HasTagExact(Tag))
            {
                CurrentTags.AddTag(Tag);
                OutTagsAdded.AddTag(Tag);
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_TagSet_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            FFragment_TagSet& InTagSet,
            const FFragment_TagSet_Requests::RemoveTagsRequestType& InRequest,
            FGameplayTagContainer& /*OutTagsAdded*/,
            FGameplayTagContainer& OutTagsRemoved)
        -> void
    {
        const auto& RequestedTags = InRequest.Get_TagsToRemove();
        auto& CurrentTags = InTagSet._Tags;

        for (const auto& Tag : RequestedTags)
        {
            if (CurrentTags.HasTagExact(Tag))
            {
                CurrentTags.RemoveTag(Tag);
                OutTagsRemoved.AddTag(Tag);
            }
        }
    }

    // ---- Replicate (Server-side) ----

    auto
        FProcessor_TagSet_Replicate::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_TagSet_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_TagSet& InTagSet)
        -> void
    {
        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

        UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_TagSet>(
            LifetimeOwner, [&](FCk_RepData_TagSet& Data)
        {
            Data.Tags = InTagSet.Get_Tags();
        });
    }

    // ---- SyncReplication (Client-side) ----

    auto
        FProcessor_TagSet_SyncReplication::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_TagSet& InTagSet,
            const FFragment_TagSet_SyncReplication& InSync)
        -> void
    {
        const auto& ReplicatedTags = InSync.Get_ReplicatedTags();
        const auto& CurrentTags    = InTagSet.Get_Tags();

        // No change
        if (ReplicatedTags == CurrentTags)
        {
            InHandle.Remove<MarkedDirtyBy>();
            return;
        }

        // Compute diff
        const auto TagsAdded   = UCk_Utils_GameplayTag_UE::Get_Difference(ReplicatedTags, CurrentTags);
        const auto TagsRemoved = UCk_Utils_GameplayTag_UE::Get_Difference(CurrentTags, ReplicatedTags);

        // Apply replicated state
        InTagSet.Set_Tags(ReplicatedTags);

        // Broadcast signal if any tags changed
        if (TagsAdded.Num() > 0 || TagsRemoved.Num() > 0)
        {
            UUtils_Signal_TagSet_OnTagsChanged::Broadcast(
                InHandle,
                MakePayload(InHandle, TagsAdded, TagsRemoved));
        }

        InHandle.Remove<MarkedDirtyBy>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
