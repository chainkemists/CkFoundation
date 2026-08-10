#include "CkEqs/Query/CkEqs_Utils.h"

#include "CkEqs/CkEqs_Log.h"
#include "CkEqs/Query/CkEqs_Algorithm.h"
#include "CkEqs/Settings/CkEqs_ProjectSettings.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkSpatialQuery/Probe/CkProbeTrace_Context.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(
    UCk_Utils_Eqs_UE,
    FCk_Handle_EqsQuery,
    FFragment_EqsQuery_Params,
    FFragment_EqsQuery_State)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Eqs_UE::
    Request_RunQuery(
        FCk_Handle& InQuerierEntity,
        const FCk_Request_Eqs_RunQuery& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle
{
    const auto QuerierIsValid = ck::IsValid(InQuerierEntity);
    CK_ENSURE_IF_NOT(QuerierIsValid,
        TEXT("Request_RunQuery called with invalid querier"))
    {}
    if (NOT QuerierIsValid)
    {
        InDelegate.ExecuteIfBound(InQuerierEntity, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InQuerierEntity;
    }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InQuerierEntity);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("CkEqs is server-authoritative. Querier [{}] is not authoritative; query rejected."),
        InQuerierEntity)
    {}
    if (NOT HasAuthority)
    {
        InDelegate.ExecuteIfBound(InQuerierEntity, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InQuerierEntity;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InQuerierEntity.AddOrGet<FFragment_EqsQuery_Requests>().Update_Requests(
        [&](TArray<FCk_Request_Eqs_RunQuery>& InContainer)
    {
        InContainer.Emplace(InRequest);
    });

    return InQuerierEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Eqs_UE::
    Request_RunQuery_Immediate(
        FCk_Handle& InQuerierEntity,
        const FCk_Eqs_QueryParams& InQueryParams,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_EqsQuery
{
    const auto QuerierIsValid = ck::IsValid(InQuerierEntity);
    CK_ENSURE_IF_NOT(QuerierIsValid,
        TEXT("Request_RunQuery_Immediate called with invalid querier"))
    {}
    if (NOT QuerierIsValid)
    {
        InDelegate.ExecuteIfBound(InQuerierEntity, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InQuerierEntity);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("CkEqs is server-authoritative. Querier [{}] is not authoritative; immediate query rejected."),
        InQuerierEntity)
    {}
    if (NOT HasAuthority)
    {
        InDelegate.ExecuteIfBound(InQuerierEntity, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    auto QueryEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InQuerierEntity);
    QueryEntity.Add<FFragment_EqsQuery_Params>(InQueryParams);
    auto& State = QueryEntity.Add<FFragment_EqsQuery_State>();
    auto& DebugInfo = QueryEntity.Add<FFragment_EqsQuery_DebugInfo>();

    auto TypedQuery = ck::StaticCast<FCk_Handle_EqsQuery>(QueryEntity);
    UCk_Utils_Handle_UE::Set_DebugName(QueryEntity,
        FName{*ck::Format_UE(TEXT("EqsQueryImmediate_{}"), InQuerierEntity)});

    const auto TraceContext = FCk_ProbeTrace_Context::Get_ForEntity(QueryEntity);

    auto Results = FCk_Eqs_QueryResults{};

    const auto Generated = FCk_Eqs_Algorithm::DoGenerate(TypedQuery, InQueryParams, State, TraceContext);

    if (Generated)
    {
        const auto HardCap = UCk_Utils_Eqs_Settings_UE::Get_MaxCandidates_ImmediatePathHardCap();
        if (State.Get_Candidates().Num() > HardCap)
        {
            ck::eqs::Warning(
                TEXT("EqsQuery_Immediate: generator produced {} candidates, hard-capping to {}. "
                     "Use Request_RunQuery (deferred) for larger queries."),
                State.Get_Candidates().Num(), HardCap);
            // Utils is a friend of FFragment_EqsQuery_State; the const_cast is the mutation path.
            const_cast<TArray<FCk_Eqs_Candidate>&>(State.Get_Candidates()).SetNum(HardCap);
        }

        auto UnboundedBudget = TNumericLimits<int32>::Max();
        FCk_Eqs_Algorithm::DoRunTests(TypedQuery, InQueryParams, State, DebugInfo, TraceContext, UnboundedBudget);
        Results = FCk_Eqs_Algorithm::DoFinalize(TypedQuery, InQueryParams, State, DebugInfo);
    }
    else
    {
        QueryEntity.AddOrGet<ck::FTag_EqsQuery_Failed>();
    }

    QueryEntity.Try_Remove<FFragment_EqsQuery_Results>();
    QueryEntity.Add<FFragment_EqsQuery_Results>(Results);
    QueryEntity.AddOrGet<ck::FTag_EqsQuery_Complete>();

    // Deliberately no OnEqsQueryComplete broadcast: the signal would fire before the caller has
    // the handle, so no delegate could ever be bound in time.

    // Immediate execution — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InQuerierEntity, Generated
        ? ECk_Request_OperationResult::Succeeded
        : ECk_Request_OperationResult::Failed);

    return TypedQuery;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Eqs_UE::
    Request_CancelQuery(
        FCk_Handle_EqsQuery& InQueryEntity,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_EqsQuery
{
    const auto QueryIsValid = ck::IsValid(InQueryEntity);
    CK_ENSURE_IF_NOT(QueryIsValid,
        TEXT("Request_CancelQuery called with invalid query handle"))
    {}
    if (NOT QueryIsValid)
    {
        InDelegate.ExecuteIfBound(InQueryEntity, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InQueryEntity;
    }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InQueryEntity);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("CkEqs is server-authoritative. Query [{}] cancel rejected (not authoritative)."),
        InQueryEntity)
    {}
    if (NOT HasAuthority)
    {
        InDelegate.ExecuteIfBound(InQueryEntity, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InQueryEntity;
    }

    InQueryEntity.AddOrGet<ck::FTag_EqsQuery_Cancelled>();

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InQueryEntity, ECk_Request_OperationResult::Succeeded);

    return InQueryEntity;
}

auto
    UCk_Utils_Eqs_UE::
    Request_CancelAllQueries(
        FCk_Handle& InQuerierEntity,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> int32
{
    const auto QuerierIsValid = ck::IsValid(InQuerierEntity);
    CK_ENSURE_IF_NOT(QuerierIsValid,
        TEXT("Request_CancelAllQueries called with invalid querier."))
    {}
    if (NOT QuerierIsValid)
    {
        InDelegate.ExecuteIfBound(InQuerierEntity, ECk_Request_OperationResult::Failed_NotEnqueued);
        return 0;
    }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InQuerierEntity);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("CkEqs is server-authoritative. Querier [{}] is not authoritative; cancel-all rejected."),
        InQuerierEntity)
    {}
    if (NOT HasAuthority)
    {
        InDelegate.ExecuteIfBound(InQuerierEntity, ECk_Request_OperationResult::Failed_NotEnqueued);
        return 0;
    }

    auto Count = int32{0};

    // There is no per-querier index of queries: the only link is the context owner, so this scans
    // every query entity in the registry.
    InQuerierEntity.View<FFragment_EqsQuery_Params>().ForEach(
        [&](FCk_Entity InEntity, const FFragment_EqsQuery_Params&)
    {
        auto QueryHandle = ck::MakeHandle(InEntity, InQuerierEntity);

        if (UCk_Utils_ContextOwner_UE::Get_ContextOwner(QueryHandle) != InQuerierEntity)
        { return; }

        if (QueryHandle.Has<ck::FTag_EqsQuery_Cancelled>() ||
            QueryHandle.Has<ck::FTag_EqsQuery_Complete>())
        { return; }

        QueryHandle.AddOrGet<ck::FTag_EqsQuery_Cancelled>();
        ++Count;
    });

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InQuerierEntity, ECk_Request_OperationResult::Succeeded);

    return Count;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Eqs_UE::
    BindTo_OnComplete(
        FCk_Handle_EqsQuery& InQueryEntity,
        const FCk_Delegate_EqsQuery_OnComplete& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_EqsQuery
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnEqsQueryComplete, InQueryEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InQueryEntity;
}

auto
    UCk_Utils_Eqs_UE::
    UnbindFrom_OnComplete(
        FCk_Handle_EqsQuery& InQueryEntity,
        const FCk_Delegate_EqsQuery_OnComplete& InDelegate)
    -> FCk_Handle_EqsQuery
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnEqsQueryComplete, InQueryEntity, InDelegate);
    return InQueryEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Eqs_UE::
    Get_HasResults(
        const FCk_Handle_EqsQuery& InQuery)
    -> bool
{
    if (ck::Is_NOT_Valid(InQuery))
    { return false; }
    if (NOT InQuery.Has<FFragment_EqsQuery_Results>())
    { return false; }
    return InQuery.Get<FFragment_EqsQuery_Results>().Get_HasResults();
}

auto
    UCk_Utils_Eqs_UE::
    Get_BestLocation(
        const FCk_Handle_EqsQuery& InQuery)
    -> FVector
{
    if (ck::Is_NOT_Valid(InQuery))
    { return FVector::ZeroVector; }
    if (NOT InQuery.Has<FFragment_EqsQuery_Results>())
    { return FVector::ZeroVector; }
    return InQuery.Get<FFragment_EqsQuery_Results>().Get_BestLocation();
}

auto
    UCk_Utils_Eqs_UE::
    Get_BestEntity(
        const FCk_Handle_EqsQuery& InQuery)
    -> FCk_Handle
{
    if (ck::Is_NOT_Valid(InQuery))
    { return {}; }
    if (NOT InQuery.Has<FFragment_EqsQuery_Results>())
    { return {}; }
    return InQuery.Get<FFragment_EqsQuery_Results>().Get_BestEntity();
}

auto
    UCk_Utils_Eqs_UE::
    Get_AllCandidates(
        const FCk_Handle_EqsQuery& InQuery)
    -> TArray<FCk_Eqs_Candidate>
{
    if (ck::Is_NOT_Valid(InQuery))
    { return {}; }
    if (NOT InQuery.Has<FFragment_EqsQuery_Results>())
    { return {}; }
    return InQuery.Get<FFragment_EqsQuery_Results>().Get_Candidates();
}

auto
    UCk_Utils_Eqs_UE::
    Get_IsComplete(
        const FCk_Handle_EqsQuery& InQuery)
    -> bool
{
    if (ck::Is_NOT_Valid(InQuery))
    { return false; }
    return InQuery.Has<ck::FTag_EqsQuery_Complete>();
}

auto
    UCk_Utils_Eqs_UE::
    Get_IsFailed(
        const FCk_Handle_EqsQuery& InQuery)
    -> bool
{
    if (ck::Is_NOT_Valid(InQuery))
    { return false; }
    return InQuery.Has<ck::FTag_EqsQuery_Failed>();
}

// --------------------------------------------------------------------------------------------------------------------
