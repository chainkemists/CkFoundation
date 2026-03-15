#include "CkAbilityOwner_Fragment.h"

#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"

#include "CkEntityExtension/CkEntityExtension_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FFragment_AbilityOwner_Current::
        Get_ActiveTags(
            const FCk_Handle_AbilityOwner& InAbilityOwner) const
        -> FGameplayTagContainer
    {
        QUICK_SCOPE_CYCLE_COUNTER(Get_ActiveTags)
        auto ActiveTags = _ActiveTags.GetExplicitGameplayTags();

        ActiveTags.AppendTags(_CachedActiveExtensionTags);

        return ActiveTags;
    }

    auto
        FFragment_AbilityOwner_Current::
        Get_PreviousTags(
            const FCk_Handle_AbilityOwner& InAbilityOwner) const
        -> FGameplayTagContainer
    {
        return _PreviousTags_IncludingAllEntityExtensions;
    }

    auto
        FFragment_AbilityOwner_Current::
        Get_ActiveTagsWithCount(
            const FCk_Handle_AbilityOwner& InAbilityOwner) const
        -> TMap<FGameplayTag, int32>
    {
        auto ActiveTagArray = TArray<FGameplayTag>{};
        Get_ActiveTags(InAbilityOwner).GetGameplayTagArray(ActiveTagArray);

        TMap<FGameplayTag, int32> Ret;

        for (const auto& ActiveTag : ActiveTagArray)
        {
            Ret.Add(ActiveTag, Get_SpecificActiveTagCount(InAbilityOwner, ActiveTag));
        }

        return Ret;
    }

    auto
        FFragment_AbilityOwner_Current::
        Get_SpecificActiveTagCount(
            const FCk_Handle_AbilityOwner& InAbilityOwner,
            const FGameplayTag& InTag) const
        -> int32
    {
        QUICK_SCOPE_CYCLE_COUNTER(Get_ActiveTagCountRecursive)
        auto Count = _ActiveTags.GetTagCount(InTag);

        // This cannot use the cached tags since we need the count and we only cache if the tags exist since there is no fast way to combine gameplay tag count containers
        // This ForEach_Entry will recurse over all extensions, even if they are not directly owned by this ability owner
        RecordOfEntityExtensions_Utils::ForEach_Entry(InAbilityOwner, [&](FCk_Handle_EntityExtension InEntityExtension)
        {
            if (const auto EntityExtensionAsAbilityOwnerHandle = UCk_Utils_AbilityOwner_UE::Cast(InEntityExtension);
                ck::IsValid(EntityExtensionAsAbilityOwnerHandle) &&
                InEntityExtension.Has<ck::FFragment_AbilityOwner_Current>())
            {
                const auto EntityExtensionAbilityOwnerFragment = InEntityExtension.Get<ck::FFragment_AbilityOwner_Current>();
                Count += EntityExtensionAbilityOwnerFragment.Get_SpecificActiveTagCountForExtension(InTag);
            }
        });

        return Count;
    }

    auto
        FFragment_AbilityOwner_Current::
        Get_AreActiveTagsDifferentFromPreviousTags() const
        -> bool
    {
        return _ActiveTags_IncludingAllEntityExtensions != _PreviousTags_IncludingAllEntityExtensions;
    }

    auto
        FFragment_AbilityOwner_Current::
        Get_AreActiveTagsDifferentFromPreviousTags(
            const FCk_Handle_AbilityOwner& InAbilityOwner) const
        -> bool
    {
        return Get_ActiveTags(InAbilityOwner) != _PreviousTags_IncludingAllEntityExtensions;
    }

    auto
        FFragment_AbilityOwner_Current::
        AppendTags(
            FCk_Handle_AbilityOwner& InAbilityOwner,
            const FGameplayTagContainer& InTagsToAdd)
        -> void
    {
        if (InTagsToAdd.IsEmpty())
        { return; }

        _ActiveTags.UpdateTagCount(InTagsToAdd, 1);

        Do_TagsUpdated(InAbilityOwner);
    }

    auto
        FFragment_AbilityOwner_Current::
        AddTag(
            FCk_Handle_AbilityOwner& InAbilityOwner,
            const FGameplayTag& InTagToAdd)
        -> void
    {
        _ActiveTags.UpdateTagCount(InTagToAdd, 1);

        Do_TagsUpdated(InAbilityOwner);
    }

    auto
        FFragment_AbilityOwner_Current::
        RemoveTag(
            FCk_Handle_AbilityOwner& InAbilityOwner,
            const FGameplayTag& InTagToRemove)
        -> void
    {
        _ActiveTags.UpdateTagCount(InTagToRemove, -1);

        Do_TagsUpdated(InAbilityOwner);
    }

    auto
        FFragment_AbilityOwner_Current::
        RemoveTags(
            FCk_Handle_AbilityOwner& InAbilityOwner,
            const FGameplayTagContainer& InTagsToRemove)
        -> void
    {
        if (InTagsToRemove.IsEmpty())
        { return; }

        _ActiveTags.UpdateTagCount(InTagsToRemove, -1);

        Do_TagsUpdated(InAbilityOwner);
    }

    auto
        FFragment_AbilityOwner_Current::
        UpdatePreviousTags()
        -> void
    {
        _PreviousTags_IncludingAllEntityExtensions = _ActiveTags_IncludingAllEntityExtensions;
    }

    auto
        FFragment_AbilityOwner_Current::
        Do_TagsUpdated(
            FCk_Handle_AbilityOwner& InAbilityOwner)
        -> void
    {
        QUICK_SCOPE_CYCLE_COUNTER(TagsUpdated)
        _ActiveTags_IncludingAllEntityExtensions = Get_ActiveTags(InAbilityOwner);

        if (Get_AreActiveTagsDifferentFromPreviousTags())
        {
            UCk_Utils_AbilityOwner_UE::Request_TagsUpdated(InAbilityOwner);
        }
        DoTry_TagsUpdatedOnExtensionOwner(InAbilityOwner);
    }

    auto
        FFragment_AbilityOwner_Current::
        Do_CacheExtensionTags(
            FCk_Handle_AbilityOwner& InAbilityOwner)
        -> void
    {
        QUICK_SCOPE_CYCLE_COUNTER(Do_CacheExtensionTags)
        _CachedActiveExtensionTags.Reset();

        RecordOfEntityExtensions_Utils::ForEach_Entry(InAbilityOwner, [&](FCk_Handle_EntityExtension InEntityExtension)
        {
            // This ForEach_Entry will loop over all extensions even if they are not directly owned by this ability owner,
            // but we can assume that the direct children extensions already have accurate caches so we only need to iterate
            // over those children and not all children
            if (UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InEntityExtension) != InAbilityOwner)
            { return; }

            if (const auto EntityExtensionAsAbilityOwnerHandle = UCk_Utils_AbilityOwner_UE::Cast(InEntityExtension);
                ck::IsValid(EntityExtensionAsAbilityOwnerHandle) &&
                InEntityExtension.Has<ck::FFragment_AbilityOwner_Current>())
            {
                const auto EntityExtensionAbilityOwnerFragment = InEntityExtension.Get<ck::FFragment_AbilityOwner_Current>();
                _CachedActiveExtensionTags.AppendTags(EntityExtensionAbilityOwnerFragment.Get_ActiveTagsForExtensionCache());
            }
        });
    }

    auto
        FFragment_AbilityOwner_Current::
        DoTry_TagsUpdatedOnExtensionOwner(
            FCk_Handle_AbilityOwner& InAbilityOwner)
        -> void
    {
        if (const auto EntityExtension = UCk_Utils_EntityExtension_UE::Cast(InAbilityOwner);
            ck::IsValid(EntityExtension, ck::IsValid_Policy_IncludePendingKill{}))
        {
            auto ExtensionOwner = UCk_Utils_EntityExtension_UE::Get_ExtensionOwner(EntityExtension);
            auto ExtensionOwnerAsAbilityOwner = UCk_Utils_AbilityOwner_UE::Cast(ExtensionOwner);

            if (ck::IsValid(ExtensionOwner))
            {
                auto& ExtensionOwnerAbilityOwnerComp = ExtensionOwnerAsAbilityOwner.Get<FFragment_AbilityOwner_Current>();
                ExtensionOwnerAbilityOwnerComp.Do_CacheExtensionTags(ExtensionOwnerAsAbilityOwner);
                ExtensionOwnerAbilityOwnerComp.Do_TagsUpdated(ExtensionOwnerAsAbilityOwner);
            }
        }
    }

    auto
        FFragment_AbilityOwner_Current::
        Get_ActiveTagsForExtensionCache() const
        -> FGameplayTagContainer
    {
        QUICK_SCOPE_CYCLE_COUNTER(Get_ActiveTagsForExtensionCache)
        auto ActiveTags = _ActiveTags.GetExplicitGameplayTags();

        for (const auto& Tag : _RelevantTagsFromAbilityOwner)
        {
            if (_ActiveTags.GetTagCount(Tag) == 1)
            {
                ActiveTags.RemoveTag(Tag);
            }
        }

        ActiveTags.AppendTags(_CachedActiveExtensionTags);

        return ActiveTags;
    }

    auto
        FFragment_AbilityOwner_Current::
        Get_SpecificActiveTagCountForExtension(
            const FGameplayTag& InTag) const
        -> int32
    {
        QUICK_SCOPE_CYCLE_COUNTER(Get_SpecificActiveTagCountForExtension)
        auto Count = _ActiveTags.GetTagCount(InTag);

        for (const auto& RelevantTag : _RelevantTagsFromAbilityOwner)
        {
            if (InTag.MatchesTag(RelevantTag))
            {
                Count--;
            }
        }
        return Count;
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for AbilityOwner

static struct FAbilityOwnerRepHandlerRegistrar
{
    FAbilityOwnerRepHandlerRegistrar()
    {
        const auto DoApplyRequests = [](FCk_Handle& Entity, const FCk_RepData_AbilityOwnerRequests& NewData)
        {
            auto AbilityOwner = UCk_Utils_AbilityOwner_UE::Cast(Entity);

            if (ck::Is_NOT_Valid(AbilityOwner) ||
                AbilityOwner.Has<ck::FTag_AbilityOwner_NeedsSetup>())
            { return; }

            auto& Progress = Entity.AddOrGet<ck::FFragment_AbilityOwner_ReplicationProgress>();

            // Process transfer requests
            for (auto Index = Progress.NextTransferIndex; Index < NewData.TransferExistingAbilityRequests.Num(); ++Index)
            {
                const auto& TransferRequest = NewData.TransferExistingAbilityRequests[Index];

                if (ck::Is_NOT_Valid(TransferRequest.Get_TransferTarget()) || ck::Is_NOT_Valid(TransferRequest.Get_Ability()))
                {
                    Progress.NextTransferIndex = Index;
                    return;
                }

                UCk_Utils_AbilityOwner_UE::Request_TransferExistingAbility_DeferUntilReadyOnClient(AbilityOwner, TransferRequest);
            }
            Progress.NextTransferIndex = NewData.TransferExistingAbilityRequests.Num();

            // Process give requests
            for (auto Index = Progress.NextGiveIndex; Index < NewData.GiveAbilityRequests.Num(); ++Index)
            {
                const auto& GiveAbilityRequest = NewData.GiveAbilityRequests[Index];
                UCk_Utils_AbilityOwner_UE::Request_GiveAbility(AbilityOwner, GiveAbilityRequest, {});
            }
            Progress.NextGiveIndex = NewData.GiveAbilityRequests.Num();

            // Process revoke requests
            for (auto Index = Progress.NextRevokeIndex; Index < NewData.RevokeAbilityRequests.Num(); ++Index)
            {
                const auto& RevokeAbilityRequest = NewData.RevokeAbilityRequests[Index];

                if (RevokeAbilityRequest.Get_SearchPolicy() == ECk_AbilityOwner_AbilitySearch_Policy::SearchByHandle &&
                    ck::Is_NOT_Valid(UCk_Utils_Ability_UE::Cast(RevokeAbilityRequest.Get_AbilityHandle())))
                {
                    Progress.NextRevokeIndex = Index;
                    return;
                }

                switch (RevokeAbilityRequest.Get_SearchPolicy())
                {
                    case ECk_AbilityOwner_AbilitySearch_Policy::SearchByClass:
                    {
                        UCk_Utils_AbilityOwner_UE::Request_RevokeAbility(AbilityOwner, RevokeAbilityRequest, {});
                        break;
                    }
                    case ECk_AbilityOwner_AbilitySearch_Policy::SearchByHandle:
                    {
                        UCk_Utils_AbilityOwner_UE::Request_RevokeAbility_DeferUntilReadyOnClient(AbilityOwner, RevokeAbilityRequest);
                        break;
                    }
                }
            }
            Progress.NextRevokeIndex = NewData.RevokeAbilityRequests.Num();
        };

        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_AbilityOwnerRequests::StaticStruct(); },
            {
                .OnChange = [DoApplyRequests](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct&)
                {
                    DoApplyRequests(Entity, New.Get<FCk_RepData_AbilityOwnerRequests>());
                },
                .OnAdd = [DoApplyRequests](FCk_Handle& Entity, const FInstancedStruct& Data)
                {
                    DoApplyRequests(Entity, Data.Get<FCk_RepData_AbilityOwnerRequests>());
                }
            });
    }
} GAbilityOwnerRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
