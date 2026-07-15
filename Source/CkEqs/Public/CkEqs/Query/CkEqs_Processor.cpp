#include "CkEqs/Query/CkEqs_Processor.h"

#include "CkEqs/CkEqs_Log.h"
#include "CkEqs/Query/CkEqs_Algorithm.h"
#include "CkEqs/Settings/CkEqs_ProjectSettings.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------
// Processor registrations
// --------------------------------------------------------------------------------------------------------------------

#define CK_EQS_FACTORY(ProcessorType) \
    CK_REGISTER_PROCESSOR_WITH_FACTORY(ProcessorType, \
        [](const FCk_Registry& InRegistry) -> ck::concepts::FTickableType \
        { \
            const auto& PhysicsSystem = InRegistry.GetContext<TWeakPtr<JPH::PhysicsSystem>>(); \
            return ProcessorType{InRegistry, PhysicsSystem}; \
        })

CK_REGISTER_PROCESSOR(ck::FProcessor_Eqs_HandleRequests);
CK_EQS_FACTORY(ck::FProcessor_Eqs_Generate);
CK_EQS_FACTORY(ck::FProcessor_Eqs_Test);
CK_REGISTER_PROCESSOR(ck::FProcessor_Eqs_Finalize);
CK_REGISTER_PROCESSOR(ck::FProcessor_Eqs_Cleanup);

