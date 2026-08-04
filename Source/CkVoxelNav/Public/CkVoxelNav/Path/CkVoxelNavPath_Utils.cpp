#include "CkVoxelNavPath_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkVoxelNav/CkVoxelNav_Log.h"
#include "CkVoxelNav/Path/CkVoxelNav_Path_Graph.h"
#include "CkVoxelNav/Volume/CkVoxelNavVolume_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNavPath_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_VoxelNavPath_ParamsData& InParams)
    -> FCk_Handle_VoxelNavPath
{
    const auto HandleIsValid = ck::IsValid(InHandle);

    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Invalid Handle [{}] supplied to UCk_Utils_VoxelNavPath_UE::Add"), InHandle)
    {}

    if (NOT HandleIsValid)
    { return {}; }

    const auto FeatureIsAbsent = NOT Has(InHandle);

    CK_ENSURE_IF_NOT(FeatureIsAbsent,
        TEXT("Entity [{}] already has the VoxelNavPath feature"), InHandle)
    {}

    if (NOT FeatureIsAbsent)
    { return Cast(InHandle); }

    InHandle.Add<ck::FFragment_VoxelNavPath_Params>(InParams);
    InHandle.Add<ck::FFragment_VoxelNavPath_Result>();

    ck::voxelnav::Verbose(TEXT("VoxelNav Path added to [{}] (agent radius [{}]uu)"),
        InHandle, InParams.Get_AgentRadiusUu());

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNavPath_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_VoxelNavPath_Params>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNavPath_UE::
    Request_FindPath(
        FCk_Handle_VoxelNavPath& InPath,
        const FCk_Request_VoxelNavPath_FindPath& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VoxelNavPath
{
    CK_CALLSTACK_RECORD(ck::FFragment_VoxelNavPath_Requests, InPath);

    const auto PathIsValid = ck::IsValid(InPath);

    CK_ENSURE_IF_NOT(PathIsValid,
        TEXT("Invalid VoxelNav Path Handle [{}] supplied to Request_FindPath"), InPath)
    {}

    if (NOT PathIsValid)
    {
        InDelegate.ExecuteIfBound(InPath, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InPath;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InPath.AddOrGet<ck::FFragment_VoxelNavPath_Requests>()._Requests.Emplace(InRequest);

    return InPath;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNavPath_UE::
    Request_SetVolume(
        FCk_Handle_VoxelNavPath& InPath,
        const FCk_Handle_VoxelNavVolume& InVolume,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VoxelNavPath
{
    const auto PathIsValid = ck::IsValid(InPath);

    CK_ENSURE_IF_NOT(PathIsValid,
        TEXT("Invalid VoxelNav Path Handle [{}] supplied to Request_SetVolume"), InPath)
    {}

    if (NOT PathIsValid)
    {
        InDelegate.ExecuteIfBound(InPath, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InPath;
    }

    InPath.Get<ck::FFragment_VoxelNavPath_Params>().Set_Volume(InVolume);

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InPath, ECk_Request_OperationResult::Succeeded);

    return InPath;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNavPath_UE::
    Get_Status(
        const FCk_Handle_VoxelNavPath& InPath)
    -> ECk_VoxelNav_PathStatus
{
    if (ck::Is_NOT_Valid(InPath))
    { return ECk_VoxelNav_PathStatus::None; }

    const auto& Result = InPath.Get<ck::FFragment_VoxelNavPath_Result>();

    if (Result.Get_Status() != ECk_VoxelNav_PathStatus::Ready)
    { return Result.Get_Status(); }

    const auto Volume = Result.Get_Volume();

    // A volume that is gone makes no claim about the space this path crosses, which is the same position
    // a caller is in when the volume rebuilt underneath it.
    if (ck::Is_NOT_Valid(Volume) || NOT UCk_Utils_VoxelNavVolume_UE::Has(Volume))
    { return ECk_VoxelNav_PathStatus::Stale; }

    const auto CurrentEpoch = UCk_Utils_VoxelNavVolume_UE::Get_BuildEpoch(Volume);

    return ck::voxelnav::Get_IsPlannedEpochStale(Result.Get_PlannedAgainstEpoch(), CurrentEpoch)
        ? ECk_VoxelNav_PathStatus::Stale
        : ECk_VoxelNav_PathStatus::Ready;
}

auto
    UCk_Utils_VoxelNavPath_UE::
    Get_IsStale(
        const FCk_Handle_VoxelNavPath& InPath)
    -> bool
{
    return Get_Status(InPath) == ECk_VoxelNav_PathStatus::Stale;
}

auto
    UCk_Utils_VoxelNavPath_UE::
    Get_Waypoints(
        const FCk_Handle_VoxelNavPath& InPath)
    -> TArray<FVector>
{
    if (ck::Is_NOT_Valid(InPath))
    { return {}; }

    return InPath.Get<ck::FFragment_VoxelNavPath_Result>().Get_Waypoints();
}

auto
    UCk_Utils_VoxelNavPath_UE::
    Get_RawWaypointCount(
        const FCk_Handle_VoxelNavPath& InPath)
    -> int32
{
    if (ck::Is_NOT_Valid(InPath))
    { return 0; }

    return InPath.Get<ck::FFragment_VoxelNavPath_Result>().Get_RawWaypointCount();
}

auto
    UCk_Utils_VoxelNavPath_UE::
    Get_RefinedWaypointCount(
        const FCk_Handle_VoxelNavPath& InPath)
    -> int32
{
    if (ck::Is_NOT_Valid(InPath))
    { return 0; }

    return InPath.Get<ck::FFragment_VoxelNavPath_Result>().Get_Waypoints().Num();
}

auto
    UCk_Utils_VoxelNavPath_UE::
    Get_PathLengthUu(
        const FCk_Handle_VoxelNavPath& InPath)
    -> float
{
    if (ck::Is_NOT_Valid(InPath))
    { return 0.0f; }

    return InPath.Get<ck::FFragment_VoxelNavPath_Result>().Get_PathLengthUu();
}

auto
    UCk_Utils_VoxelNavPath_UE::
    Get_LastSearchOutcome(
        const FCk_Handle_VoxelNavPath& InPath)
    -> ECk_VoxelNav_PathSearchOutcome
{
    if (ck::Is_NOT_Valid(InPath))
    { return ECk_VoxelNav_PathSearchOutcome::InvalidRequest; }

    return InPath.Get<ck::FFragment_VoxelNavPath_Result>().Get_Outcome();
}

auto
    UCk_Utils_VoxelNavPath_UE::
    Get_PlannedAgainstEpoch(
        const FCk_Handle_VoxelNavPath& InPath)
    -> int32
{
    if (ck::Is_NOT_Valid(InPath))
    { return 0; }

    return InPath.Get<ck::FFragment_VoxelNavPath_Result>().Get_PlannedAgainstEpoch();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNavPath_UE::
    BindTo_OnPathReady(
        FCk_Handle_VoxelNavPath& InPath,
        const FCk_Delegate_VoxelNavPath_OnPathReady& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_VoxelNavPath
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnVoxelNavPathReady, InPath, InDelegate,
        InBindingPolicy, InPostFireBehavior);

    return InPath;
}

auto
    UCk_Utils_VoxelNavPath_UE::
    UnbindFrom_OnPathReady(
        FCk_Handle_VoxelNavPath& InPath,
        const FCk_Delegate_VoxelNavPath_OnPathReady& InDelegate)
    -> FCk_Handle_VoxelNavPath
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnVoxelNavPathReady, InPath, InDelegate);

    return InPath;
}

auto
    UCk_Utils_VoxelNavPath_UE::
    BindTo_OnPathFailed(
        FCk_Handle_VoxelNavPath& InPath,
        const FCk_Delegate_VoxelNavPath_OnPathFailed& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_VoxelNavPath
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnVoxelNavPathFailed, InPath, InDelegate,
        InBindingPolicy, InPostFireBehavior);

    return InPath;
}

auto
    UCk_Utils_VoxelNavPath_UE::
    UnbindFrom_OnPathFailed(
        FCk_Handle_VoxelNavPath& InPath,
        const FCk_Delegate_VoxelNavPath_OnPathFailed& InDelegate)
    -> FCk_Handle_VoxelNavPath
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnVoxelNavPathFailed, InPath, InDelegate);

    return InPath;
}

// --------------------------------------------------------------------------------------------------------------------
