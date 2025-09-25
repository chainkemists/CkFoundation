#include "CkEntityTag_Utils.h"

#include "CkEntityTag/CkEntityTag_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityTag_UE::
    Add(
        FCk_Handle& InHandle,
        FName InTag)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Unable to add EntityTag [{}] to Handle [{}] that is INVALID"), InTag, InHandle)
    { return {}; }

    auto& Current = InHandle.AddOrGet<ck::FFragment_EntityTag_Current>();

    const auto& TagIndex = ck::algo::FindIndex(Current.Get_Tags(), [InTag](const ck::FEntityTagCount& TagCountPair)
    {
        return TagCountPair._Name == InTag;
    });

    const auto& AddNew = TagIndex == INDEX_NONE;
    if (AddNew)
    {
        Current.Get_Tags().Emplace(InTag, 1);
    }
    else
    {
        Current.Get_Tags()[TagIndex]._Count++;
    }

    const auto Entity = InHandle.Get_Entity().Get_ID();
    auto&& Storage = InHandle->Storage<ck::FFragment_EntityTag_StorageParams>(entt::id_type{GetTypeHash(InTag)});

    if(NOT Storage.contains(Entity))
    {
        auto StorageParams = ck::FFragment_EntityTag_StorageParams{InTag};
        Storage.emplace<ck::FFragment_EntityTag_StorageParams>(InHandle.Get_Entity().Get_ID(), std::move(StorageParams));
    }

    if (AddNew)
    {
        ck::UUtils_Signal_EntityTag_OnTagUpdated::Broadcast(InHandle, ck::MakePayload(InHandle, InTag, ECk_EntityTagUpdate::Added));
    }

    return InHandle;
}

auto
    UCk_Utils_EntityTag_UE::
    Add_UsingGameplayTag(
        FCk_Handle& InHandle,
        FGameplayTag InTag)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Unable to add EntityTag [{}] to Handle [{}] that is INVALID"), InTag, InHandle)
    { return {}; }

    auto& Current = InHandle.AddOrGet<ck::FFragment_EntityTag_Current>();

    Current.Get_GameplayTags().UpdateTagCount(InTag, 1);
    for (const auto& Tag : InTag.GetGameplayTagParents())
    {
        const auto& HadPrev = Has(InHandle, Tag.GetTagName());
        Add(InHandle, Tag.GetTagName());
        if (HadPrev != Has(InHandle, Tag.GetTagName()))
        {
            ck::UUtils_Signal_EntityTag_OnGameplayTagUpdated::Broadcast(InHandle, ck::MakePayload(InHandle, Tag, ECk_EntityTagUpdate::Added));
        }
    }

    return InHandle;
}

auto
    UCk_Utils_EntityTag_UE::
    Has(
        const FCk_Handle& InHandle,
        FName InTag)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Unable to find EntityTag [{}] on Handle [{}] that is INVALID"), InTag, InHandle)
    { return {}; }

    if (NOT InHandle.Has<ck::FFragment_EntityTag_Current>())
    { return false; }

    auto& Current = InHandle.Get<ck::FFragment_EntityTag_Current>();

    return ck::algo::AnyOf(Current.Get_Tags(), [InTag](const ck::FEntityTagCount& TagCountPair)
    {
        return TagCountPair._Name == InTag;
    });
}

auto
    UCk_Utils_EntityTag_UE::
    Has_UsingGameplayTag(
        const FCk_Handle& InHandle,
        FGameplayTag InTag)
    -> bool
{
    return Has(InHandle, InTag.GetTagName());
}

auto
    UCk_Utils_EntityTag_UE::
    Get_AllTagsAsContainer(
        const FCk_Handle& InHandle)
    -> FGameplayTagContainer
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Unable to get entity tags on Handle [{}] that is INVALID"), InHandle)
    { return {}; }

    if (NOT InHandle.Has<ck::FFragment_EntityTag_Current>())
    { return {}; }

    auto& Current = InHandle.Get<ck::FFragment_EntityTag_Current>();

    return Current.Get_GameplayTags().GetExplicitGameplayTags();
}