#undef CK_EQS_FACTORY

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ----------------------------------------------------------------------------------------------------------------
    // Local helper — fail a query, broadcast OnComplete, transition tags.
    //
    // Friend access on FFragment_EqsQuery_State / FCk_Eqs_QueryResults is granted to
    // FProcessor_Eqs_Generate / Test / Finalize so the file-scope helper can manipulate
    // them via members of those processors. Here we don't need to mutate State or Results
    // beyond what's exposed publicly; we add tags + broadcast an empty result struct.
    // ----------------------------------------------------------------------------------------------------------------

    namespace
    {
        auto
        Eqs_FailQueryAndBroadcast(
            FCk_Handle_EqsQuery InQueryHandle) -> void
        {
            InQueryHandle.Try_Remove<FTag_EqsQuery_Pending>();
            InQueryHandle.Try_Remove<FTag_EqsQuery_InProgress>();
            InQueryHandle.AddOrGet<FTag_EqsQuery_Failed>();
            InQueryHandle.AddOrGet<FTag_EqsQuery_Complete>();

            // Replace any prior results with an empty results fragment.
            // (Use Try_Remove+Add — there's a latent bug in FCk_Registry::AddOrReplace at
            // CkRegistry.h:584 that forgets to forward InEntity; the Try_Remove+Add pattern
            // sidesteps it cleanly.)
            const auto EmptyResults = FCk_Eqs_QueryResults{};
            InQueryHandle.Try_Remove<FFragment_EqsQuery_Results>();
            InQueryHandle.Add<FFragment_EqsQuery_Results>(EmptyResults);

            UUtils_Signal_OnEqsQueryComplete::Broadcast(InQueryHandle, MakePayload(InQueryHandle, EmptyResults));
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Eqs_HandleRequests
    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Eqs_HandleRequests::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InHandle,
            FFragment_EqsQuery_Requests& InRequests) const
        -> void
    {
        if (InRequests._Requests.IsEmpty())
        {
            InHandle.Try_Remove<FFragment_EqsQuery_Requests>();
            return;
        }

        for (const auto& Request : InRequests._Requests)
        {
            const auto& QueryParams = Request.Get_QueryParams();

            if (QueryParams.Get_Tests().IsEmpty())
            {
                ck::eqs::Warning(
                    TEXT("EqsQuery from querier [{}]: request has empty Tests array. Query will return generator output unscored. "
                         "Tests are essential at the constructor level — passing {{}} for tests is almost certainly a bug."),
                    InHandle);
            }

            // Spawn the query entity as a child of the querier (context-owner cascade
            // gives us automatic lifetime tie-in: if querier dies, query dies).
            auto QueryEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
            QueryEntity.Add<FFragment_EqsQuery_Params>(QueryParams);
            QueryEntity.Add<FFragment_EqsQuery_State>();
            QueryEntity.Add<FFragment_EqsQuery_DebugInfo>();   // Pass-5.1; lazy-sized in DoRunTests
            QueryEntity.AddOrGet<FTag_EqsQuery_Pending>();

            if (Request.Get_AutoDestroy())
            { QueryEntity.AddOrGet<FTag_EqsQuery_AutoDestroy>(); }

            UCk_Utils_Handle_UE::Set_DebugName(
                QueryEntity,
                FName{*ck::Format_UE(TEXT("EqsQuery_{}"), InHandle)});

            // Cast to typed handle for the signal payload + delegate bind.
            auto TypedQueryHandle = ck::StaticCast<FCk_Handle_EqsQuery>(QueryEntity);

            // Optional per-request OnComplete delegate. CK_SIGNAL_BIND_REQUEST_FULFILLED
            // ignores in-flight payloads and auto-unbinds after first fire.
            const auto& OnComplete = Request.Get_OnComplete();
            if (OnComplete.IsBound())
            {
                CK_SIGNAL_BIND_REQUEST_FULFILLED(
                    UUtils_Signal_OnEqsQueryComplete,
                    TypedQueryHandle,
                    OnComplete);
            }
        }

        InRequests._Requests.Reset();
        InHandle.Try_Remove<FFragment_EqsQuery_Requests>();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Eqs_Generate
    // ----------------------------------------------------------------------------------------------------------------

    FProcessor_Eqs_Generate::
        FProcessor_Eqs_Generate(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : TProcessor(InRegistry)
        , _PhysicsSystem(InPhysicsSystem)
    {
    }

    auto
        FProcessor_Eqs_Generate::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InHandle,
            const FFragment_EqsQuery_Params& InParams,
            FFragment_EqsQuery_State& InState) const
        -> void
    {
        // Validate querier validity + has a transform.
        const auto& Querier = InParams.Get_Querier();

        if (ck::Is_NOT_Valid(Querier))
        {
            ck::eqs::Warning(TEXT("EqsQuery [{}] has an INVALID querier entity. Cannot generate candidates."), InHandle);
            Eqs_FailQueryAndBroadcast(InHandle);
            return;
        }

        if (NOT Querier.Has<ck::FFragment_Transform>())
        {
            ck::eqs::Warning(TEXT("EqsQuery [{}] querier [{}] has no FFragment_Transform. Cannot generate candidates."),
                InHandle, Querier);
            Eqs_FailQueryAndBroadcast(InHandle);
            return;
        }

        const auto Generated = FCk_Eqs_Algorithm::DoGenerate(InHandle, InParams, InState, _PhysicsSystem);

        if (NOT Generated || InState.Get_Candidates().IsEmpty())
        {
            ck::eqs::Verbose(TEXT("EqsQuery [{}] generated zero candidates; failing query."), InHandle);
            Eqs_FailQueryAndBroadcast(InHandle);
            return;
        }

        // Generated successfully → transition to InProgress.
        InHandle.Try_Remove<FTag_EqsQuery_Pending>();
        InHandle.AddOrGet<FTag_EqsQuery_InProgress>();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Eqs_Test
    // ----------------------------------------------------------------------------------------------------------------

    FProcessor_Eqs_Test::
        FProcessor_Eqs_Test(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : TProcessor(InRegistry)
        , _PhysicsSystem(InPhysicsSystem)
    {
    }

    auto
        FProcessor_Eqs_Test::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        // P3-E2: per-tick budget reset. Mirrors CkProbe_Processor.cpp:818-827 shape.
        _RemainingBudgetThisFrame = UCk_Utils_Eqs_Settings_UE::Get_MaxCandidatesPerFrame();

        // P3-E2 / P3-E8: 0 == disabled-with-warn, NOT unbounded. Default to a non-zero
        // floor so the query pipeline doesn't deadlock when configured at 0.
        if (_RemainingBudgetThisFrame <= 0)
        {
            // Static one-time warn would be ideal; for v1 we let the anti-deadlock clause
            // in DoRunTests run one test per tick as the floor (effectively a slow path).
            _RemainingBudgetThisFrame = 1;
        }

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_Eqs_Test::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InHandle,
            const FFragment_EqsQuery_Params& InParams,
            FFragment_EqsQuery_State& InState,
            FFragment_EqsQuery_DebugInfo& InDebug)
        -> void
    {
        // Pass-3.1 E3: caller-issued cancel.
        if (InHandle.Has<FTag_EqsQuery_Cancelled>())
        {
            ck::eqs::Verbose(TEXT("EqsQuery [{}] cancelled by caller; failing query."), InHandle);
            Eqs_FailQueryAndBroadcast(InHandle);
            return;
        }

        // P3-E6: re-validate querier — multi-frame queries must tolerate the querier dying
        // between Generate and Test (or between Test resumes).
        const auto& Querier = InParams.Get_Querier();
        if (ck::Is_NOT_Valid(Querier))
        {
            ck::eqs::Verbose(TEXT("EqsQuery [{}] querier died mid-query; failing query."), InHandle);
            Eqs_FailQueryAndBroadcast(InHandle);
            return;
        }

        // Run tests with the budget cursor. May yield at a test boundary.
        FCk_Eqs_Algorithm::DoRunTests(InHandle, InParams, InState, InDebug, _PhysicsSystem, _RemainingBudgetThisFrame);
        // No tag changes here — Finalize handles the transition once the cursor resets to 0.
    }

    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Eqs_Finalize
    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Eqs_Finalize::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InHandle,
            const FFragment_EqsQuery_Params& InParams,
            FFragment_EqsQuery_State& InState,
            FFragment_EqsQuery_DebugInfo& InDebug) const
        -> void
    {
        // If Test yielded mid-pipeline, _NextTestIndex is non-zero; nothing to finalise yet.
        if (InState.Get_NextTestIndex() != 0)
        { return; }

        auto Results = FCk_Eqs_Algorithm::DoFinalize(InHandle, InParams, InState, InDebug);

        InHandle.Try_Remove<FFragment_EqsQuery_Results>();
        InHandle.Add<FFragment_EqsQuery_Results>(Results);
        InHandle.Try_Remove<FTag_EqsQuery_InProgress>();
        InHandle.AddOrGet<FTag_EqsQuery_Complete>();

        UUtils_Signal_OnEqsQueryComplete::Broadcast(InHandle, MakePayload(InHandle, Results));
    }

    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Eqs_Cleanup
    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Eqs_Cleanup::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InHandle) const
        -> void
    {
        auto QueryEntity = static_cast<FCk_Handle&>(InHandle);
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(QueryEntity);
    }
}

// --------------------------------------------------------------------------------------------------------------------
