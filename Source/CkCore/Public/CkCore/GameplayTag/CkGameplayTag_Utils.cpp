#include "CkGameplayTag_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/CkCoreLog.h"

#include <GameplayTagsManager.h>

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
    Make_LiteralGameplayTag_FromString(
        const FString& InTagNameAsString)
    -> FGameplayTag
{
    return Make_LiteralGameplayTag_FromName(*InTagNameAsString);
}

auto
    UCk_Utils_GameplayTag_UE::
    TryMake_LiteralGameplayTag_FromString(
        const FString& InTagNameAsString)
    -> FGameplayTag
{
    return TryMake_LiteralGameplayTag_FromName(*InTagNameAsString);
}

auto
    UCk_Utils_GameplayTag_UE::
    Make_LiteralGameplayTag_FromName(
        FName InTagNameAsString)
    -> FGameplayTag
{
    const auto& GameplayTag = TryMake_LiteralGameplayTag_FromName(InTagNameAsString);

    CK_ENSURE_IF_NOT(ck::IsValid(GameplayTag),
        TEXT("Faile to Make Literal Gameplay Tag from string [{}]. No such Gameplay Tag exists!"), InTagNameAsString)
    { return {}; }

    return GameplayTag;
}

auto
    UCk_Utils_GameplayTag_UE::
    TryMake_LiteralGameplayTag_FromName(
        FName InTagNameAsString)
    -> FGameplayTag
{
    constexpr auto ErrorIfNotFound = false;
    return UGameplayTagsManager::Get().RequestGameplayTag(InTagNameAsString, ErrorIfNotFound);
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
