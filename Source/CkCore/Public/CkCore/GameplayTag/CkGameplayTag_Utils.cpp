#include "CkGameplayTag_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GameplayTag_UE::
    Get_AreTagRequirementsMet(
        const FGameplayTagRequirements& InTagRequirements,
        const FGameplayTagContainer& InTagsToTest)
    -> bool
{
    return InTagRequirements.RequirementsMet(InTagsToTest);
}

auto
    UCk_Utils_GameplayTag_UE::
    Get_TagsWithCount_FromContainer(
        const FGameplayTagCountContainer& InTagContainer)
    -> TMap<FGameplayTag, int32>
{
    TMap<FGameplayTag, int32> Ret;

    for (const auto& ActiveTag : InTagContainer.GetExplicitGameplayTags())
    {
        Ret.Add(ActiveTag, InTagContainer.GetTagCount(ActiveTag));
    }

    return Ret;
}

// --------------------------------------------------------------------------------------------------------------------
