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

    struct CKENTITYTAG_API FFragment_EntityTag_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityTag_Requests);

        friend class FProcessor_EntityTag_HandleRequests;

        using AddType       = FCk_Request_EntityTag_Add;
        using TryRemoveType = FCk_Request_EntityTag_TryRemove;
        using RequestType   = std::variant<AddType, TryRemoveType>;

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

    // --------------------------------------------------------------------------------------------------------------------
}
