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

namespace JPH { class PhysicsSystem; }

// --------------------------------------------------------------------------------------------------------------------
// Has / Cast / DoCast / DoCastChecked / Get_InvalidHandle — generated via macro.
// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(
    UCk_Utils_Eqs_UE,
    FCk_Handle_EqsQuery,
    FFragment_EqsQuery_Params,
    FFragment_EqsQuery_State)

// --------------------------------------------------------------------------------------------------------------------
// Request_RunQuery — deferred path
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Eqs_UE::
    Request_RunQuery(
        FCk_Handle& InQuerierEntity,
        const FCk_Request_Eqs_RunQuery& InRequest)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InQuerierEntity),
        TEXT("Request_RunQuery called with invalid querier"))
    { return InQuerierEntity; }

    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_HasAuthority(InQuerierEntity),
        TEXT("CkEqs is server-authoritative. Querier [{}] is not authoritative; query rejected."),
        InQuerierEntity)
    { return InQuerierEntity; }

    InQuerierEntity.AddOrGet<FFragment_EqsQuery_Requests>().Update_Requests(
        [&](TArray<FCk_Request_Eqs_RunQuery>& InContainer)
    {
        InContainer.Emplace(InRequest);
    });

    return InQuerierEntity;
}

// --------------------------------------------------------------------------------------------------------------------
// Request_RunQuery_Immediate — synchronous bypass path
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Eqs_UE::
    Request_RunQuery_Immediate(
        FCk_Handle& InQuerierEntity,
        const FCk_Eqs_QueryParams& InQueryParams)
    -> FCk_Handle_EqsQuery
{
    CK_ENSURE_IF_NOT(ck::IsValid(InQuerierEntity),
        TEXT("Request_RunQuery_Immediate called with invalid querier"))
    { return {}; }

    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_HasAuthority(InQuerierEntity),
        TEXT("CkEqs is server-authoritative. Querier [{}] is not authoritative; immediate query rejected."),
        InQuerierEntity)
    { return {}; }

    auto QueryEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InQuerierEntity);
    QueryEntity.Add<FFragment_EqsQuery_Params>(InQueryParams);
    auto& State = QueryEntity.Add<FFragment_EqsQuery_State>();
    auto& DebugInfo = QueryEntity.Add<FFragment_EqsQuery_DebugInfo>();

    auto TypedQuery = ck::StaticCast<FCk_Handle_EqsQuery>(QueryEntity);
    UCk_Utils_Handle_UE::Set_DebugName(QueryEntity,
        FName{*ck::Format_UE(TEXT("EqsQueryImmediate_{}"), InQuerierEntity)});

    const auto Registry       = QueryEntity.Get_RegistryView();
    const auto& PhysicsSystem = Registry.GetContext<TWeakPtr<JPH::PhysicsSystem>>();

    auto Results = FCk_Eqs_QueryResults{};

    if (FCk_Eqs_Algorithm::DoGenerate(TypedQuery, InQueryParams, State, PhysicsSystem))
    {
        // Hard-cap candidate count for the Immediate path so a runaway generator
        // can't stall the main thread. Truncate-with-warning, do NOT silently drop —
        // include both numbers (configured count + cap) so the consumer can see the
        // root cause, not just the symptom.
        const auto HardCap = UCk_Utils_Eqs_Settings_UE::Get_MaxCandidates_ImmediatePathHardCap();
        if (State.Get_Candidates().Num() > HardCap)
        {
            ck::eqs::Warning(
                TEXT("EqsQuery_Immediate: generator produced {} candidates, hard-capping to {}. "
                     "Use Request_RunQuery (deferred) for larger queries."),
                State.Get_Candidates().Num(), HardCap);
            // Truncation requires friend access (which Utils has via the friend grant
            // on FFragment_EqsQuery_State).
            const_cast<TArray<FCk_Eqs_Candidate>&>(State.Get_Candidates()).SetNum(HardCap);
        }

        auto UnboundedBudget = TNumericLimits<int32>::Max();
        FCk_Eqs_Algorithm::DoRunTests(TypedQuery, InQueryParams, State, DebugInfo, PhysicsSystem, UnboundedBudget);
        Results = FCk_Eqs_Algorithm::DoFinalize(TypedQuery, InQueryParams, State, DebugInfo);
    }
    else
    {
        QueryEntity.AddOrGet<ck::FTag_EqsQuery_Failed>();
    }

    QueryEntity.Try_Remove<FFragment_EqsQuery_Results>();
    QueryEntity.Add<FFragment_EqsQuery_Results>(Results);
    QueryEntity.AddOrGet<ck::FTag_EqsQuery_Complete>();

    // Immediate does NOT broadcast OnEqsQueryComplete. The signal would fire before
    // the caller has the handle, making delegate binding impossible to time. Callers read
    // results via the returned handle's accessors. For delegate-driven completion, use
    // the deferred Request_RunQuery path.

    return TypedQuery;
}

// --------------------------------------------------------------------------------------------------------------------
// Request_CancelQuery
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Eqs_UE::
    Request_CancelQuery(
        FCk_Handle_EqsQuery& InQueryEntity)
    -> FCk_Handle_EqsQuery
{
    CK_ENSURE_IF_NOT(ck::IsValid(InQueryEntity),
        TEXT("Request_CancelQuery called with invalid query handle"))
    { return InQueryEntity; }

    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_HasAuthority(InQueryEntity),
        TEXT("CkEqs is server-authoritative. Query [{}] cancel rejected (not authoritative)."),
        InQueryEntity)
    { return InQueryEntity; }

    InQueryEntity.AddOrGet<ck::FTag_EqsQuery_Cancelled>();
    return InQueryEntity;
}

auto
    UCk_Utils_Eqs_UE::
    Request_CancelAllQueries(
        FCk_Handle& InQuerierEntity)
    -> int32
{
    CK_ENSURE_IF_NOT(ck::IsValid(InQuerierEntity),
        TEXT("Request_CancelAllQueries called with invalid querier."))
    { return 0; }

    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_HasAuthority(InQuerierEntity),
        TEXT("CkEqs is server-authoritative. Querier [{}] is not authoritative; cancel-all rejected."),
        InQuerierEntity)
    { return 0; }

    auto Count = int32{0};

    // Query entities are context-owned by their querier (Request_CreateEntity(InQuerier)).
    // Iterate every entity carrying FFragment_EqsQuery_Params and tag the ones whose
    // context owner matches InQuerierEntity. Skip already-cancelled / already-complete
    // queries (no point re-tagging — harmless but pollutes the count).
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

    return Count;
}

// --------------------------------------------------------------------------------------------------------------------
// Signal bindings
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
// Result accessors
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
