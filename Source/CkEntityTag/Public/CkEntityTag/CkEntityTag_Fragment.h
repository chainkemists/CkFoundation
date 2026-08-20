#pragma once

#include <NativeGameplayTags.h>

#include "CkEntityTag_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_EntityTag_UE;

// --------------------------------------------------------------------------------------------------------------------

CKENTITYTAG_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_EntityTag_Root);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    // Presence in the per-tag hashed EnTT storage (the view behind ForEach_Entity) IS the entity's
    // "Has" answer for that tag; the count itself lives in FFragment_EntityTag_Current._Tags.
    using FFragment_EntityTag_StorageParams = FCk_Fragment_EntityTag_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct FEntityTagCount
    {
        FName _Name;
        int32 _Count = 0;

        FEntityTagCount() = default;
        FEntityTagCount(FName InName, int32 InCount)
            : _Name(InName)
            , _Count(InCount)
        {}
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FEntityGameplayTagCount
    {
        FGameplayTag _Tag;
        int32 _Count = 0;

        FEntityGameplayTagCount() = default;
        FEntityGameplayTagCount(FGameplayTag InTag, int32 InCount)
            : _Tag(InTag)
            , _Count(InCount)
        {}
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKENTITYTAG_API FFragment_EntityTag_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityTag_Current);

        friend class ::UCk_Utils_EntityTag_UE;

    private:
        TArray<FEntityTagCount>          _Tags;
        TArray<FEntityGameplayTagCount>  _GameplayTagCounts;

    public:
        CK_PROPERTY_GET(_Tags);
        CK_PROPERTY_GET(_GameplayTagCounts);

    public:
        FFragment_EntityTag_Current() = default;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Presence in the per-tag-keyed storage (ck_entity_tag_utils::Get_SubscriptionStorageId — the
    // name hash SALTED so these pools can never collide with the FFragment_EntityTag_StorageParams
    // pools that key by the same names) marks the listener as interested in that tag; NAME_None is
    // the wildcard. Refcounted so multiple delegates on one (entity, tag) pair share a marker.
    struct CKENTITYTAG_API FFragment_EntityTag_AnyEntitySubscription
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityTag_AnyEntitySubscription);
        friend class ::UCk_Utils_EntityTag_UE;

    private:
        int32 _SubscriptionCount = 0;

    public:
        CK_PROPERTY_GET(_SubscriptionCount);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKENTITYTAG_API FFragment_EntityTag_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityTag_Requests);

        friend class FProcessor_EntityTag_HandleRequests;
        friend class ::UCk_Utils_EntityTag_UE;

        using AddType                  = FCk_Request_EntityTag_Add;
        using TryRemoveType            = FCk_Request_EntityTag_TryRemove;
        using AddGameplayTagType       = FCk_Request_EntityTag_AddGameplayTag;
        using TryRemoveGameplayTagType = FCk_Request_EntityTag_TryRemoveGameplayTag;
        using RestoreSetType           = FCk_Request_EntityTag_RestoreSet;
        using RequestType = std::variant<
            AddType,
            TryRemoveType,
            AddGameplayTagType,
            TryRemoveGameplayTagType,
            RestoreSetType>;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKENTITYTAG_API,
        EntityTag_OnTagUpdated,
        FCk_Delegate_EntityTag_OnTagUpdated,
        FCk_Handle,
        FName,
        ECk_EntityTagUpdate);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKENTITYTAG_API,
        EntityTag_OnGameplayTagUpdated,
        FCk_Delegate_EntityTag_OnGameplayTagUpdated,
        FCk_Handle,
        FGameplayTag,
        ECk_EntityTagUpdate);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKENTITYTAG_API,
        EntityTag_OnTagUpdated_AnyEntity,
        FCk_Delegate_EntityTag_OnTagUpdated_AnyEntity,
        FName,
        FCk_Handle,
        ECk_EntityTagUpdate);

    // --------------------------------------------------------------------------------------------------------------------
}
