#pragma once

#include "CkTagSet/CkTagSet_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
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
            TExclude<FTag_DestroyEntity_Initiate>,
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

    // HandleRequests excludes owners already tagged for destruction, so a destroyed TagSet's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKTAGSET_API FProcessor_TagSet_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_TagSet_CancelPendingRequests,
        FCk_Handle_TagSet,
        ck::TReadOnly<FFragment_TagSet_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_TagSet_Requests& InRequestsComp)
            -> void;
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
