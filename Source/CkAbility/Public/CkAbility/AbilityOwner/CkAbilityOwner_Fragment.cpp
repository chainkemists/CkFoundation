#include "CkAbilityOwner_Fragment.h"

#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"

#include <Net/UnrealNetwork.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FFragment_AbilityOwner_Current::
        Get_ActiveTags() const
        -> FGameplayTagContainer
    {
        return _ActiveTags.GetExplicitGameplayTags();
    }

    auto
        FFragment_AbilityOwner_Current::
        Get_ActiveTagsWithCount() const
        -> TMap<FGameplayTag, int32>
    {
        TArray<FGameplayTag> OutActiveTagArray;
        Get_ActiveTags().GetGameplayTagArray(OutActiveTagArray);

        TMap<FGameplayTag, int32> Ret;

        for (const auto& ActiveTag : OutActiveTagArray)
        {
            Ret.Add(ActiveTag, Get_SpecificActiveTagCount(ActiveTag));
        }

        return Ret;
    }

    auto
        FFragment_AbilityOwner_Current::
        Get_SpecificActiveTagCount(
            const FGameplayTag& InTag) const
        -> int32
    {
        return _ActiveTags.GetTagCount(InTag);
    }

    auto
        FFragment_AbilityOwner_Current::
        AppendTags(
            const FGameplayTagContainer& InTagsToAdd)
        -> void
    {
        _ActiveTags.UpdateTagCount(InTagsToAdd, 1);
    }

    auto
        FFragment_AbilityOwner_Current::
        AddTag(
            const FGameplayTag& InTagToAdd)
        -> void
    {
        _ActiveTags.UpdateTagCount(InTagToAdd, 1);
    }

    auto
        FFragment_AbilityOwner_Current::
        RemoveTag(
            const FGameplayTag& InTagToRemove)
        -> void
    {
        _ActiveTags.UpdateTagCount(InTagToRemove, -1);
    }

    auto
        FFragment_AbilityOwner_Current::
        RemoveTags(
            const FGameplayTagContainer& InTagsToRemove)
        -> void
    {
        _ActiveTags.UpdateTagCount(InTagsToRemove, -1);
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
        UCk_Utils_AbilityOwner_UE::Request_GiveAbility(AssociatedEntityAbilityOwner, GiveAbilityRequest);
    }
    _NextPendingGiveAbilityRequests = _PendingGiveAbilityRequests.Num();
}

// --------------------------------------------------------------------------------------------------------------------

