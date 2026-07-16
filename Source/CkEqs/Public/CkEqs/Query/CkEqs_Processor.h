#pragma once

#include "CkEqs/Query/CkEqs_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include <Templates/SharedPointer.h>

// --------------------------------------------------------------------------------------------------------------------

namespace JPH { class PhysicsSystem; }

// --------------------------------------------------------------------------------------------------------------------
// CkEqs_Processor — five processors that drive the EQS pipeline:
//
//   FProcessor_Eqs_HandleRequests   (FGroup_PostTransform)
//       drains FFragment_EqsQuery_Requests on querier entities, spawns child query entities,
//       binds per-request OnComplete delegates via CK_SIGNAL_BIND_REQUEST_FULFILLED.
//   ↓
//   FProcessor_Eqs_Generate         (FGroup_PostTransform, RunAfter HandleRequests)
//       runs the chosen generator on Pending queries, transitions Pending → InProgress.
//   ↓
//   FProcessor_Eqs_Test             (FGroup_PostTransform, RunAfter Generate)
//       runs tests with a per-frame budget cursor; test-boundary atomic
//       yields. DoTick override resets _RemainingBudgetThisFrame. Re-validates the
//       querier each tick and honors FTag_EqsQuery_Cancelled.
//   ↓
//   FProcessor_Eqs_Finalize         (FGroup_PostTransform, RunAfter Test)
//       finalises results, broadcasts OnEqsQueryComplete, transitions InProgress → Complete.
//   ↓
//   FProcessor_Eqs_Cleanup          (FGroup_PostTransform, RunAfter Finalize)
//       destroys Complete + AutoDestroy queries.
//
// All processors use the modern ck_exp::TProcessor pattern with explicit ck::TReadOnly /
// ck::TReadWrite wrappers.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Eqs_HandleRequests
    //
    // Matches any entity with FFragment_EqsQuery_Requests (any entity can be a querier).
    // HandleType is generic FCk_Handle. No physics dependency.
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
    // FProcessor_Eqs_Generate
    //
    // Matches FCk_Handle_EqsQuery + Pending tag. Captures physics weak ref via factory.
    // Runs the chosen generator, transitions Pending → InProgress (or Failed → Complete).
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

        // Generator may need physics for Grid ground-projection traces; ghost out in
        // editor worlds that have no JPH::PhysicsSystem in the registry context.
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
    // FProcessor_Eqs_Test
    //
    // Matches FCk_Handle_EqsQuery + InProgress tag. Captures physics weak ref via factory.
    //
    // _RemainingBudgetThisFrame resets in DoTick (per-tick, NOT per-entity), tests are
    // atomic with respect to budget yields, anti-deadlock clause runs at least one test
    // per tick when the candidate set is larger than the budget.
    //
    // Re-validate querier at the top of ForEachEntity; broadcast OnComplete with
    // empty results + Failed tag if the querier died mid-query.
    //
    // Respect FTag_EqsQuery_Cancelled (caller-issued cancel); fail the query.
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
    // FProcessor_Eqs_Finalize
    //
    // Matches FCk_Handle_EqsQuery + InProgress tag. Runs after Test. No physics dep.
    // Builds final results, broadcasts OnEqsQueryComplete, transitions InProgress → Complete.
    //
    // Note: Test only removes InProgress and adds Complete via Finalize when DoRunTests
    // returned true (all tests done). If Test yielded mid-pipeline, InProgress stays and
    // Finalize doesn't fire — the next frame Test resumes.
    //
    // The state's _NextTestIndex is checked here: if non-zero, Test yielded; do nothing.
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
    // FProcessor_Eqs_Cleanup
    //
    // Matches FCk_Handle_EqsQuery + Complete + AutoDestroy. Destroys the query entity.
    // Runs after Finalize. No physics dep.
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
