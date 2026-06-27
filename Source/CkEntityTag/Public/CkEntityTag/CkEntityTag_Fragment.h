#pragma once

#include <NativeGameplayTags.h>

#include "CkEntityTag_Fragment_Data.h"

#include "CkEcs/Snapshot/CkSnapshot_Context.h" // ck::FSnapshotContext (referenced by FFragment_EntityTag_Current::SerializeSnapshot)

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_EntityTag_UE;
class FArchive;

// --------------------------------------------------------------------------------------------------------------------

CKENTITYTAG_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_EntityTag_Root);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    // Per-tag storage marker placed in the hashed EnTT storage view used by ForEach_Entity.
    // The presence (or absence) of this storage entry mirrors whether the entity currently
    // "Has" the tag — the count itself lives in FFragment_EntityTag_Current._Tags.
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
        using IsSnapshotable = void;

        friend class ::UCk_Utils_EntityTag_UE;

    private:
        TArray<FEntityTagCount>          _Tags;
        TArray<FEntityGameplayTagCount>  _GameplayTagCounts;

    public:
        CK_PROPERTY_GET(_Tags);
        CK_PROPERTY_GET(_GameplayTagCounts);

    public:
        FFragment_EntityTag_Current() = default;

        // Tier-C: authoritative tag membership (name + gameplay-tag counts) with no entity-handle ref. Body in the
        // .cpp serializes both count arrays so a restored entity keeps Has/Get_AllTags. Registered alongside the
        // per-tag StorageParams (which drives ForEach_Entity) so the two views stay consistent across a save/load.
        auto SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& InCtx) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Per-tag subscription marker. The presence of this fragment in the EnTT
    // per-tag-keyed storage (keyed by entt::id_type{GetTypeHash(Tag)}, mirroring
    // FFragment_EntityTag_StorageParams) marks the listener entity as interested
    // in mutations to that tag. NAME_None storage is the wildcard ("any tag").
    //
    // Carries a refcount because the same entity may bind multiple delegates for
    // the same tag — we only want one marker per (entity, tag) pair, removed when
    // all delegates unbind.
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
        using RequestType = std::variant<
            AddType,
            TryRemoveType,
            AddGameplayTagType,
            TryRemoveGameplayTagType>;

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
