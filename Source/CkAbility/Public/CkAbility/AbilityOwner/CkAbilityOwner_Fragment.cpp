#include "CkAbilityOwner_Fragment.h"

#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"

#include "CkEntityExtension/CkEntityExtension_Utils.h"

#include <Engine/World.h>
#include <Net/UnrealNetwork.h>
#include <Net/Core/PushModel/PushModel.h>

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
        auto PreviousTags = _PreviousTags.GetExplicitGameplayTags();

        RecordOfEntityExtensions_Utils::ForEach_Entry(InAbilityOwner, [&](FCk_Handle_EntityExtension InEntityExtension)
        {
            if (const auto EntityExtensionAsAbilityOwnerHandle = UCk_Utils_AbilityOwner_UE::Cast(InEntityExtension);
                ck::IsValid(EntityExtensionAsAbilityOwnerHandle))
            {
                PreviousTags.AppendTags(UCk_Utils_AbilityOwner_UE::Get_PreviousTags(EntityExtensionAsAbilityOwnerHandle));
            }
        });

        return PreviousTags;
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
        Get_AreActiveTagsDifferentThanPreviousTags() const
        -> bool
    {
        return _ActiveTags_IncludingAllEntityExtensions != _PreviousTags_IncludingAllEntityExtensions;
    }

    auto
        FFragment_AbilityOwner_Current::
        Get_AreActiveTagsDifferentThanPreviousTags(
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
        _PreviousTags = _ActiveTags;
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

        if (Get_AreActiveTagsDifferentThanPreviousTags())
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

auto
    UCk_Fragment_AbilityOwner_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    constexpr auto Params = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _PendingTransferExistingAbilityRequests, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _PendingGiveAbilityRequests, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _PendingRevokeAbilityRequests, Params);
}

auto
    UCk_Fragment_AbilityOwner_Rep::
    Request_TryUpdateReplicatedFragment()
    -> void
{
    OnRep_PendingTransferExistingAbilityRequests();
    OnRep_PendingGiveAbilityRequests();
    OnRep_PendingRevokeAbilityRequests();
}

auto
    UCk_Fragment_AbilityOwner_Rep::
    OnRep_PendingTransferExistingAbilityRequests()
    -> void
{
    if (ck::Is_NOT_Valid(Get_AssociatedEntity()))
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    auto AssociatedEntityAbilityOwner = UCk_Utils_AbilityOwner_UE::Cast(_AssociatedEntity);

    // If associated entity ability owner is not yet valid or setup, we should not process replicated requests until setup calls Request_TryUpdateReplicatedFragment
    if (ck::Is_NOT_Valid(AssociatedEntityAbilityOwner) ||
        AssociatedEntityAbilityOwner.Has<ck::FTag_AbilityOwner_NeedsSetup>())
    { return; }

    for (auto Index = _NextPendingTransferExistingAbilityRequests; Index < _PendingTransferExistingAbilityRequests.Num(); ++Index)
    {
        const auto& TransferRequest = _PendingTransferExistingAbilityRequests[Index];

        if (ck::Is_NOT_Valid(TransferRequest.Get_TransferTarget()) || ck::Is_NOT_Valid(TransferRequest.Get_Ability()))
        {
            _NextPendingRevokeAbilityRequests = Index;
            return;
        }

        UCk_Utils_AbilityOwner_UE::Request_TransferExistingAbility(AssociatedEntityAbilityOwner, TransferRequest, {});
    }
    _NextPendingTransferExistingAbilityRequests = _PendingTransferExistingAbilityRequests.Num();
}

auto
    UCk_Fragment_AbilityOwner_Rep::
    OnRep_PendingGiveAbilityRequests()
    -> void
{
    if (ck::Is_NOT_Valid(Get_AssociatedEntity()))
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    auto AssociatedEntityAbilityOwner = UCk_Utils_AbilityOwner_UE::Cast(_AssociatedEntity);

    // If associated entity ability owner is not yet valid or setup, we should not process replicated requests until setup calls Request_TryUpdateReplicatedFragment
    if (ck::Is_NOT_Valid(AssociatedEntityAbilityOwner) ||
        AssociatedEntityAbilityOwner.Has<ck::FTag_AbilityOwner_NeedsSetup>())
    { return; }

    for (auto Index = _NextPendingGiveAbilityRequests; Index < _PendingGiveAbilityRequests.Num(); ++Index)
    {
        const auto& GiveAbilityRequest = _PendingGiveAbilityRequests[Index];
        UCk_Utils_AbilityOwner_UE::Request_GiveAbility(AssociatedEntityAbilityOwner, GiveAbilityRequest, {});
    }
    _NextPendingGiveAbilityRequests = _PendingGiveAbilityRequests.Num();
}

auto
    UCk_Fragment_AbilityOwner_Rep::
    OnRep_PendingRevokeAbilityRequests()
    -> void
{
    if (ck::Is_NOT_Valid(Get_AssociatedEntity()))
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    auto AssociatedEntityAbilityOwner = UCk_Utils_AbilityOwner_UE::Cast(_AssociatedEntity);

    // If associated entity ability owner is not yet valid or setup, we should not process replicated requests until setup calls Request_TryUpdateReplicatedFragment
    if (ck::Is_NOT_Valid(AssociatedEntityAbilityOwner) ||
        AssociatedEntityAbilityOwner.Has<ck::FTag_AbilityOwner_NeedsSetup>())
    { return; }

    for (auto Index = _NextPendingRevokeAbilityRequests; Index < _PendingRevokeAbilityRequests.Num(); ++Index)
    {
        const auto& RevokeAbilityRequest = _PendingRevokeAbilityRequests[Index];

        if (RevokeAbilityRequest.Get_SearchPolicy() == ECk_AbilityOwner_AbilitySearch_Policy::SearchByHandle &&
            ck::Is_NOT_Valid(UCk_Utils_Ability_UE::Cast(RevokeAbilityRequest.Get_AbilityHandle())))
        {
            _NextPendingRevokeAbilityRequests = Index;
            return;
        }

        UCk_Utils_AbilityOwner_UE::Request_RevokeAbility(AssociatedEntityAbilityOwner, RevokeAbilityRequest, {});
    }
    _NextPendingRevokeAbilityRequests = _PendingRevokeAbilityRequests.Num();
}

auto
    UCk_Fragment_AbilityOwner_Rep::
    Request_TransferExistingAbility(
        const FCk_Request_AbilityOwner_TransferExistingAbility& InRequest)
    -> void
{
    _PendingTransferExistingAbilityRequests.Emplace(InRequest);
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _PendingTransferExistingAbilityRequests, this);
}

auto
    UCk_Fragment_AbilityOwner_Rep::
    Request_GiveAbility(
        const FCk_Request_AbilityOwner_GiveAbility& InRequest)
    -> void
{
    _PendingGiveAbilityRequests.Emplace(InRequest);
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _PendingGiveAbilityRequests, this);
}

auto
    UCk_Fragment_AbilityOwner_Rep::
    Request_RevokeAbility(
        const FCk_Request_AbilityOwner_RevokeAbility& InRequest)
    -> void
{
    _PendingRevokeAbilityRequests.Emplace(InRequest);
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _PendingRevokeAbilityRequests, this);
}

// --------------------------------------------------------------------------------------------------------------------
