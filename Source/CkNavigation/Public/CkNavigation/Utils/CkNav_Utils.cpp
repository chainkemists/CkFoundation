#include "CkNav_Utils.h"

#include "CkNavigation/CkNavigation_Log.h"
#include "CkNavigation/Nav/CkNav_Fragment.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

// (Engine includes for runtime-nav-setup helper removed — see comment near end of file.)

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Nav_UE, FCk_Handle_NavAgent, ck::FFragment_Nav_AgentParams);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Nav_UE::
    Add(
        FCk_Handle_Transform& InHandle,
        const FCk_Nav_AgentParams& InParams)
    -> FCk_Handle_NavAgent
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("UCk_Utils_Nav_UE::Add called on invalid FCk_Handle_Transform"))
    { return {}; }

    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_HasAuthority(InHandle),
        TEXT("UCk_Utils_Nav_UE::Add: no authority over [{}]"), InHandle)
    { return {}; }

    // P3-7: guard against double-add. Mutating params on a registered agent requires
    // Request_AgentParamsUpdated (v1.1, not yet implemented).
    if (Has(InHandle))
    {
        ck::nav::Warning(TEXT("Nav feature already added to [{}]; ignoring duplicate Add."), InHandle);
        return DoCastChecked(InHandle);
    }

    InHandle.Add<ck::FFragment_Nav_AgentParams>(InParams);
    InHandle.Add<ck::FFragment_Nav_PathResult>();
    InHandle.Add<ck::FFragment_Nav_CrowdVelocity>();
    InHandle.Add<ck::FTag_Nav_NeedsSetup>();
    // FFragment_Nav_CrowdAgent and FTag_Nav_CrowdRegistered are added next frame by CrowdSetup.

    return DoCastChecked(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Nav_UE::
    Request_FindPath(
        FCk_Handle_NavAgent& InHandle,
        const FCk_Request_Nav_FindPath& InRequest)
    -> FCk_Handle_NavAgent
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("UCk_Utils_Nav_UE::Request_FindPath called on invalid nav agent"))
    { return {}; }

    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_HasAuthority(InHandle),
        TEXT("UCk_Utils_Nav_UE::Request_FindPath: no authority over [{}]"), InHandle)
    { return InHandle; }

    InHandle.AddOrGet<ck::FFragment_Nav_Requests>().Update_Requests(
        [&](auto& InContainer)
        {
            InContainer.Emplace(InRequest);
        });

    InHandle.AddOrGet<ck::FTag_Nav_PathPending>();

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Nav_UE::
    BindTo_OnPathReady(
        FCk_Handle_NavAgent& InHandle,
        const FCk_Delegate_Nav_OnPathReady& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_NavAgent
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnNavPathReady, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_Nav_UE::
    UnbindFrom_OnPathReady(
        FCk_Handle_NavAgent& InHandle,
        const FCk_Delegate_Nav_OnPathReady& InDelegate)
    -> FCk_Handle_NavAgent
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnNavPathReady, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_Nav_UE::
    BindTo_OnPathFailed(
        FCk_Handle_NavAgent& InHandle,
        const FCk_Delegate_Nav_OnPathFailed& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_NavAgent
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnNavPathFailed, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_Nav_UE::
    UnbindFrom_OnPathFailed(
        FCk_Handle_NavAgent& InHandle,
        const FCk_Delegate_Nav_OnPathFailed& InDelegate)
    -> FCk_Handle_NavAgent
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnNavPathFailed, InHandle, InDelegate);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Nav_UE::
    Get_PathStatus(
        const FCk_Handle_NavAgent& InHandle)
    -> ECk_Nav_PathStatus
{
    if (ck::Is_NOT_Valid(InHandle))
    { return ECk_Nav_PathStatus::None; }

    if (NOT InHandle.Has<ck::FFragment_Nav_PathResult>())
    { return ECk_Nav_PathStatus::None; }

    return InHandle.Get<ck::FFragment_Nav_PathResult>().Get_Status();
}

auto
    UCk_Utils_Nav_UE::
    Get_Waypoints(
        const FCk_Handle_NavAgent& InHandle)
    -> TArray<FVector>
{
    if (ck::Is_NOT_Valid(InHandle))
    { return {}; }

    if (NOT InHandle.Has<ck::FFragment_Nav_PathResult>())
    { return {}; }

    return InHandle.Get<ck::FFragment_Nav_PathResult>().Get_Waypoints();
}

auto
    UCk_Utils_Nav_UE::
    Get_HasPath(
        const FCk_Handle_NavAgent& InHandle)
    -> bool
{
    const auto Status = Get_PathStatus(InHandle);
    return Status == ECk_Nav_PathStatus::Ready || Status == ECk_Nav_PathStatus::Partial;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Nav_UE::
    Get_CurrentVelocity(
        const FCk_Handle_NavAgent& InHandle)
    -> FVector
{
    if (ck::Is_NOT_Valid(InHandle))
    { return FVector::ZeroVector; }

    if (NOT InHandle.Has<ck::FFragment_Nav_CrowdVelocity>())
    { return FVector::ZeroVector; }

    return InHandle.Get<ck::FFragment_Nav_CrowdVelocity>().Get_Velocity();
}

auto
    UCk_Utils_Nav_UE::
    Get_DesiredVelocity(
        const FCk_Handle_NavAgent& InHandle)
    -> FVector
{
    if (ck::Is_NOT_Valid(InHandle))
    { return FVector::ZeroVector; }

    if (NOT InHandle.Has<ck::FFragment_Nav_CrowdVelocity>())
    { return FVector::ZeroVector; }

    return InHandle.Get<ck::FFragment_Nav_CrowdVelocity>().Get_DesiredVelocity();
}

auto
    UCk_Utils_Nav_UE::
    Get_IsCrowdRegistered(
        const FCk_Handle_NavAgent& InHandle)
    -> bool
{
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }

    return InHandle.Has<ck::FTag_Nav_CrowdRegistered>();
}

// --------------------------------------------------------------------------------------------------------------------

// Runtime nav-bounds setup intentionally REMOVED. Multiple attempts (volume spawn, direct
// AddNavigationBounds, PerformNavigationBoundsUpdate dance) all produced 0-sized navmeshes
// because UE's runtime nav generation is designed for incremental updates to a PRE-BAKED
// navmesh, not for baking from zero in a Game-mode world.
//
// The supported workflow is: place ANavMeshBoundsVolume in the level at edit-time, bake nav,
// save the level. After that, runtime path queries work via the normal CkNavigation pipeline.
//
// If a future need arises for runtime nav setup (multiplayer dynamic level streaming etc.),
// the right approach is FNavigationInvoker + GenerateNavigationOnlyAroundNavigationInvokers.

// --------------------------------------------------------------------------------------------------------------------
