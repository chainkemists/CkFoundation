#pragma once

#include "CkEntityTag_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKENTITYTAG_API FProcessor_EntityTag_HandleRequests : public ck_exp::TProcessor<
            FProcessor_EntityTag_HandleRequests,
            FCk_Handle,
            ck::TReadWrite<FFragment_EntityTag_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_EntityTag_Requests;
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_EntityTag_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType& InHandle,
            const FCk_Request_EntityTag_Add& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType& InHandle,
            const FCk_Request_EntityTag_TryRemove& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType& InHandle,
            const FCk_Request_EntityTag_AddGameplayTag& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType& InHandle,
            const FCk_Request_EntityTag_TryRemoveGameplayTag& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType& InHandle,
            const FCk_Request_EntityTag_RestoreSet& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed entity's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKENTITYTAG_API FProcessor_EntityTag_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_EntityTag_CancelPendingRequests,
        FCk_Handle,
        ck::TReadOnly<FFragment_EntityTag_Requests>,
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
            const FFragment_EntityTag_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // When an entity carrying tags is destroyed, broadcast a Removed event to global
    // listeners for every tag it currently carries. Runs in FGroup_EndPlay with
    // CK_IF_END_PLAY, mirroring FProcessor_EntityTagQuery_TrackedEntity_Destructor.
    class CKENTITYTAG_API FProcessor_EntityTag_BroadcastOnDestroy : public ck_exp::TProcessor<
            FProcessor_EntityTag_BroadcastOnDestroy,
            FCk_Handle,
            ck::TReadOnly<FFragment_EntityTag_Current>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EntityTag_Current& InCurrent) const -> void;
    };
}
