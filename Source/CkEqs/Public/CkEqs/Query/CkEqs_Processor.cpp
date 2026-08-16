#include "CkEqs/Query/CkEqs_Processor.h"

#include "CkEqs/Query/CkEqs_Utils.h"

#include "CkEqs/CkEqs_Log.h"
#include "CkEqs/Query/CkEqs_Algorithm.h"
#include "CkEqs/Settings/CkEqs_ProjectSettings.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkSpatialQuery/Probe/CkProbeTrace_Context.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Eqs_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Eqs_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Eqs_Generate);
CK_REGISTER_PROCESSOR(ck::FProcessor_Eqs_Test);
CK_REGISTER_PROCESSOR(ck::FProcessor_Eqs_Finalize);
CK_REGISTER_PROCESSOR(ck::FProcessor_Eqs_Cleanup);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
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

            // Try_Remove+Add rather than AddOrReplace: FCk_Registry::AddOrReplace (CkRegistry.h:584)
            // forgets to forward InEntity.
            const auto EmptyResults = FCk_Eqs_QueryResults{};
            InQueryHandle.Try_Remove<FFragment_EqsQuery_Results>();
            InQueryHandle.Add<FFragment_EqsQuery_Results>(EmptyResults);

            UUtils_Signal_OnEqsQueryComplete::Broadcast(InQueryHandle, MakePayload(InQueryHandle, EmptyResults));
        }
    }

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

        // Drained from a copy: a completion delegate firing below may enqueue a new request on this
        // same querier, which would reallocate the live container mid-iteration.
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        for (const auto& Request : RequestsCopy)
        {
            // Nothing below rejects a request, so reaching the end of the loop body IS the success condition.
            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(Request, InHandle, Result);

            const auto& QueryParams = Request.Get_QueryParams();

            if (QueryParams.Get_Tests().IsEmpty())
            {
                ck::eqs::Warning(
                    TEXT("EqsQuery from querier [{}]: request has empty Tests array. Query will return generator output unscored. "
                         "Tests are essential at the constructor level — passing {{}} for tests is almost certainly a bug."),
                    InHandle);
            }

            // Child of the querier, so the context-owner cascade kills the query with it.
            auto QueryEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
            QueryEntity.Add<FFragment_EqsQuery_Params>(QueryParams);
            QueryEntity.Add<FFragment_EqsQuery_State>();
            QueryEntity.Add<FFragment_EqsQuery_DebugInfo>();   // lazy-sized in DoRunTests
            QueryEntity.AddOrGet<FTag_EqsQuery_Pending>();

            if (Request.Get_AutoDestroy())
            { QueryEntity.AddOrGet<FTag_EqsQuery_AutoDestroy>(); }

            UCk_Utils_Handle_UE::Set_DebugName(
                QueryEntity,
                FName{*ck::Format_UE(TEXT("EqsQuery_{}"), InHandle)});

            auto TypedQueryHandle = UCk_Utils_Eqs_UE::CastChecked(QueryEntity);

            const auto& OnComplete = Request.Get_OnComplete();
            if (OnComplete.IsBound())
            {
                CK_SIGNAL_BIND_REQUEST_FULFILLED(
                    UUtils_Signal_OnEqsQueryComplete,
                    TypedQueryHandle,
                    OnComplete);
            }

            Result = ECk_Request_OperationResult::Succeeded;
        }

        if (InRequests._Requests.IsEmpty())
        {
            InHandle.Try_Remove<FFragment_EqsQuery_Requests>();
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Eqs_Generate::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InHandle,
            const FFragment_EqsQuery_Params& InParams,
            FFragment_EqsQuery_State& InState) const
        -> void
    {
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

        const auto Generated = FCk_Eqs_Algorithm::DoGenerate(InHandle, InParams, InState,
            FCk_ProbeTrace_Context::Get_ForEntity(InHandle));

        if (NOT Generated || InState.Get_Candidates().IsEmpty())
        {
            ck::eqs::Verbose(TEXT("EqsQuery [{}] generated zero candidates; failing query."), InHandle);
            Eqs_FailQueryAndBroadcast(InHandle);
            return;
        }

        InHandle.Try_Remove<FTag_EqsQuery_Pending>();
        InHandle.AddOrGet<FTag_EqsQuery_InProgress>();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Eqs_Test::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _RemainingBudgetThisFrame = UCk_Utils_Eqs_Settings_UE::Get_MaxCandidatesPerFrame();

        // A configured 0 means disabled-with-warn, NOT unbounded — floor it so the pipeline
        // degrades to DoRunTests' one-test-per-tick slow path instead of deadlocking.
        if (_RemainingBudgetThisFrame <= 0)
        {
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
        if (InHandle.Has<FTag_EqsQuery_Cancelled>())
        {
            ck::eqs::Verbose(TEXT("EqsQuery [{}] cancelled by caller; failing query."), InHandle);
            Eqs_FailQueryAndBroadcast(InHandle);
            return;
        }

        // Re-validated every tick: a multi-frame query must tolerate the querier dying mid-run.
        const auto& Querier = InParams.Get_Querier();
        if (ck::Is_NOT_Valid(Querier))
        {
            ck::eqs::Verbose(TEXT("EqsQuery [{}] querier died mid-query; failing query."), InHandle);
            Eqs_FailQueryAndBroadcast(InHandle);
            return;
        }

        FCk_Eqs_Algorithm::DoRunTests(InHandle, InParams, InState, InDebug,
            FCk_ProbeTrace_Context::Get_ForEntity(InHandle), _RemainingBudgetThisFrame);
    }

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
        const auto TestsStillRunning = InState.Get_NextTestIndex() != 0;
        if (TestsStillRunning)
        { return; }

        auto Results = FCk_Eqs_Algorithm::DoFinalize(InHandle, InParams, InState, InDebug);

        InHandle.Try_Remove<FFragment_EqsQuery_Results>();
        InHandle.Add<FFragment_EqsQuery_Results>(Results);
        InHandle.Try_Remove<FTag_EqsQuery_InProgress>();
        InHandle.AddOrGet<FTag_EqsQuery_Complete>();

        UUtils_Signal_OnEqsQueryComplete::Broadcast(InHandle, MakePayload(InHandle, Results));
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Eqs_Cleanup::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InHandle) const
        -> void
    {
        auto QueryEntity = InHandle.ConvertToHandle();
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(QueryEntity);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Eqs_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EqsQuery_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
