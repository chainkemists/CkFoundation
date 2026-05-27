#include "CkEntityTag_Utils.h"

#include "CkEntityTag/CkEntityTag_Fragment.h"
#include "CkEntityTag/CkEntityTag_Log.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityTag_UE::
    Set_StoragePresence(
        FCk_Handle& InHandle,
        FName InTag,
        bool InPresent)
    -> void
{
    auto&& Storage = InHandle.Get_RegistryView().Storage<ck::FFragment_EntityTag_StorageParams>(
        entt::id_type{GetTypeHash(InTag)});
    const auto Entity = InHandle.Get_Entity().Get_ID();

    if (InPresent)
    {
        if (NOT Storage.contains(Entity))
        {
            Storage.emplace<ck::FFragment_EntityTag_StorageParams>(
                Entity,
                ck::FFragment_EntityTag_StorageParams{InTag});
        }
    }
    else
    {
        if (Storage.contains(Entity))
        {
            Storage.remove(Entity);
        }
    }
}

auto
    UCk_Utils_EntityTag_UE::
    Set_SubscriptionMarker(
        FCk_Handle& InListenerHost,
        FName InTagFilter,
        bool InIncrement)
    -> void
{
    auto&& Storage = InListenerHost.Get_RegistryView()
        .Storage<ck::FFragment_EntityTag_AnyEntitySubscription>(
            entt::id_type{GetTypeHash(InTagFilter)});

    const auto Entity = InListenerHost.Get_Entity().Get_ID();

    if (InIncrement)
    {
        if (NOT Storage.contains(Entity))
        {
            Storage.emplace<ck::FFragment_EntityTag_AnyEntitySubscription>(
                Entity,
                ck::FFragment_EntityTag_AnyEntitySubscription{});
        }
        auto& Marker = Storage.get(Entity);
        ++Marker._SubscriptionCount;
    }
    else
    {
        if (NOT Storage.contains(Entity))
        { return; }

        auto& Marker = Storage.get(Entity);
        if (Marker._SubscriptionCount > 0)
        { --Marker._SubscriptionCount; }

        if (Marker._SubscriptionCount <= 0)
        {
            Storage.remove(Entity);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityTag_UE::
    DoBroadcast_AnyEntityListeners(
        FCk_Handle& InMutatedEntity,
        FName InTag,
        ECk_EntityTagUpdate InUpdate)
    -> void
{
    const auto BroadcastTo = [&](FName InStorageKey)
    {
        auto&& Storage = InMutatedEntity.Get_RegistryView()
            .Storage<ck::FFragment_EntityTag_AnyEntitySubscription>(
                entt::id_type{GetTypeHash(InStorageKey)});

        const auto View = entt::basic_view{Storage};
        View.each([&](const auto InListenerEntity, const ck::FFragment_EntityTag_AnyEntitySubscription&)
        {
            if (NOT InMutatedEntity.Get_RegistryView().IsValid(InListenerEntity))
            { return; }

            auto ListenerHandle = InMutatedEntity.Get_ValidHandle(InListenerEntity);
            if (ck::Is_NOT_Valid(ListenerHandle))
            { return; }

            ck::UUtils_Signal_EntityTag_OnTagUpdated_AnyEntity::Broadcast(
                ListenerHandle,
                ck::MakePayload(InTag, InMutatedEntity, InUpdate));
        });
    };

    BroadcastTo(InTag);

    if (NOT InTag.IsNone())
    { BroadcastTo(NAME_None); }
}

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

    if (InTag.IsNone())
    {
        ck::entity_tag::Display(
            TEXT("EntityTag Add ignored: NAME_None is not a valid tag. Entity [{}]"), InHandle);
        return InHandle;
    }

    auto& Requests = InHandle.AddOrGet<ck::FFragment_EntityTag_Requests>();
    Requests._Requests.Emplace(FCk_Request_EntityTag_Add{InTag});

    return InHandle;
}

// ----

auto
    UCk_Utils_EntityTag_UE::
    DoApply_Add(
        FCk_Handle& InHandle,
        FName InTag)
    -> void
{
    auto& Current = InHandle.AddOrGet<ck::FFragment_EntityTag_Current>();

    const auto TagIndex = ck::algo::FindIndex(Current._Tags, [InTag](const ck::FEntityTagCount& InPair)
    {
        return InPair._Name == InTag;
    });

    const auto AddNew = TagIndex == INDEX_NONE;
    if (AddNew)
    {
        Current._Tags.Emplace(InTag, 1);
    }
    else
    {
        Current._Tags[TagIndex]._Count++;
    }

    Set_StoragePresence(InHandle, InTag, true);

    if (AddNew)
    {
        ck::UUtils_Signal_EntityTag_OnTagUpdated::Broadcast(
            InHandle,
            ck::MakePayload(InHandle, InTag, ECk_EntityTagUpdate::Added));

        DoBroadcast_AnyEntityListeners(InHandle, InTag, ECk_EntityTagUpdate::Added);
    }
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

    if (NOT InTag.IsValid())
    { return InHandle; }

    auto& Requests = InHandle.AddOrGet<ck::FFragment_EntityTag_Requests>();
    Requests._Requests.Emplace(FCk_Request_EntityTag_AddGameplayTag{InTag});

    return InHandle;
}

// ----

auto
    UCk_Utils_EntityTag_UE::
    DoApply_AddGameplayTag(
        FCk_Handle& InHandle,
        FGameplayTag InTag)
    -> void
{
    auto& Current = InHandle.AddOrGet<ck::FFragment_EntityTag_Current>();

    const auto GameplayTagIndex = ck::algo::FindIndex(Current._GameplayTagCounts,
        [InTag](const ck::FEntityGameplayTagCount& InPair)
    {
        return InPair._Tag == InTag;
    });

    const auto AddNewGameplayTag = GameplayTagIndex == INDEX_NONE;
    if (AddNewGameplayTag)
    {
        Current._GameplayTagCounts.Emplace(InTag, 1);
    }
    else
    {
        Current._GameplayTagCounts[GameplayTagIndex]._Count++;
    }

    for (const auto& TagInChain : InTag.GetGameplayTagParents())
    {
        DoApply_Add(InHandle, TagInChain.GetTagName());
    }

    if (AddNewGameplayTag)
    {
        ck::UUtils_Signal_EntityTag_OnGameplayTagUpdated::Broadcast(
            InHandle,
            ck::MakePayload(InHandle, InTag, ECk_EntityTagUpdate::Added));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityTag_UE::
    Has(
        const FCk_Handle& InHandle,
        FName InTag)
    -> bool
{
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }

    if (NOT InHandle.Has<ck::FFragment_EntityTag_Current>())
    { return false; }

    const auto& Current = InHandle.Get<ck::FFragment_EntityTag_Current>();

    return ck::algo::AnyOf(Current._Tags, [InTag](const ck::FEntityTagCount& InPair)
    {
        return InPair._Name == InTag;
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
    Has_AnyTag(
        const FCk_Handle& InHandle)
    -> bool
{
    return ck::IsValid(InHandle) && InHandle.Has<ck::FFragment_EntityTag_Current>();
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

    const auto& Current = InHandle.Get<ck::FFragment_EntityTag_Current>();

    auto Container = FGameplayTagContainer{};
    for (const auto& GameplayTagCount : Current._GameplayTagCounts)
    {
        Container.AddTag(GameplayTagCount._Tag);
    }
    return Container;
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

    const auto& Current = InHandle.Get<ck::FFragment_EntityTag_Current>();
    return ck::algo::Transform<TArray<FName>>(Current._Tags, [](const ck::FEntityTagCount& InPair)
    {
        return InPair._Name;
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
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid Handle passed. Unable to remove Tag [{}] from Entity"), InTag)
    { return ECk_SucceededFailed::Failed; }

    auto& Requests = InHandle.AddOrGet<ck::FFragment_EntityTag_Requests>();
    Requests._Requests.Emplace(FCk_Request_EntityTag_TryRemove{InTag});

    return ECk_SucceededFailed::Succeeded;
}

// ----

auto
    UCk_Utils_EntityTag_UE::
    DoApply_TryRemove(
        FCk_Handle& InHandle,
        FName InTag)
    -> void
{
    if (NOT InHandle.Has<ck::FFragment_EntityTag_Current>())
    { return; }

    auto& Current = InHandle.Get<ck::FFragment_EntityTag_Current>();

    const auto TagIndex = ck::algo::FindIndex(Current._Tags, [InTag](const ck::FEntityTagCount& InPair)
    {
        return InPair._Name == InTag;
    });

    if (TagIndex == INDEX_NONE)
    { return; }

    Current._Tags[TagIndex]._Count--;

    const auto WasRemoved = Current._Tags[TagIndex]._Count <= 0;

    if (WasRemoved)
    {
        Current._Tags.RemoveAtSwap(TagIndex);
        Set_StoragePresence(InHandle, InTag, false);
    }

    const auto NowEmpty =
        Current._Tags.IsEmpty() &&
        Current._GameplayTagCounts.IsEmpty();

    if (NowEmpty)
    {
        InHandle.Try_Remove<ck::FFragment_EntityTag_Current>();
    }

    if (WasRemoved)
    {
        ck::UUtils_Signal_EntityTag_OnTagUpdated::Broadcast(
            InHandle,
            ck::MakePayload(InHandle, InTag, ECk_EntityTagUpdate::Removed));

        DoBroadcast_AnyEntityListeners(InHandle, InTag, ECk_EntityTagUpdate::Removed);
    }
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

    if (NOT InTag.IsValid())
    { return ECk_SucceededFailed::Failed; }

    auto& Requests = InHandle.AddOrGet<ck::FFragment_EntityTag_Requests>();
    Requests._Requests.Emplace(FCk_Request_EntityTag_TryRemoveGameplayTag{InTag});

    return ECk_SucceededFailed::Succeeded;
}

// ----

auto
    UCk_Utils_EntityTag_UE::
    DoApply_TryRemoveGameplayTag(
        FCk_Handle& InHandle,
        FGameplayTag InTag)
    -> void
{
    if (NOT InHandle.Has<ck::FFragment_EntityTag_Current>())
    { return; }

    auto& Current = InHandle.Get<ck::FFragment_EntityTag_Current>();

    const auto GameplayTagIndex = ck::algo::FindIndex(Current._GameplayTagCounts,
        [InTag](const ck::FEntityGameplayTagCount& InPair)
    {
        return InPair._Tag == InTag;
    });

    if (GameplayTagIndex == INDEX_NONE)
    { return; }

    Current._GameplayTagCounts[GameplayTagIndex]._Count--;
    const auto WasRemoved = Current._GameplayTagCounts[GameplayTagIndex]._Count <= 0;

    if (WasRemoved)
    {
        Current._GameplayTagCounts.RemoveAtSwap(GameplayTagIndex);
    }

    for (const auto& TagInChain : InTag.GetGameplayTagParents())
    {
        DoApply_TryRemove(InHandle, TagInChain.GetTagName());
    }

    if (WasRemoved)
    {
        ck::UUtils_Signal_EntityTag_OnGameplayTagUpdated::Broadcast(
            InHandle,
            ck::MakePayload(InHandle, InTag, ECk_EntityTagUpdate::Removed));
    }
}

// --------------------------------------------------------------------------------------------------------------------

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
    auto& Storage = Handle.Get_RegistryView().Storage<ck::FFragment_EntityTag_StorageParams>(
        entt::id_type{GetTypeHash(InTag)});

    const auto View = entt::basic_view{Storage};
    View.each([&](const auto InEntity, const ck::FFragment_EntityTag_StorageParams& /*InParams*/)
    {
        if (NOT InAnyHandle.Get_RegistryView().IsValid(InEntity))
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
            [InRelevantTags](FCk_Handle /*InOwner*/, FGameplayTag InTag, ECk_EntityTagUpdate /*InUpdateType*/)
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

auto
    UCk_Utils_EntityTag_UE::
    BindTo_OnTagUpdated_AnyEntity(
        FCk_Handle& InListenerHost,
        FName InTagFilter,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_EntityTag_OnTagUpdated_AnyEntity& InDelegate)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InListenerHost),
        TEXT("Invalid Listener Host Handle [{}] passed to BindTo_OnTagUpdated_AnyEntity"), InListenerHost)
    { return InListenerHost; }

    if (InTagFilter.IsNone())
    {
        CK_SIGNAL_BIND(
            ck::UUtils_Signal_EntityTag_OnTagUpdated_AnyEntity,
            InListenerHost, InDelegate, InBindingPolicy, InPostFireBehavior);
    }
    else
    {
        CK_SIGNAL_BIND_WITH_CONDITION(
            ck::UUtils_Signal_EntityTag_OnTagUpdated_AnyEntity,
            InListenerHost, InDelegate, InBindingPolicy, InPostFireBehavior,
            [InTagFilter](FName InTag, FCk_Handle /*InEntity*/, ECk_EntityTagUpdate /*InUpdate*/)
            {
                return InTag == InTagFilter;
            });
    }

    Set_SubscriptionMarker(InListenerHost, InTagFilter, true);

    return InListenerHost;
}

auto
    UCk_Utils_EntityTag_UE::
    UnbindFrom_OnTagUpdated_AnyEntity(
        FCk_Handle& InListenerHost,
        FName InTagFilter,
        const FCk_Delegate_EntityTag_OnTagUpdated_AnyEntity& InDelegate)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InListenerHost),
        TEXT("Invalid Listener Host Handle [{}] passed to UnbindFrom_OnTagUpdated_AnyEntity"), InListenerHost)
    { return InListenerHost; }

    CK_SIGNAL_UNBIND(
        ck::UUtils_Signal_EntityTag_OnTagUpdated_AnyEntity,
        InListenerHost, InDelegate);

    Set_SubscriptionMarker(InListenerHost, InTagFilter, false);

    return InListenerHost;
}

// --------------------------------------------------------------------------------------------------------------------
