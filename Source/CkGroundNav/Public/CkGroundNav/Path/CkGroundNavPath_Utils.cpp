#include "CkGroundNavPath_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkGroundNav/CkGroundNav_Log.h"

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
