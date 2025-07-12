#include "CkGameplayTag_Utils.h"

#include "CkCore/CkCoreLog.h"
#include "CkCore/Ensure/CkEnsure.h"

#include <GameplayEffectTypes.h>
#include <GameplayTagsManager.h>

#if WITH_EDITOR
#include "GameplayTagsEditorModule.h"
#endif

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
    Get_DoContainersIntersect(
        const FGameplayTagContainer& A,
        FGameplayTagContainer B)
    -> bool
{
    return A.HasAny(B);
}

auto
    UCk_Utils_GameplayTag_UE::
    Get_DoContainersIntersect_Exact(
        const FGameplayTagContainer& A,
        FGameplayTagContainer B)
    -> bool
{
    return A.HasAnyExact(B);
}

auto
    UCk_Utils_GameplayTag_UE::
    Get_AllIntersectingTags(
        const FGameplayTagContainer& A,
        FGameplayTagContainer B)
    -> FGameplayTagContainer
{
    return A.Filter(B);
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
    Get_Leaf_AsName(
        FGameplayTag InGameplayTag)
    -> FName
{
    return FName{Get_Leaf_AsString(InGameplayTag)};
}

auto
    UCk_Utils_GameplayTag_UE::
    Get_Leaf_AsString(
        FGameplayTag InGameplayTag)
    -> FString
{
    static auto CachedTags = TMap<FGameplayTag, FString>{};

    if (const auto& FoundCachedResult = CachedTags.Find(InGameplayTag);
        ck::IsValid(FoundCachedResult, ck::IsValid_Policy_NullptrOnly{}))
    {
        return *FoundCachedResult;
    }

    const auto& TagString = InGameplayTag.GetTagName().ToString();

    auto OutIndex = 0;

    if (NOT TagString.FindLastChar(TEXT('.'), OutIndex))
    { return TagString; }

    const auto& Leaf = TagString.RightChop(OutIndex + 1);

    CachedTags.Add(InGameplayTag, Leaf);

    return Leaf;
}

auto
    UCk_Utils_GameplayTag_UE::
    Get_Leaf_AsText(
        FGameplayTag InGameplayTag)
    -> FText
{
    return FText::FromString(Get_Leaf_AsString(InGameplayTag));
}

auto
    UCk_Utils_GameplayTag_UE::
    Get_Root_AsName(
        FGameplayTag InGameplayTag)
    -> FName
{
    return FName{Get_Root_AsString(InGameplayTag)};
}

auto
    UCk_Utils_GameplayTag_UE::
    Get_Root_AsString(
        FGameplayTag InGameplayTag)
    -> FString
{
    static auto CachedTags = TMap<FGameplayTag, FString>{};

    if (const auto& FoundCachedResult = CachedTags.Find(InGameplayTag);
        ck::IsValid(FoundCachedResult, ck::IsValid_Policy_NullptrOnly{}))
    {
        return *FoundCachedResult;
    }

    const auto& TagString = InGameplayTag.GetTagName().ToString();

    auto OutIndex = 0;

    if (NOT TagString.FindChar(TEXT('.'), OutIndex))
    { return TagString; }

    const auto& Root = TagString.Left(OutIndex);

    CachedTags.Add(InGameplayTag, Root);

    return Root;
}

auto
    UCk_Utils_GameplayTag_UE::
    Get_Root_AsTag(
        FGameplayTag InGameplayTag)
    -> FGameplayTag
{
    return Make_LiteralGameplayTag_FromString(Get_Root_AsString(InGameplayTag));
}

auto
    UCk_Utils_GameplayTag_UE::
    Get_Root_AsText(
        FGameplayTag InGameplayTag)
    -> FText
{
    return FText::FromString(Get_Root_AsString(InGameplayTag));
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

auto
    UCk_Utils_GameplayTag_UE::
    ResolveGameplayTag(
        FName TagName,
        const FString& Comment)
    -> FGameplayTag
{
    CK_ENSURE_IF_NOT(ck::IsValid(TagName),
        TEXT("Cannot add Gameplay Tag [{}] to INI because the TagName is Invalid!"), TagName)
    { return {}; }

    const auto& Manager = UGameplayTagsManager::Get();

    if (Manager.RequestGameplayTag(TagName, false).IsValid())
    { return Manager.RequestGameplayTag(TagName); }

#if WITH_EDITOR

    auto& GameplayTagsEditorModule = FModuleManager::LoadModuleChecked<IGameplayTagsEditorModule>("GameplayTagsEditor");
    GameplayTagsEditorModule.AddNewGameplayTagToINI(TagName.ToString(), Comment);

    UGameplayTagsManager::Get().DoneAddingNativeTags();
    UGameplayTagsManager::Get().ConstructGameplayTagTree();

    CK_ENSURE_IF_NOT(Manager.RequestGameplayTag(TagName, false).IsValid(),
        TEXT("Failed to add Gameplay Tag [{}] to INI!"), TagName)
    { return {}; }

    return Manager.RequestGameplayTag(TagName);
#else
    CK_TRIGGER_ENSURE(TEXT("Cannot add Gameplay Tag [{}] to INI outside of the Editor!"));
    return {};
#endif
}


// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GameplayTagStack_UE::
    Request_AddStack(
        FCk_GameplayTagStackContainer& InStackContainer,
        const FCk_GameplayTagStack& InStack)
    -> void
{
    std::ignore = InStackContainer.AddStack(InStack);
}

auto
    UCk_Utils_GameplayTagStack_UE::
    Request_RemoveStack(
        FCk_GameplayTagStackContainer& InStackContainer,
        const FCk_GameplayTagStack& InStack)
 -> void
{
    std::ignore = InStackContainer.RemoveStack(InStack);
}

auto
    UCk_Utils_GameplayTagStack_UE::
    Get_ContainsTag(
        const FCk_GameplayTagStackContainer& InStackContainer,
        const FGameplayTag& InTag)
    -> bool
{
    return InStackContainer.Get_ContainsTag(InTag);
}

auto
    UCk_Utils_GameplayTagStack_UE::
    Get_StackCount(
        const FCk_GameplayTagStackContainer& InStackContainer,
        const FGameplayTag& InTag)
    -> int32
{
    return InStackContainer.Get_StackCount(InTag);
}

auto
    UCk_Utils_GameplayTagStack_UE::
    Make_GameplayTagStackContainer(
        const TArray<FCk_GameplayTagStack>& InStacks)
    -> FCk_GameplayTagStackContainer
{
    auto GameplayTagStackContainer = FCk_GameplayTagStackContainer{};

    for (const auto& Stack : InStacks)
    {
        GameplayTagStackContainer.AddStack(Stack);
    }

    return GameplayTagStackContainer;
}

auto
    UCk_Utils_GameplayTagStack_UE::
    Break_GameplayTagStackContainer(
        const FCk_GameplayTagStackContainer& InStackContainer)
    -> TArray<FCk_GameplayTagStack>
{
    return InStackContainer.Get_Stacks();
}

auto
    UCk_Utils_GameplayTagStack_UE::
    Break_GameplayTagStackContainer_AsMap(
        const FCk_GameplayTagStackContainer& InStackContainer)
    -> TMap<FGameplayTag, int32>
{
    return InStackContainer.Get_TagToCountMap();
}

// --------------------------------------------------------------------------------------------------------------------