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
    Get_AllDirectChildTags(
        FGameplayTag InParentTag)
    -> FGameplayTagContainer
{
    auto DirectChildren = FGameplayTagContainer{};

    const auto& TagManager = UGameplayTagsManager::Get();

    const auto& ParentNode = TagManager.FindTagNode(InParentTag);

    if (ck::Is_NOT_Valid(ParentNode))
    { return DirectChildren; }

    for (const auto& ChildNodes = ParentNode->GetChildTagNodes();
         const auto& ChildNode : ChildNodes)
    {
        if (ChildNode.IsValid())
        {
            DirectChildren.AddTag(ChildNode->GetCompleteTag());
        }
    }

    return DirectChildren;
}

auto
    UCk_Utils_GameplayTag_UE::
    Get_AllDescendantTags(
        FGameplayTag InParentTag)
    -> FGameplayTagContainer
{
    auto AllDescendants = FGameplayTagContainer{};
    Get_AllDescendantTags_Recursive(InParentTag, AllDescendants);
    return AllDescendants;
}

auto
    UCk_Utils_GameplayTag_UE::
    Get_ParentTag(
        FGameplayTag InChildTag)
    -> FGameplayTag
{
    if (ck::Is_NOT_Valid(InChildTag))
    { return {}; }

    const auto& TagString = InChildTag.ToString();
    auto LastDotIndex = 0;

    if (NOT TagString.FindLastChar(TEXT('.'), LastDotIndex))
    { return {}; }

    const auto& ParentString = TagString.Left(LastDotIndex);
    return TryMake_LiteralGameplayTag_FromString(ParentString);
}

auto
    UCk_Utils_GameplayTag_UE::
    Get_TagDepth(
        FGameplayTag InTag)
    -> int32
{
    if (ck::Is_NOT_Valid(InTag))
    { return {}; }

    const auto& TagString = InTag.ToString();
    auto Depth = 1;

    for (auto i = 0; i < TagString.Len(); ++i)
    {
        if (TagString[i] == TEXT('.'))
        {
            ++Depth;
        }
    }

    return Depth;
}

auto
    UCk_Utils_GameplayTag_UE::
    Get_AllAncestorTags(
        FGameplayTag InTag)
    -> FGameplayTagContainer
{
    auto Ancestors = FGameplayTagContainer{};

    auto CurrentTag = Get_ParentTag(InTag);
    while (CurrentTag.IsValid())
    {
        Ancestors.AddTag(CurrentTag);
        CurrentTag = Get_ParentTag(CurrentTag);
    }

    return Ancestors;
}

auto
    UCk_Utils_GameplayTag_UE::
    Get_SiblingTags(
        FGameplayTag InTag)
    -> FGameplayTagContainer
{
    const auto& ParentTag = Get_ParentTag(InTag);

    if (ck::Is_NOT_Valid(ParentTag))
    {
        // This is a root tag, get all other root tags
        const auto& TagManager = UGameplayTagsManager::Get();
        auto AllTags = FGameplayTagContainer{};
        TagManager.RequestAllGameplayTags(AllTags, true);

        auto RootTags = FGameplayTagContainer{};
        for (const auto& Tag : AllTags)
        {
            if (Get_TagDepth(Tag) == 1 && Tag != InTag)
            {
                RootTags.AddTag(Tag);
            }
        }
        return RootTags;
    }

    auto Siblings = Get_AllDirectChildTags(ParentTag);
    Siblings.RemoveTag(InTag);
    return Siblings;
}

auto
    UCk_Utils_GameplayTag_UE::
    Get_TagsByDepth(
        const FGameplayTagContainer& InContainer,
        int32 InDepth)
    -> FGameplayTagContainer
{
    auto FilteredTags = FGameplayTagContainer{};

    for (const auto& Tag : InContainer)
    {
        if (Get_TagDepth(Tag) == InDepth)
        {
            FilteredTags.AddTag(Tag);
        }
    }

    return FilteredTags;
}