auto
    UCk_Utils_EntityTag_UE::
    Get_AllTags(
        const FCk_Handle& InHandle)
    -> TArray<FName>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Unable to get entity tags on Handle [{}] that is INVALID"), InHandle)
    { return {}; }

    if (NOT InHandle.Has<ck::FFragment_EntityTag_Current>())
    { return {}; }

    auto& Current = InHandle.Get<ck::FFragment_EntityTag_Current>();

    return ck::algo::Transform<TArray<FName>>(Current.Get_Tags(), [](const ck::FEntityTagCount& TagCountPair)
    {
        return TagCountPair._Name;
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityTag_UE::
    Request_TryRemove(
        FCk_Handle& InHandle,
        FName InTag)
    -> ECk_SucceededFailed
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid Handle passed. Unable to remove Tag [{}] from Entity"), InTag)
    { return ECk_SucceededFailed::Failed; }

    if (NOT InHandle.Has<ck::FFragment_EntityTag_Current>())
    { return ECk_SucceededFailed::Failed; }

    auto& Current = InHandle.AddOrGet<ck::FFragment_EntityTag_Current>();

    const auto& TagIndex = ck::algo::FindIndex(Current.Get_Tags(), [InTag](const ck::FEntityTagCount& TagCountPair)
    {
        return TagCountPair._Name == InTag;
    });

    if (TagIndex == INDEX_NONE)
    { return ECk_SucceededFailed::Failed; }

    Current.Get_Tags()[TagIndex]._Count--;

    const auto& WasRemoved = Current.Get_Tags()[TagIndex]._Count <= 0;

    if (WasRemoved)
    {
        Current.Get_Tags().RemoveAtSwap(TagIndex);

        auto&& Storage = InHandle->Storage<ck::FFragment_EntityTag_StorageParams>(entt::id_type{GetTypeHash(InTag)});
        const auto Entity = InHandle.Get_Entity().Get_ID();
        if(Storage.contains(Entity))
        {
            Storage.remove(Entity);
        }
    }

    if (Current.Get_Tags().IsEmpty())
    {
        InHandle.Try_Remove<ck::FFragment_EntityTag_Current>();
    }

    if (WasRemoved)
    {
        ck::UUtils_Signal_EntityTag_OnTagUpdated::Broadcast(InHandle, ck::MakePayload(InHandle, InTag, ECk_EntityTagUpdate::Removed));
    }

    return ECk_SucceededFailed::Succeeded;
}

auto
    UCk_Utils_EntityTag_UE::
    Request_TryRemove_UsingGameplayTag(
        FCk_Handle& InHandle,
        FGameplayTag InTag)
    -> ECk_SucceededFailed
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid Handle passed. Unable to remove Tag [{}] from Entity"), InTag)
    { return ECk_SucceededFailed::Failed; }

    if (NOT InHandle.Has<ck::FFragment_EntityTag_Current>())
    { return ECk_SucceededFailed::Failed; }

    auto& Current = InHandle.Get<ck::FFragment_EntityTag_Current>();

    // Don't allow removing partial matching tags (ex. removing A.B from container with A.B.C)
    if (NOT Current.Get_GameplayTags().GetExplicitGameplayTags().HasTagExact(InTag))
    { return ECk_SucceededFailed::Failed; }

    Current.Get_GameplayTags().UpdateTagCount(InTag, -1);

    for (const auto& Tag : InTag.GetGameplayTagParents())
    {
        const auto& HadPrev = Has(InHandle, Tag.GetTagName());
        const auto& Result = Request_TryRemove(InHandle, Tag.GetTagName());

        CK_ENSURE_IF_NOT(Result == ECk_SucceededFailed::Succeeded,
            TEXT("Failed to remove Tag [{}] as a parent of Tag [{}] from Entity [{}]. This should never happen!"), Tag, InTag, InHandle)
        { return {}; }

        if (HadPrev != Has(InHandle, Tag.GetTagName()))
        {
            ck::UUtils_Signal_EntityTag_OnGameplayTagUpdated::Broadcast(InHandle, ck::MakePayload(InHandle, Tag, ECk_EntityTagUpdate::Removed));
        }
    }

    return ECk_SucceededFailed::Succeeded;
}

