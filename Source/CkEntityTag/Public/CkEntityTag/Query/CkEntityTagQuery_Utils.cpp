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

// ----

auto
    UCk_Utils_EntityTagQuery_UE::
    BindTo_OnContinuousUpdate(
        FCk_Handle_EntityTagQuery& InQuery,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_EntityTagQuery_OnContinuousUpdate& InDelegate)
    -> FCk_Handle_EntityTagQuery
{
    CK_ENSURE_IF_NOT(ck::IsValid(InQuery),
        TEXT("Invalid Query Handle [{}] passed to BindTo_OnContinuousUpdate"), InQuery)
    { return InQuery; }

    CK_SIGNAL_BIND(ck::UUtils_Signal_EntityTagQuery_OnContinuousUpdate, InQuery, InDelegate, InBindingPolicy, InPostFireBehavior);

    auto& Current = InQuery.AddOrGet<ck::FFragment_EntityTagQuery_Current>();
    ++Current._ContinuousUpdateListenerCount;

    return InQuery;
}

auto
    UCk_Utils_EntityTagQuery_UE::
    UnbindFrom_OnContinuousUpdate(
        FCk_Handle_EntityTagQuery& InQuery,
        const FCk_Delegate_EntityTagQuery_OnContinuousUpdate& InDelegate)
    -> FCk_Handle_EntityTagQuery
{
    CK_ENSURE_IF_NOT(ck::IsValid(InQuery),
        TEXT("Invalid Query Handle [{}] passed to UnbindFrom_OnContinuousUpdate"), InQuery)
    { return InQuery; }

    CK_SIGNAL_UNBIND(ck::UUtils_Signal_EntityTagQuery_OnContinuousUpdate, InQuery, InDelegate);

    if (InQuery.Has<ck::FFragment_EntityTagQuery_Current>())
    {
        auto& Current = InQuery.Get<ck::FFragment_EntityTagQuery_Current>();
        if (Current._ContinuousUpdateListenerCount > 0)
        {
            --Current._ContinuousUpdateListenerCount;
        }
    }

    return InQuery;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityTagQuery_UE::
    Request_AddRequirement(
        FCk_Handle_EntityTagQuery& InQuery,
        const FCk_Request_EntityTagQuery_AddRequirement& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_EntityTagQuery
{
    const auto QueryIsValid = ck::IsValid(InQuery);
    CK_ENSURE_IF_NOT(QueryIsValid,
        TEXT("Invalid Query Handle [{}] passed to Request_AddRequirement"), InQuery)
    {
        InDelegate.ExecuteIfBound(InQuery, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InQuery;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    auto& Requests = InQuery.AddOrGet<ck::FFragment_EntityTagQuery_Requests>();
    Requests._Requests.Emplace(InRequest);

    return InQuery;
}

auto
    UCk_Utils_EntityTagQuery_UE::
    Request_RemoveRequirement(
        FCk_Handle_EntityTagQuery& InQuery,
        const FCk_Request_EntityTagQuery_RemoveRequirement& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_EntityTagQuery
{
    const auto QueryIsValid = ck::IsValid(InQuery);
    CK_ENSURE_IF_NOT(QueryIsValid,
        TEXT("Invalid Query Handle [{}] passed to Request_RemoveRequirement"), InQuery)
    {
        InDelegate.ExecuteIfBound(InQuery, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InQuery;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

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
        // Polling snapshot read — no meaningful deltas (caller isn't synchronized with the
        // Evaluate reset cycle). Surface empty add/remove arrays rather than the live
        // accumulator state, which would be misleading.
        Out.Emplace(FCk_EntityTagQuery_Result{
            Reqs[i].Get_Tag(),
            Handles,
            TArray<FCk_Handle>{},
            TArray<FCk_Handle>{}});
    }
    return Out;
}

// --------------------------------------------------------------------------------------------------------------------
// Make_Requirement_* factories (Blueprint/AS surface — mirror the C++ named factories on FCk_EntityTagQuery_Requirement)

auto
    UCk_Utils_EntityTagQuery_UE::
    Make_Requirement_Single(
        FName InTag)
    -> FCk_EntityTagQuery_Requirement
{
    return FCk_EntityTagQuery_Requirement::Single(InTag);
}

auto
    UCk_Utils_EntityTagQuery_UE::
    Make_Requirement_Of(
        FName InTag,
        int32 InCount)
    -> FCk_EntityTagQuery_Requirement
{
    return FCk_EntityTagQuery_Requirement::Of(InTag, InCount);
}

auto
    UCk_Utils_EntityTagQuery_UE::
    Make_Requirement_All(
        FName InTag)
    -> FCk_EntityTagQuery_Requirement
{
    return FCk_EntityTagQuery_Requirement::All(InTag);
}

auto
    UCk_Utils_EntityTagQuery_UE::
    Make_Requirement_Single_WithEnsure(
        FName InTag,
        int32 InMaxAllowed)
    -> FCk_EntityTagQuery_Requirement
{
    return FCk_EntityTagQuery_Requirement::Single(InTag).WithEnsure(InMaxAllowed);
}

auto
    UCk_Utils_EntityTagQuery_UE::
    Make_Requirement_Of_WithEnsure(
        FName InTag,
        int32 InCount,
        int32 InMaxAllowed)
    -> FCk_EntityTagQuery_Requirement
{
    return FCk_EntityTagQuery_Requirement::Of(InTag, InCount).WithEnsure(InMaxAllowed);
}

auto
    UCk_Utils_EntityTagQuery_UE::
    Make_Requirement_All_WithEnsure(
        FName InTag,
        int32 InMaxAllowed)
    -> FCk_EntityTagQuery_Requirement
{
    return FCk_EntityTagQuery_Requirement::All(InTag).WithEnsure(InMaxAllowed);
}

// ---- GameplayTag flavours: convert via InTag.GetTagName() so parent-flattening carries through ----

auto
    UCk_Utils_EntityTagQuery_UE::
    Make_Requirement_Single_FromGameplayTag(
        FGameplayTag InTag)
    -> FCk_EntityTagQuery_Requirement
{
    return Make_Requirement_Single(InTag.GetTagName());
}

auto
    UCk_Utils_EntityTagQuery_UE::
    Make_Requirement_Of_FromGameplayTag(
        FGameplayTag InTag,
        int32 InCount)
    -> FCk_EntityTagQuery_Requirement
{
    return Make_Requirement_Of(InTag.GetTagName(), InCount);
}

auto
    UCk_Utils_EntityTagQuery_UE::
    Make_Requirement_All_FromGameplayTag(
        FGameplayTag InTag)
    -> FCk_EntityTagQuery_Requirement
{
    return Make_Requirement_All(InTag.GetTagName());
}

auto
    UCk_Utils_EntityTagQuery_UE::
    Make_Requirement_Single_FromGameplayTag_WithEnsure(
        FGameplayTag InTag,
        int32 InMaxAllowed)
    -> FCk_EntityTagQuery_Requirement
{
    return Make_Requirement_Single_WithEnsure(InTag.GetTagName(), InMaxAllowed);
}

auto
    UCk_Utils_EntityTagQuery_UE::
    Make_Requirement_Of_FromGameplayTag_WithEnsure(
        FGameplayTag InTag,
        int32 InCount,
        int32 InMaxAllowed)
    -> FCk_EntityTagQuery_Requirement
{
    return Make_Requirement_Of_WithEnsure(InTag.GetTagName(), InCount, InMaxAllowed);
}

auto
    UCk_Utils_EntityTagQuery_UE::
    Make_Requirement_All_FromGameplayTag_WithEnsure(
        FGameplayTag InTag,
        int32 InMaxAllowed)
    -> FCk_EntityTagQuery_Requirement
{
    return Make_Requirement_All_WithEnsure(InTag.GetTagName(), InMaxAllowed);
}
