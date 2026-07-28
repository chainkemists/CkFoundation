#include "CkBallisticMotion_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkProjectile/BallisticMotion/CkBallisticMotion_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_BallisticMotion_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_BallisticMotion_ParamsData& InParams)
    -> FCk_Handle_BallisticMotion
{
    CK_ENSURE_IF_NOT(UCk_Utils_Transform_UE::Has(InHandle),
        TEXT("Cannot Add BallisticMotion to Entity [{}] because it does NOT have a Transform"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_BallisticMotion_Params>(InParams);
    InHandle.Add<ck::FFragment_BallisticMotion_Current>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_BallisticMotion_UE, FCk_Handle_BallisticMotion,
    ck::FFragment_BallisticMotion_Params, ck::FFragment_BallisticMotion_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_BallisticMotion_UE::
    Get_IsInFlight(
        const FCk_Handle_BallisticMotion& InHandle)
    -> bool
{
    return InHandle.Has<ck::FTag_BallisticMotion_Active>();
}

auto
    UCk_Utils_BallisticMotion_UE::
    Get_CurrentVelocity(
        const FCk_Handle_BallisticMotion& InHandle)
    -> FVector
{
    return InHandle.Get<ck::FFragment_BallisticMotion_Current>().Get_CurrentVelocity();
}

auto
    UCk_Utils_BallisticMotion_UE::
    Get_InitialConditions(
        const FCk_Handle_BallisticMotion& InHandle)
    -> FCk_Ballistic_InitialConditions
{
    return InHandle.Get<ck::FFragment_BallisticMotion_Current>().Get_InitialConditions();
}

auto
    UCk_Utils_BallisticMotion_UE::
    Get_TrajectorySegmentIndex(
        const FCk_Handle_BallisticMotion& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_BallisticMotion_Current>().Get_TrajectorySegmentIndex();
}

auto
    UCk_Utils_BallisticMotion_UE::
    Get_TrajectoryParams(
        const FCk_Handle_BallisticMotion& InHandle)
    -> FCk_Ballistic_TrajectoryParams
{
    return InHandle.Get<ck::FFragment_BallisticMotion_Params>().Get_TrajectoryParams();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_BallisticMotion_UE::
    Request_Launch(
        FCk_Handle_BallisticMotion& InHandle,
        const FCk_Request_BallisticMotion_Launch& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_BallisticMotion
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_BallisticMotion_Requests>().Update_Requests([&](auto& InContainer)
    {
        InContainer.Emplace(InRequest);
    });

    return InHandle;
}

auto
    UCk_Utils_BallisticMotion_UE::
    Request_Stop(
        FCk_Handle_BallisticMotion& InHandle,
        const FCk_Request_BallisticMotion_Stop& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_BallisticMotion
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_BallisticMotion_Requests>().Update_Requests([&](auto& InContainer)
    {
        InContainer.Emplace(InRequest);
    });

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_BallisticMotion_UE::
    BindTo_OnImpact(
        FCk_Handle_BallisticMotion& InHandle,
        const FCk_Delegate_BallisticMotion_OnImpact& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_BallisticMotion
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_BallisticMotion_OnImpact, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_BallisticMotion_UE::
    UnbindFrom_OnImpact(
        FCk_Handle_BallisticMotion& InHandle,
        const FCk_Delegate_BallisticMotion_OnImpact& InDelegate)
    -> FCk_Handle_BallisticMotion
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_BallisticMotion_OnImpact, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_BallisticMotion_UE::
    BindTo_OnTrajectoryChanged(
        FCk_Handle_BallisticMotion& InHandle,
        const FCk_Delegate_BallisticMotion_OnTrajectoryChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_BallisticMotion
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_BallisticMotion_OnTrajectoryChanged, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_BallisticMotion_UE::
    UnbindFrom_OnTrajectoryChanged(
        FCk_Handle_BallisticMotion& InHandle,
        const FCk_Delegate_BallisticMotion_OnTrajectoryChanged& InDelegate)
    -> FCk_Handle_BallisticMotion
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_BallisticMotion_OnTrajectoryChanged, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_BallisticMotion_UE::
    BindTo_OnStopped(
        FCk_Handle_BallisticMotion& InHandle,
        const FCk_Delegate_BallisticMotion_OnStopped& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_BallisticMotion
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_BallisticMotion_OnStopped, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_BallisticMotion_UE::
    UnbindFrom_OnStopped(
        FCk_Handle_BallisticMotion& InHandle,
        const FCk_Delegate_BallisticMotion_OnStopped& InDelegate)
    -> FCk_Handle_BallisticMotion
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_BallisticMotion_OnStopped, InHandle, InDelegate);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------
