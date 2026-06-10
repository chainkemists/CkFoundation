#pragma once

#include "CkTagSet/CkTagSet_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKTAGSET_API FProcessor_TagSet_HandleRequests : public ck_exp::TProcessor<
            FProcessor_TagSet_HandleRequests,
            FCk_Handle_TagSet,
            ck::TReadWrite<FFragment_TagSet>,
            ck::TReadWrite<FFragment_TagSet_Requests>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using MarkedDirtyBy = FFragment_TagSet_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_TagSet& InTagSet,
            FFragment_TagSet_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType& InHandle,
            FFragment_TagSet& InTagSet,
            const FFragment_TagSet_Requests::AddTagsRequestType& InRequest,
            FGameplayTagContainer& OutTagsAdded,
            FGameplayTagContainer& OutTagsRemoved) -> void;

        static auto
        DoHandleRequest(
            HandleType& InHandle,
            FFragment_TagSet& InTagSet,
            const FFragment_TagSet_Requests::RemoveTagsRequestType& InRequest,
            FGameplayTagContainer& OutTagsAdded,
            FGameplayTagContainer& OutTagsRemoved) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // ---- Replicate (Server-side) ----

    class CKTAGSET_API FProcessor_TagSet_Replicate : public ck_exp::TProcessor<
            FProcessor_TagSet_Replicate,
            FCk_Handle_TagSet,
            ck::TReadOnly<FFragment_TagSet>,
            FTag_TagSet_MayRequireReplication,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Replication;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;
        using MarkedDirtyBy = FTag_TagSet_MayRequireReplication;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_TagSet& InTagSet) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // ---- ReplicateOnRestore (Server-side, post-snapshot-load) ----

    // Re-drives replication of a RESTORED TagSet to clients after a snapshot load: once this
    // entity's replication driver is re-established (snapshot respawn pass), re-create the
    // self-resident container entry (Construct is abstained during reconstitution) and re-arm
    // FTag_TagSet_MayRequireReplication. Pairs ck::FTag_Snapshot_JustRestored (shared per-entity,
    // never removed here) with the per-feature FTag_TagSet_RestoreReplicated done tag. The view
    // iterates the clean FFragment_TagSet and POINT-QUERIES the in_place marker (listing it in the
    // view would surface tombstones).
    class CKTAGSET_API FProcessor_TagSet_ReplicateOnRestore : public ck_exp::TProcessor<
            FProcessor_TagSet_ReplicateOnRestore,
            FCk_Handle_TagSet,
            ck::TReadOnly<FFragment_TagSet>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_TagSet& InTagSet) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // ---- SyncReplication (Client-side) ----

    class CKTAGSET_API FProcessor_TagSet_SyncReplication : public ck_exp::TProcessor<
            FProcessor_TagSet_SyncReplication,
            FCk_Handle_TagSet,
            ck::TReadWrite<FFragment_TagSet>,
            ck::TReadOnly<FFragment_TagSet_SyncReplication>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_TagSet_HandleRequests>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::ClientOnly;
        using MarkedDirtyBy = FFragment_TagSet_SyncReplication;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_TagSet& InTagSet,
            const FFragment_TagSet_SyncReplication& InSync) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
