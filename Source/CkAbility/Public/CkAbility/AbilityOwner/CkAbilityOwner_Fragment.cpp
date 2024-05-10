#include "CkAbilityOwner_Fragment.h"

#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"

#include "CkEntityExtension/CkEntityExtension_Utils.h"

#include <Net/UnrealNetwork.h>
#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FFragment_AbilityOwner_Current::
        Get_ActiveTags(
            const FCk_Handle_AbilityOwner& InAbilityOwner) const
        -> FGameplayTagContainer
    {
        auto ActiveTags = _ActiveTags.GetExplicitGameplayTags();

        RecordOfEntityExtensions_Utils::ForEach_Entry(InAbilityOwner, [&](FCk_Handle_EntityExtension InEntityExtension)
        {
            if (const auto EntityExtensionAsAbilityOwnerHandle = UCk_Utils_AbilityOwner_UE::Cast(InEntityExtension);
                ck::IsValid(EntityExtensionAsAbilityOwnerHandle))
            {
                ActiveTags.AppendTags(UCk_Utils_AbilityOwner_UE::Get_ActiveTags(EntityExtensionAsAbilityOwnerHandle));
            }
        });

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
        auto Count = _ActiveTags.GetTagCount(InTag);

        RecordOfEntityExtensions_Utils::ForEach_Entry(InAbilityOwner, [&](FCk_Handle_EntityExtension InEntityExtension)
        {
            if (const auto EntityExtensionAsAbilityOwnerHandle = UCk_Utils_AbilityOwner_UE::Cast(InEntityExtension);
                ck::IsValid(EntityExtensionAsAbilityOwnerHandle))
            {
                Count += UCk_Utils_AbilityOwner_UE::Get_SpecificActiveTagCount(EntityExtensionAsAbilityOwnerHandle, InTag);
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
            const FCk_Handle_AbilityOwner& InAbilityOwner,
            const FGameplayTagContainer& InTagsToAdd)
        -> void
    {
        if (InTagsToAdd.IsEmpty())
        { return; }

        DoTry_TagsUpdatedOnExtensionOwner(InAbilityOwner);
        _PreviousTags = _ActiveTags;
        _ActiveTags.UpdateTagCount(InTagsToAdd, 1);

        _PreviousTags_IncludingAllEntityExtensions = _ActiveTags_IncludingAllEntityExtensions;
        _ActiveTags_IncludingAllEntityExtensions = Get_ActiveTags(InAbilityOwner);
    }

    auto
        FFragment_AbilityOwner_Current::
        AddTag(
            const FCk_Handle_AbilityOwner& InAbilityOwner,
            const FGameplayTag& InTagToAdd)
        -> void
    {
        DoTry_TagsUpdatedOnExtensionOwner(InAbilityOwner);
        _PreviousTags = _ActiveTags;
        _ActiveTags.UpdateTagCount(InTagToAdd, 1);

        _PreviousTags_IncludingAllEntityExtensions = _ActiveTags_IncludingAllEntityExtensions;
        _ActiveTags_IncludingAllEntityExtensions = Get_ActiveTags(InAbilityOwner);
    }

    auto
        FFragment_AbilityOwner_Current::
        RemoveTag(
            const FCk_Handle_AbilityOwner& InAbilityOwner,
            const FGameplayTag& InTagToRemove)
        -> void
    {
        DoTry_TagsUpdatedOnExtensionOwner(InAbilityOwner);
        _PreviousTags = _ActiveTags;
        _ActiveTags.UpdateTagCount(InTagToRemove, -1);

        _PreviousTags_IncludingAllEntityExtensions = _ActiveTags_IncludingAllEntityExtensions;
        _ActiveTags_IncludingAllEntityExtensions = Get_ActiveTags(InAbilityOwner);
    }

    auto
        FFragment_AbilityOwner_Current::
        RemoveTags(
            const FCk_Handle_AbilityOwner& InAbilityOwner,
            const FGameplayTagContainer& InTagsToRemove)
        -> void
    {
        if (InTagsToRemove.IsEmpty())
        { return; }

        DoTry_TagsUpdatedOnExtensionOwner(InAbilityOwner);
        _PreviousTags = _ActiveTags;
        _ActiveTags.UpdateTagCount(InTagsToRemove, -1);

        _PreviousTags_IncludingAllEntityExtensions = _ActiveTags_IncludingAllEntityExtensions;
        _ActiveTags_IncludingAllEntityExtensions = Get_ActiveTags(InAbilityOwner);
    }

    auto
        FFragment_AbilityOwner_Current::
        DoTry_TagsUpdatedOnExtensionOwner(
            const FCk_Handle_AbilityOwner& InAbilityOwner)
        -> void
    {
        if (const auto EntityExtension = UCk_Utils_EntityExtension_UE::Cast(InAbilityOwner);
            ck::IsValid(EntityExtension, ck::IsValid_Policy_IncludePendingKill{}))
        {
            auto ExtensionOwner = UCk_Utils_EntityExtension_UE::Get_ExtensionOwner(EntityExtension);
            auto ExtensionOwnerAsAbilityOwner = UCk_Utils_AbilityOwner_UE::Cast(ExtensionOwner);

            if (ck::IsValid(ExtensionOwner))
            {
                UCk_Utils_AbilityOwner_UE::Request_TagsUpdated(ExtensionOwnerAsAbilityOwner);
            }
        }
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

    DOREPLIFETIME(ThisType, _PendingGiveAbilityRequests);
    DOREPLIFETIME(ThisType, _PendingRevokeAbilityRequests);
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

    auto AssociatedEntityAbilityOwner = ck::StaticCast<FCk_Handle_AbilityOwner>(_AssociatedEntity);

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

    auto AssociatedEntityAbilityOwner = ck::StaticCast<FCk_Handle_AbilityOwner>(_AssociatedEntity);

    for (auto Index = _NextPendingRevokeAbilityRequests; Index < _PendingRevokeAbilityRequests.Num(); ++Index)
    {
        const auto& RevokeAbilityRequest = _PendingRevokeAbilityRequests[Index];
        UCk_Utils_AbilityOwner_UE::Request_RevokeAbility(AssociatedEntityAbilityOwner, RevokeAbilityRequest, {});
    }
    _NextPendingRevokeAbilityRequests = _PendingRevokeAbilityRequests.Num();
}

// --------------------------------------------------------------------------------------------------------------------

