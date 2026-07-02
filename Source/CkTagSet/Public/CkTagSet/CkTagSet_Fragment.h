#pragma once

#include "CkTagSet_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include <variant>
#include <GameplayTagContainer.h>
#include <Serialization/Archive.h>

#include "CkTagSet_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_TagSet_UE;

namespace ck { class FSnapshotContext; }

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ---- Fragment ----

    struct CKTAGSET_API FFragment_TagSet
    {
        CK_GENERATED_BODY(FFragment_TagSet);

        friend class FProcessor_TagSet_HandleRequests;

    public:
        using IsSnapshotable = void;

        // Tier-C: tags persist by NAME (stable across runs — net tag indices are not). A saved tag
        // missing from the load-time tag registry is dropped instead of faulting the whole restore.
        auto SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& /*InCtx*/) -> void
        {
            auto TagNames = TArray<FName>{};

            if (InAr.IsSaving())
            {
                for (const auto& Tag : _Tags)
                { TagNames.Add(Tag.GetTagName()); }

                InAr << TagNames;
            }
            else
            {
                InAr << TagNames;

                _Tags.Reset();
                constexpr auto ErrorIfNotFound = false;
                for (const auto& Name : TagNames)
                {
                    if (const auto Tag = FGameplayTag::RequestGameplayTag(Name, ErrorIfNotFound);
                        Tag.IsValid())
                    { _Tags.AddTag(Tag); }
                }
            }
        }

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

    // Per-feature restore-replication done marker. ck::FTag_Snapshot_JustRestored lives on the OWNER
    // entity and is shared by every owner-resident feature, so no single feature may remove it —
    // each feature pairs it with its own done tag instead. Never leaks across restores: a restore
    // rebuilds entities from snapshot bytes and this tag is not snapshotted.
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_TagSet_RestoreReplicated);

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