auto
    UCk_Utils_EntityTag_UE::
    ForEach_Entity(
        const FCk_Handle& InAnyHandle,
        FName InTag)
        -> TArray<FCk_Handle>
{
    auto EntityTags = TArray<FCk_Handle>{};

    ForEach_Entity(InAnyHandle, InTag, [&](FCk_Handle InEntity)
    {
        EntityTags.Emplace(InEntity);
    });

    return EntityTags;
}

auto
    UCk_Utils_EntityTag_UE::
    ForEach_Entity(
        const FCk_Handle& InAnyHandle,
        FName InTag,
        const TFunction<void(FCk_Handle)>& InFunc)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAnyHandle), TEXT("Invalid Handle passed. Unable to iterate over Entities with Tag [{}]"), InTag)
    { return; }

    auto Handle = InAnyHandle;
    auto& Storage = Handle->Storage<ck::FFragment_EntityTag_StorageParams>(entt::id_type{GetTypeHash(InTag)});

    const auto View = entt::basic_view{Storage};
    View.each([&](const auto InEntity, const ck::FFragment_EntityTag_StorageParams& InParams)
    {
        if (NOT InAnyHandle->IsValid(InEntity))
        { return; }

        auto HandleFromEntity = InAnyHandle.Get_ValidHandle(InEntity);
        InFunc(HandleFromEntity);
    });
}

auto
    UCk_Utils_EntityTag_UE::
    ForEach_Entity_UsingGameplayTag(
        const FCk_Handle& InAnyHandle,
        FGameplayTag InTag)
    -> TArray<FCk_Handle>
{
    return ForEach_Entity(InAnyHandle, InTag.GetTagName());
}

auto
    UCk_Utils_EntityTag_UE::
    ForEach_Entity_UsingGameplayTag(
        const FCk_Handle& InAnyHandle,
        FGameplayTag InTag,
        const TFunction<void(FCk_Handle)>& InFunc)
    -> void
{
    ForEach_Entity(InAnyHandle, InTag.GetTagName(), InFunc);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityTag_UE::
    BindTo_OnTagUpdated(
        FCk_Handle& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_EntityTag_OnTagUpdated& InDelegate)
    -> FCk_Handle
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_EntityTag_OnTagUpdated, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_EntityTag_UE::
    UnbindFrom_OnTagUpdated(
        FCk_Handle& InHandle,
        const FCk_Delegate_EntityTag_OnTagUpdated& InDelegate)
    -> FCk_Handle
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_EntityTag_OnTagUpdated, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_EntityTag_UE::
    BindTo_OnGameplayTagUpdated(
        FCk_Handle& InHandle,
        FGameplayTagContainer InRelevantTags,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_EntityTag_OnGameplayTagUpdated& InDelegate)
    -> FCk_Handle
{
    if (InRelevantTags.IsEmpty())
    {
        CK_SIGNAL_BIND(ck::UUtils_Signal_EntityTag_OnGameplayTagUpdated, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    }
    else
    {
        CK_SIGNAL_BIND_WITH_CONDITION(ck::UUtils_Signal_EntityTag_OnGameplayTagUpdated, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior,
        [InRelevantTags](FCk_Handle InHandle, FGameplayTag InTag, ECk_EntityTagUpdate InUpdateType)
        {
            return InRelevantTags.HasTag(InTag);
        });
    }
    return InHandle;
}

auto
    UCk_Utils_EntityTag_UE::
    UnbindFrom_OnGameplayTagUpdated(
        FCk_Handle& InHandle,
        const FCk_Delegate_EntityTag_OnGameplayTagUpdated& InDelegate)
    -> FCk_Handle
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_EntityTag_OnGameplayTagUpdated, InHandle, InDelegate);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------
