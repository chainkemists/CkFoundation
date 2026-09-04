#include "CkGroundNavPath_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkGroundNav/CkGroundNav_Log.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    auto
        Get_LinksOnPath(
            const FCk_GroundNavPath_Result& InResult)
        -> TArray<FCk_GroundNavPath_LinkSpan>
    {
        auto Spans = TArray<FCk_GroundNavPath_LinkSpan>{};

        for (const auto& LinkWaypoint : InResult.Get_LinkWaypoints())
        {
            if (LinkWaypoint.Get_Role() == ECk_GroundNavPath_LinkWaypointRole::Entry)
            {
                Spans.Emplace(FCk_GroundNavPath_LinkSpan{
                    LinkWaypoint.Get_LinkId(),
                    LinkWaypoint.Get_WaypointIndex(),
                    LinkWaypoint.Get_DistanceFromStartUu(),
                    LinkWaypoint.Get_EntryDirection()});

                continue;
            }

            if (LinkWaypoint.Get_Role() != ECk_GroundNavPath_LinkWaypointRole::Exit)
            { continue; }

            // Newest open span of that id first: a route walks a link's two ends one after the other,
            // so an exit closes the last entry of its link that nothing has closed. An exit with no
            // open entry - which the stamp cannot produce, only a hand-written result can - names no
            // span and is left where it is rather than inventing one.
            for (auto Index = Spans.Num() - 1; Index >= 0; --Index)
            {
                auto& Span = Spans[Index];

                if (Span.Get_LinkId() != LinkWaypoint.Get_LinkId() ||
                    Span.Get_ExitWaypointIndex() != INDEX_NONE)
                { continue; }

                Span.Set_ExitWaypointIndex(LinkWaypoint.Get_WaypointIndex());
                Span.Set_ExitDistanceUu(LinkWaypoint.Get_DistanceFromStartUu());

                break;
            }
        }

        return Spans;
    }

    auto
        TryGet_NextLinkBeyond(
            const FCk_GroundNavPath_Result& InResult,
            float                           InDistanceUu)
        -> FCk_GroundNavPath_LinkSpan
    {
        for (const auto& Span : Get_LinksOnPath(InResult))
        {
            if (Span.Get_EntryDistanceUu() > InDistanceUu)
            { return Span; }
        }

        return {};
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GroundNavPath_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_GroundNavPath_ParamsData& InParams)
    -> FCk_Handle_GroundNavPath
{
    const auto HandleIsValid = ck::IsValid(InHandle);

    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Invalid Handle [{}] supplied to UCk_Utils_GroundNavPath_UE::Add"), InHandle)
    { return {}; }

    const auto FeatureIsAbsent = NOT Has(InHandle);

    CK_ENSURE_IF_NOT(FeatureIsAbsent,
        TEXT("Entity [{}] already has the GroundNavPath feature"), InHandle)
    { return Cast(InHandle); }

    InHandle.Add<ck::FFragment_GroundNavPath_Params>(InParams);
    InHandle.Add<ck::FFragment_GroundNavPath_Current>();
    InHandle.Add<ck::FFragment_GroundNavPath_Result>();

    ck::groundnav::Verbose(TEXT("GroundNav Path added to [{}] (agent radius [{}]uu)"),
        InHandle, InParams.Get_AgentRadiusUu());

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GroundNavPath_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_GroundNavPath_Params>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GroundNavPath_UE::
    Request_FindPath(
        FCk_Handle_GroundNavPath& InPath,
        const FCk_Request_GroundNavPath_FindPath& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_GroundNavPath
{
    CK_CALLSTACK_RECORD(ck::FFragment_GroundNavPath_Requests, InPath);

    const auto PathIsValid = ck::IsValid(InPath);

    CK_ENSURE_IF_NOT(PathIsValid,
        TEXT("Invalid GroundNav Path Handle [{}] supplied to Request_FindPath"), InPath)
    {
        InDelegate.ExecuteIfBound(InPath, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InPath;
    }

    // Refused here, at the same bound an AUTHORED multiplier is refused at, and for the same reason: a
    // link edge priced under its own span makes an edge cheaper than the distance it covers, which is
    // the one property the search's Euclidean heuristic is admissible under at w = 1. Hoisted out of
    // the ensure expression because a profile build compiles that expression out.
    auto EveryRewriteIsAdmissible = true;

    for (const auto& Rewrite : InRequest.Get_LinkCostMultipliers())
    {
        if (Rewrite.Value < 1.0f)
        {
            EveryRewriteIsAdmissible = false;
            break;
        }
    }

    CK_ENSURE_IF_NOT(EveryRewriteIsAdmissible,
        TEXT("GroundNav Path [{}] was asked to plan with a per-query link cost multiplier below 1.0. ")
        TEXT("Every multiplier in _LinkCostMultipliers must be at least 1.0"), InPath)
    {
        InDelegate.ExecuteIfBound(InPath, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InPath;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InPath.AddOrGet<ck::FFragment_GroundNavPath_Requests>()._Requests.Emplace(InRequest);

    return InPath;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GroundNavPath_UE::
    Request_AbandonPath(
        FCk_Handle_GroundNavPath& InPath,
        const FCk_Request_GroundNavPath_AbandonPath& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_GroundNavPath
{
    CK_CALLSTACK_RECORD(ck::FFragment_GroundNavPath_Requests, InPath);

    const auto PathIsValid = ck::IsValid(InPath);

    CK_ENSURE_IF_NOT(PathIsValid,
        TEXT("Invalid GroundNav Path Handle [{}] supplied to Request_AbandonPath"), InPath)
    {
        InDelegate.ExecuteIfBound(InPath, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InPath;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InPath.AddOrGet<ck::FFragment_GroundNavPath_Requests>()._Requests.Emplace(InRequest);

    return InPath;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GroundNavPath_UE::
    Get_Result(
        const FCk_Handle_GroundNavPath& InPath)
    -> FCk_GroundNavPath_Result
{
    if (ck::Is_NOT_Valid(InPath))
    { return {}; }

    return InPath.Get<ck::FFragment_GroundNavPath_Result>().Get_Result();
}

auto
    UCk_Utils_GroundNavPath_UE::
    Get_Status(
        const FCk_Handle_GroundNavPath& InPath)
    -> ECk_GroundNav_PathStatus
{
    if (ck::Is_NOT_Valid(InPath))
    { return ECk_GroundNav_PathStatus::InProgress; }

    const auto& Result = InPath.Get<ck::FFragment_GroundNavPath_Result>();

    return Result.Get_HasFreshResult()
        ? Result.Get_Result().Get_Status()
        : ECk_GroundNav_PathStatus::InProgress;
}

auto
    UCk_Utils_GroundNavPath_UE::
    Get_HasFreshResult(
        const FCk_Handle_GroundNavPath& InPath)
    -> bool
{
    if (ck::Is_NOT_Valid(InPath))
    { return false; }

    return InPath.Get<ck::FFragment_GroundNavPath_Result>().Get_HasFreshResult();
}

auto
    UCk_Utils_GroundNavPath_UE::
    Get_Diagnostics(
        const FCk_Handle_GroundNavPath& InPath)
    -> FFragment_GroundNavPath_Diagnostics
{
    if (ck::Is_NOT_Valid(InPath))
    { return {}; }

    if (NOT InPath.Has<FFragment_GroundNavPath_Diagnostics>())
    { return {}; }

    return InPath.Get<FFragment_GroundNavPath_Diagnostics>();
}

auto
    UCk_Utils_GroundNavPath_UE::
    Get_LastCorridorBounds(
        const FCk_Handle_GroundNavPath& InPath)
    -> FBox
{
    if (ck::Is_NOT_Valid(InPath))
    { return FBox{ForceInit}; }

    return InPath.Get<ck::FFragment_GroundNavPath_Current>().Get_LastCorridorBounds();
}

auto
    UCk_Utils_GroundNavPath_UE::
    Get_LinksOnPath(
        const FCk_Handle_GroundNavPath& InPath)
    -> TArray<FCk_GroundNavPath_LinkSpan>
{
    return ck::groundnav::Get_LinksOnPath(Get_Result(InPath));
}

auto
    UCk_Utils_GroundNavPath_UE::
    TryGet_NextLinkBeyond(
        const FCk_Handle_GroundNavPath& InPath,
        float                           InDistanceUu)
    -> FCk_GroundNavPath_LinkSpan
{
    return ck::groundnav::TryGet_NextLinkBeyond(Get_Result(InPath), InDistanceUu);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GroundNavPath_UE::
    BindTo_OnPathReady(
        FCk_Handle_GroundNavPath& InPath,
        const FCk_Delegate_GroundNavPath_OnPathReady& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_GroundNavPath
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnGroundNavPathReady, InPath, InDelegate,
        InBindingPolicy, InPostFireBehavior);

    return InPath;
}

auto
    UCk_Utils_GroundNavPath_UE::
    UnbindFrom_OnPathReady(
        FCk_Handle_GroundNavPath& InPath,
        const FCk_Delegate_GroundNavPath_OnPathReady& InDelegate)
    -> FCk_Handle_GroundNavPath
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGroundNavPathReady, InPath, InDelegate);

    return InPath;
}

auto
    UCk_Utils_GroundNavPath_UE::
    BindTo_OnPathFailed(
        FCk_Handle_GroundNavPath& InPath,
        const FCk_Delegate_GroundNavPath_OnPathFailed& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_GroundNavPath
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnGroundNavPathFailed, InPath, InDelegate,
        InBindingPolicy, InPostFireBehavior);

    return InPath;
}

auto
    UCk_Utils_GroundNavPath_UE::
    UnbindFrom_OnPathFailed(
        FCk_Handle_GroundNavPath& InPath,
        const FCk_Delegate_GroundNavPath_OnPathFailed& InDelegate)
    -> FCk_Handle_GroundNavPath
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGroundNavPathFailed, InPath, InDelegate);

    return InPath;
}

// --------------------------------------------------------------------------------------------------------------------

// The two REFLECTED casts. The C++ Cast/CastChecked templates beside them are unreachable from
// Blueprint and from AngelScript - both call a UFUNCTION by name - so a script that has a bare handle
// and wants a planner needs these. Written out rather than taken from
// CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE because that macro defines Has as well, and this class
// already has its own.

auto
    UCk_Utils_GroundNavPath_UE::
    DoCast(
        FCk_Handle&          InHandle,
        ECk_SucceededFailed& OutResult)
    -> FCk_Handle_GroundNavPath
{
    if (ck::Is_NOT_Valid(InHandle))
    {
        OutResult = ECk_SucceededFailed::Failed;
        return {};
    }

    if (NOT Has(InHandle))
    {
        OutResult = ECk_SucceededFailed::Failed;
        return {};
    }

    OutResult = ECk_SucceededFailed::Succeeded;
    return ck::StaticCast<FCk_Handle_GroundNavPath>(InHandle);
}

auto
    UCk_Utils_GroundNavPath_UE::
    DoCastChecked(
        FCk_Handle InHandle)
    -> FCk_Handle_GroundNavPath
{
    if (ck::Is_NOT_Valid(InHandle))
    { return {}; }

    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have a [{}]. Unable to convert Handle."),
        InHandle, ck::Get_RuntimeTypeToString<FCk_Handle_GroundNavPath>())
    { return {}; }

    return ck::StaticCast<FCk_Handle_GroundNavPath>(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------
