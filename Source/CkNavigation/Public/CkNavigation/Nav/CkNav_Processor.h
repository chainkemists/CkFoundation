#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkNavigation/Nav/CkNav_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::nav
{
    // Drops every deferred FindPath queued for this entity, completing each with Failed_Cancelled.
    // The deferral queue is world-scoped and drained only by the processor TU, so an abandon
    // issued from anywhere else needs this seam to reach it — otherwise a query the caller has
    // abandoned keeps re-projecting until its deferral timeout and then writes the slot the caller
    // just released.
    CKNAVIGATION_API auto PurgeDeferredRequestsFor(FCk_Handle& InHandle) -> void;

    // Releases EVERY in-flight query for this entity — the undrained requests still sitting in
    // FFragment_Nav_Requests and this entity's entries in the deferral queue — completing each
    // with Failed_Cancelled. Deliberately does not touch the result slot: callers differ on what
    // the slot should end up saying (an abandon wants None, a timeout wants Failed), but they
    // agree that nothing may still be computing an answer for an episode that has ended.
    CKNAVIGATION_API auto PurgeInFlightQueriesFor(FCk_Handle& InHandle) -> void;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKNAVIGATION_API FProcessor_Nav_HandleRequests : public ck_exp::TProcessor<
        FProcessor_Nav_HandleRequests,
        FCk_Handle,
        ck::TReadWrite<FFragment_Nav_Requests>,
        ck::TReadWrite<FFragment_Nav_PathResult>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using MarkedDirtyBy = FFragment_Nav_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(FCk_Time InDeltaT) -> void;

        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Nav_Requests& InRequests,
            FFragment_Nav_PathResult& InResult) const -> void;

    private:
        // Reset in DoTick but never enforced — the per-tick query cap is not wired up.
        mutable int32 _BudgetRemainingThisTick = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed entity's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKNAVIGATION_API FProcessor_Nav_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_Nav_CancelPendingRequests,
        FCk_Handle,
        ck::TReadOnly<FFragment_Nav_Requests>,
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
            const FFragment_Nav_Requests& InRequestsComp)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
