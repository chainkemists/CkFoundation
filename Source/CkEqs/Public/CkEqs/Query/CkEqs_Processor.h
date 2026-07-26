#pragma once

#include "CkEqs/Query/CkEqs_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include <Templates/SharedPointer.h>

// --------------------------------------------------------------------------------------------------------------------

namespace JPH { class PhysicsSystem; }

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

        // Grid ground-projection needs a JPH::PhysicsSystem, absent in editor worlds.
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::RuntimeOnly;

    public:
        FProcessor_Eqs_Generate(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem);

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EqsQuery_Params& InParams,
            FFragment_EqsQuery_State& InState) const -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
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
        FProcessor_Eqs_Test(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem);

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
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
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