auto
    UCk_Utils_GameplayTag_UE::
    Get_TagsByRoot(
        const FGameplayTagContainer& InContainer,
        FGameplayTag InRootTag)
    -> FGameplayTagContainer
{
    auto FilteredTags = FGameplayTagContainer{};

    for (const auto& Tag : InContainer)
    {
        if (Get_Root_AsTag(Tag) == InRootTag)
        {
            FilteredTags.AddTag(Tag);
        }
    }

    return FilteredTags;
}

auto
    UCk_Utils_GameplayTag_UE::
    Get_LeafTagsOnly(
        const FGameplayTagContainer& InContainer)
    -> FGameplayTagContainer
{
    auto LeafTags = FGameplayTagContainer{};

    for (const auto& Tag : InContainer)
    {
        if (const auto& Children = Get_AllDirectChildTags(Tag);
            Children.IsEmpty())
        {
            LeafTags.AddTag(Tag);
        }
    }

    return LeafTags;
}

auto
    UCk_Utils_GameplayTag_UE::
    Get_RootTagsOnly(
        const FGameplayTagContainer& InContainer)
    -> FGameplayTagContainer
{
    auto RootTags = FGameplayTagContainer{};

    for (const auto& Tag : InContainer)
    {
        if (Get_TagDepth(Tag) == 1)
        {
            RootTags.AddTag(Tag);
        }
    }

    return RootTags;
}

auto
    UCk_Utils_GameplayTag_UE::
    Get_IsDirectChildOf(
        FGameplayTag InChildTag,
        FGameplayTag InParentTag)
    -> bool
{
    return Get_ParentTag(InChildTag) == InParentTag;
}

auto
    UCk_Utils_GameplayTag_UE::
    Get_AreTagsSiblings(
        FGameplayTag InTagA,
        FGameplayTag InTagB)
    -> bool
{
    if (InTagA == InTagB)
    { return false; } // Same tag, not siblings

    return Get_ParentTag(InTagA) == Get_ParentTag(InTagB);
}

auto
    UCk_Utils_GameplayTag_UE::
    Get_CommonAncestor(
        FGameplayTag InTagA,
        FGameplayTag InTagB)
    -> FGameplayTag
{
    const auto& AncestorsA = Get_AllAncestorTags(InTagA);
    const auto& AncestorsB = Get_AllAncestorTags(InTagB);

    // Find the deepest common ancestor
    auto CommonAncestor = FGameplayTag{};
    auto MaxDepth = 0;

    for (const auto& AncestorA : AncestorsA)
    {
        if (AncestorsB.HasTagExact(AncestorA))
        {
            const auto& Depth = Get_TagDepth(AncestorA);
            if (Depth > MaxDepth)
            {
                MaxDepth = Depth;
                CommonAncestor = AncestorA;
            }
        }
    }

    return CommonAncestor;
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
        TEXT("Failed to Make Literal Gameplay Tag from string [{}]. No such Gameplay Tag exists!"), InTagNameAsString)
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

auto
    UCk_Utils_GameplayTag_UE::
    Get_AllDescendantTags_Recursive(
        const FGameplayTag& InParentTag,
        FGameplayTagContainer& OutTags)
    -> void
{
    const auto& TagManager = UGameplayTagsManager::Get();
    const auto& ParentNode = TagManager.FindTagNode(InParentTag);

    if (ck::Is_NOT_Valid(ParentNode))
    { return; }

    for (const auto& ChildNodes = ParentNode->GetChildTagNodes();
         const auto& ChildNode : ChildNodes)
    {
        if (ChildNode.IsValid())
        {
            const auto& ChildTag = ChildNode->GetCompleteTag();
            OutTags.AddTag(ChildTag);

            Get_AllDescendantTags_Recursive(ChildTag, OutTags);
        }
    }
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