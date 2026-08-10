#pragma once

#include "CkEqs/Query/CkEqs_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------
// Pipeline: HandleRequests → Generate → Test → Finalize → Cleanup, all in FGroup_PostTransform,
// chained by the RunAfter lists below.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ----------------------------------------------------------------------------------------------------------------
    // Matched on the request fragment, not a typed handle: any entity can be a querier.
    // ----------------------------------------------------------------------------------------------------------------

    class CKEQS_API FProcessor_Eqs_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Eqs_HandleRequests,
            FCk_Handle,
            ck::TReadWrite<FFragment_EqsQuery_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using MarkedDirtyBy = FFragment_EqsQuery_Requests;
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_EqsQuery_Requests& InRequests) const -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes queriers already tagged for destruction, so a destroyed querier's still-
    // queued requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKEQS_API FProcessor_Eqs_CancelPendingRequests : public ck_exp::TProcessor<
            FProcessor_Eqs_CancelPendingRequests,
            FCk_Handle,
            ck::TReadOnly<FFragment_EqsQuery_Requests>,
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
            const FFragment_EqsQuery_Requests& InRequestsComp)
            -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    class CKEQS_API FProcessor_Eqs_Generate : public ck_exp::TProcessor<
            FProcessor_Eqs_Generate,
            FCk_Handle_EqsQuery,
            ck::TReadOnly<FFragment_EqsQuery_Params>,
            ck::TReadWrite<FFragment_EqsQuery_State>,
            FTag_EqsQuery_Pending,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using RunAfter = TDepList<FProcessor_Eqs_HandleRequests>;
        using MarkedDirtyBy = FTag_EqsQuery_Pending;

        // Grid ground-projection needs a valid ProbeTrace context, absent in editor worlds.
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::RuntimeOnly;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EqsQuery_Params& InParams,
            FFragment_EqsQuery_State& InState) const -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // _RemainingBudgetThisFrame is shared across every query: it resets in DoTick, per-tick and
    // NOT per-entity.
    // ----------------------------------------------------------------------------------------------------------------

    class CKEQS_API FProcessor_Eqs_Test : public ck_exp::TProcessor<
            FProcessor_Eqs_Test,
            FCk_Handle_EqsQuery,
            ck::TReadOnly<FFragment_EqsQuery_Params>,
            ck::TReadWrite<FFragment_EqsQuery_State>,
            ck::TReadWrite<FFragment_EqsQuery_DebugInfo>,
            FTag_EqsQuery_InProgress,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using RunAfter = TDepList<FProcessor_Eqs_Generate>;
        using MarkedDirtyBy = FTag_EqsQuery_InProgress;

        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::RuntimeOnly;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EqsQuery_Params& InParams,
            FFragment_EqsQuery_State& InState,
            FFragment_EqsQuery_DebugInfo& InDebug) -> void;

    private:
        int32 _RemainingBudgetThisFrame = 0;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // Runs on the same InProgress tag as Test and self-gates on State's _NextTestIndex: a yielded
    // query keeps InProgress and resumes next frame instead of finalising.
    // ----------------------------------------------------------------------------------------------------------------

    class CKEQS_API FProcessor_Eqs_Finalize : public ck_exp::TProcessor<
            FProcessor_Eqs_Finalize,
            FCk_Handle_EqsQuery,
            ck::TReadOnly<FFragment_EqsQuery_Params>,
            ck::TReadWrite<FFragment_EqsQuery_State>,
            ck::TReadWrite<FFragment_EqsQuery_DebugInfo>,    // reorder in lockstep
            FTag_EqsQuery_InProgress,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using RunAfter = TDepList<FProcessor_Eqs_Test>;
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EqsQuery_Params& InParams,
            FFragment_EqsQuery_State& InState,
            FFragment_EqsQuery_DebugInfo& InDebug) const -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    class CKEQS_API FProcessor_Eqs_Cleanup : public ck_exp::TProcessor<
            FProcessor_Eqs_Cleanup,
            FCk_Handle_EqsQuery,
            FTag_EqsQuery_Complete,
            FTag_EqsQuery_AutoDestroy,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using RunAfter = TDepList<FProcessor_Eqs_Finalize>;
        using MarkedDirtyBy = FTag_EqsQuery_Complete;
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
