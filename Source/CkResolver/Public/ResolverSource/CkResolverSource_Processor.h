#pragma once

#include "CkResolverSource_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKRESOLVER_API FProcessor_ResolverSource_HandleRequests : public ck_exp::TProcessor<
            FProcessor_ResolverSource_HandleRequests,
            FCk_Handle_ResolverSource,
            ck::TReadOnly<FFragment_ResolverSource_Params>,
            ck::TReadWrite<FFragment_ResolverSource_Requests>,
            ck::TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using MarkedDirtyBy = FFragment_ResolverSource_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const ck::FFragment_ResolverSource_Params& InParams,
            FFragment_ResolverSource_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const ck::FFragment_ResolverSource_Params& InParams,
            const FCk_Request_ResolverSource_InitiateNewResolution& InNewResolution) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed source's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKRESOLVER_API FProcessor_ResolverSource_CancelPendingRequests : public ck_exp::TProcessor<
            FProcessor_ResolverSource_CancelPendingRequests,
            FCk_Handle_ResolverSource,
            ck::TReadOnly<FFragment_ResolverSource_Requests>,
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
            const FFragment_ResolverSource_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
