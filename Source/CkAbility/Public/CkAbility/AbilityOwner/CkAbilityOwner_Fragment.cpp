#include "CkAbilityOwner_Fragment.h"

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
    UCk_Fragment_AbilityOwner_Events_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisType, _Events);
}

auto
    UCk_Fragment_AbilityOwner_Events_Rep::
    OnRep_NewEvents()
    -> void
{
    if (ck::Is_NOT_Valid(Get_AssociatedEntity()))
    { return; }

    for (auto Index = _NextEventToProcess; Index < _Events.Num(); ++Index)
    {
        // Fire the event here
    }
}

// --------------------------------------------------------------------------------------------------------------------

