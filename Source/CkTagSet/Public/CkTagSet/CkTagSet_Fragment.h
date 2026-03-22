#pragma once

#include "CkTagSet_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include <variant>
#include <GameplayTagContainer.h>

#include "CkTagSet_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_TagSet_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ---- Fragment ----

    struct CKTAGSET_API FFragment_TagSet
    {
        CK_GENERATED_BODY(FFragment_TagSet);

        friend class FProcessor_TagSet_HandleRequests;

    private:
        FGameplayTagContainer _Tags;

    public:
        CK_PROPERTY_GET(_Tags);
        CK_PROPERTY_SET(_Tags);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_TagSet, _Tags);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // ---- Requests ----

    struct CKTAGSET_API FFragment_TagSet_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_TagSet_Requests);

    public:
        friend class FProcessor_TagSet_HandleRequests;
        friend class UCk_Utils_TagSet_UE;

        using AddTagsRequestType    = FCk_Request_TagSet_AddTags;
        using RemoveTagsRequestType = FCk_Request_TagSet_RemoveTags;

        using RequestType = std::variant<AddTagsRequestType, RemoveTagsRequestType>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // ---- Tags ----

    CK_DEFINE_ECS_TAG(FTag_TagSet_MayRequireReplication);

    // --------------------------------------------------------------------------------------------------------------------

    // ---- Sync Replication (Client-side) ----

    struct CKTAGSET_API FFragment_TagSet_SyncReplication
    {
    public:
        CK_GENERATED_BODY(FFragment_TagSet_SyncReplication);

    private:
        FGameplayTagContainer _ReplicatedTags;

    public:
        CK_PROPERTY_GET(_ReplicatedTags);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_TagSet_SyncReplication, _ReplicatedTags);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // ---- Signal ----

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKTAGSET_API,
        TagSet_OnTagsChanged,
        FCk_Delegate_TagSet_OnTagsChanged,
        FCk_Handle_TagSet,
        FGameplayTagContainer,
        FGameplayTagContainer);
}

// --------------------------------------------------------------------------------------------------------------------

// ---- Rep Data ----

USTRUCT()
struct CKTAGSET_API FCk_RepData_TagSet
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_RepData_TagSet);

    UPROPERTY()
    FGameplayTagContainer Tags;
};

namespace ck
{
    using FFragment_ContainerRef_TagSet = TFragment_ContainerEntryRef<FCk_RepData_TagSet>;
}

// --------------------------------------------------------------------------------------------------------------------
