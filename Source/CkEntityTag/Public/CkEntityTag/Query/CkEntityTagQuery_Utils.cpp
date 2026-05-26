#include "CkEntityTagQuery_Utils.h"

#include "CkEntityTag/CkEntityTag_Log.h"
#include "CkEntityTag/Query/CkEntityTagQuery_Fragment.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityTagQuery_UE::
    Add(
        FCk_Handle& InOwner)
    -> FCk_Handle_EntityTagQuery
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwner),
        TEXT("Invalid Owner Handle [{}] passed to EntityTagQuery::Add"), InOwner)
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);

    NewEntity.Add<ck::FFragment_EntityTagQuery_Current>();

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
    UCk_Utils_Handle_UE::Set_DebugName(
        NewEntity,
        *ck::Format_UE(TEXT("EntityTagQuery (Owner: {})"), InOwner));
#endif

    return Cast(NewEntity);
}

// ----

auto
    UCk_Utils_EntityTagQuery_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return ck::IsValid(InHandle) && InHandle.Has<ck::FFragment_EntityTagQuery_Current>();
}

// ----

auto
    UCk_Utils_EntityTagQuery_UE::
    BindTo_OnSatisfied(
        FCk_Handle_EntityTagQuery& InQuery,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_EntityTagQuery_OnSatisfied& InDelegate)
    -> FCk_Handle_EntityTagQuery
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_EntityTagQuery_OnSatisfied, InQuery, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InQuery;
}

auto
    UCk_Utils_EntityTagQuery_UE::
    UnbindFrom_OnSatisfied(
        FCk_Handle_EntityTagQuery& InQuery,
        const FCk_Delegate_EntityTagQuery_OnSatisfied& InDelegate)
    -> FCk_Handle_EntityTagQuery
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_EntityTagQuery_OnSatisfied, InQuery, InDelegate);
    return InQuery;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityTagQuery_UE::
    Request_AddRequirement(
        FCk_Handle_EntityTagQuery& InQuery,
        const FCk_Request_EntityTagQuery_AddRequirement& InRequest)
    -> FCk_Handle_EntityTagQuery
{
    CK_ENSURE_IF_NOT(ck::IsValid(InQuery),
        TEXT("Invalid Query Handle [{}] passed to Request_AddRequirement"), InQuery)
    { return InQuery; }

    auto& Requests = InQuery.AddOrGet<ck::FFragment_EntityTagQuery_Requests>();
    Requests._Requests.Emplace(InRequest);

    return InQuery;
}

auto
    UCk_Utils_EntityTagQuery_UE::
    Request_RemoveRequirement(
        FCk_Handle_EntityTagQuery& InQuery,
        const FCk_Request_EntityTagQuery_RemoveRequirement& InRequest)
    -> FCk_Handle_EntityTagQuery
{
    CK_ENSURE_IF_NOT(ck::IsValid(InQuery),
        TEXT("Invalid Query Handle [{}] passed to Request_RemoveRequirement"), InQuery)
    { return InQuery; }

    auto& Requests = InQuery.AddOrGet<ck::FFragment_EntityTagQuery_Requests>();
    Requests._Requests.Emplace(InRequest);

    return InQuery;
}

// ----

auto
    UCk_Utils_EntityTagQuery_UE::
    Get_IsSatisfied(
        const FCk_Handle_EntityTagQuery& InQuery)
    -> bool
{
    if (ck::Is_NOT_Valid(InQuery))
    { return false; }

    if (NOT InQuery.Has<ck::FFragment_EntityTagQuery_Current>())
    { return false; }

    return InQuery.Get<ck::FFragment_EntityTagQuery_Current>().Get_IsSatisfied();
}

auto
    UCk_Utils_EntityTagQuery_UE::
    Get_AllRequirements(
        const FCk_Handle_EntityTagQuery& InQuery)
    -> TArray<FCk_EntityTagQuery_Requirement>
{
    if (ck::Is_NOT_Valid(InQuery) || NOT InQuery.Has<ck::FFragment_EntityTagQuery_Current>())
    { return {}; }

    return InQuery.Get<ck::FFragment_EntityTagQuery_Current>().Get_Requirements();
}

auto
    UCk_Utils_EntityTagQuery_UE::
    Get_CurrentResults(
        const FCk_Handle_EntityTagQuery& InQuery)
    -> TArray<FCk_EntityTagQuery_Result>
{
    if (ck::Is_NOT_Valid(InQuery) || NOT InQuery.Has<ck::FFragment_EntityTagQuery_Current>())
    { return {}; }

    const auto& Current = InQuery.Get<ck::FFragment_EntityTagQuery_Current>();
    const auto& Reqs    = Current.Get_Requirements();
    const auto& Results = Current.Get_ResultsPerRequirement();

    auto Out = TArray<FCk_EntityTagQuery_Result>{};
    Out.Reserve(Reqs.Num());
    for (int32 i = 0; i < Reqs.Num(); ++i)
    {
        const auto Handles = (i < Results.Num()) ? Results[i] : TArray<FCk_Handle>{};
        Out.Emplace(FCk_EntityTagQuery_Result{Reqs[i].Get_Tag(), Handles});
    }
    return Out;
}
