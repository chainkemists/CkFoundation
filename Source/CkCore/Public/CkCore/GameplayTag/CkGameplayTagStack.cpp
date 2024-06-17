#include "CkGameplayTagStack.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/CkCoreLog.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_GameplayTagStackContainer::
    AddStack(
        const FGameplayTag& InTag,
        int32 InStackCount)
    -> FCk_GameplayTagStackContainer&
{
    if (InStackCount <= 0)
    { return *this; }

    CK_LOG_ERROR_IF_NOT(ck::core, ck::IsValid(InTag), TEXT("Invalid GameplayTag passed to FCk_GameplayTagStackContainer::AddStack"))
    { return *this; }

    if (const auto& FoundStackWithTag = _Stacks.FindByPredicate(ck::algo::MatchesGameplayTagStackNameExact{InTag});
        ck::IsValid(FoundStackWithTag, ck::IsValid_Policy_NullptrOnly{}))
    {
        auto& StackWithTag = *FoundStackWithTag;

        const auto& NewCount = StackWithTag.Get_StackCount() + InStackCount;
        StackWithTag._StackCount = NewCount;
        _TagToCountMap[InTag] = NewCount;

        MarkItemDirty(StackWithTag);

        return *this;
    }

    auto& NewStack = _Stacks.Emplace_GetRef(InTag, InStackCount);
    MarkItemDirty(NewStack);
    _TagToCountMap.Add(InTag, InStackCount);

    return *this;
}

auto
    FCk_GameplayTagStackContainer::
    AddStack(
        const FCk_GameplayTagStack& InStack)
    -> FCk_GameplayTagStackContainer&
{
    return AddStack(InStack.Get_Tag(), InStack.Get_StackCount());
}

auto
    FCk_GameplayTagStackContainer::
    RemoveStack(
        const FGameplayTag& InTag,
        int32 InStackCount)
    -> FCk_GameplayTagStackContainer&
{
    if (InStackCount <= 0)
    { return *this; }

    CK_LOG_ERROR_IF_NOT(ck::core, ck::IsValid(InTag), TEXT("Invalid GameplayTag passed to FCk_GameplayTagStackContainer::RemoveStack"))
    { return *this; }

    for (auto It = _Stacks.CreateIterator(); It; ++It)
    {
        auto& Stack = *It;

        if (Stack.Get_Tag() != InTag)
        { continue; }

        if (Stack.Get_StackCount() <= InStackCount)
        {
            It.RemoveCurrent();
            _TagToCountMap.Remove(InTag);
            MarkArrayDirty();
        }
        else
        {
            const auto& NewCount = Stack.Get_StackCount() - InStackCount;
            Stack._StackCount = NewCount;
            _TagToCountMap[InTag] = NewCount;
            MarkItemDirty(Stack);
        }

        return *this;
    }

    return *this;
}

auto
    FCk_GameplayTagStackContainer::
    RemoveStack(
        const FCk_GameplayTagStack& InStack)
    -> FCk_GameplayTagStackContainer&
{
    return RemoveStack(InStack.Get_Tag(), InStack.Get_StackCount());
}

auto
    FCk_GameplayTagStackContainer::
    Get_StackCount(
        const FGameplayTag& InTag) const
    -> int32
{
    return _TagToCountMap.FindRef(InTag);
}

auto
    FCk_GameplayTagStackContainer::
    Get_ContainsTag(
        const FGameplayTag& InTag) const
    -> bool
{
    return _TagToCountMap.Contains(InTag);
}

auto
    FCk_GameplayTagStackContainer::
    PreReplicatedRemove(
        const TArrayView<int32> InRemovedIndices,
        int32 InFinalSize)
    -> void
{
    for (const auto& Index : InRemovedIndices)
    {
        const auto& Tag = _Stacks[Index].Get_Tag();
        _TagToCountMap.Remove(Tag);
    }
}

auto
    FCk_GameplayTagStackContainer::
    PostReplicatedAdd(
        const TArrayView<int32> InAddedIndices,
        int32 InFinalSize)
    -> void
{
    for (const auto& Index : InAddedIndices)
    {
        const auto& Stack = _Stacks[Index];
        _TagToCountMap.Add(Stack.Get_Tag(), Stack.Get_StackCount());
    }
}

auto
    FCk_GameplayTagStackContainer::
    PostReplicatedChange(
        const TArrayView<int32> InChangedIndices,
        int32 InFinalSize)
    -> void
{
    for (const auto& Index : InChangedIndices)
    {
        const auto& Stack = _Stacks[Index];
        _TagToCountMap[Stack.Get_Tag()] = Stack.Get_StackCount();
    }
}

auto
    FCk_GameplayTagStackContainer::
    NetDeltaSerialize(
        FNetDeltaSerializeInfo& InDeltaParams)
    -> bool
{
    return FastArrayDeltaSerialize<GameplayTagStackType, ThisType>(_Stacks, InDeltaParams, *this);
}

// --------------------------------------------------------------------------------------------------------------------
