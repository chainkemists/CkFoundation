#include "CkLagCompProjectile_Utils.h"

#include "CkLagCompensation/LagCompProjectile/CkLagCompProjectile_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_LagCompProjectile_UE::
    Request_LaunchCompensated(
        FCk_Handle_BallisticMotion& InHandle,
        const FCk_Request_LagCompProjectile_LaunchCompensated& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_BallisticMotion
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_LagCompProjectile_Requests>().Update_Requests([&](auto& InContainer)
    {
        InContainer.Emplace(InRequest);
    });

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_LagCompProjectile_UE::
    BindTo_OnRewindHit(
        FCk_Handle_BallisticMotion& InHandle,
        const FCk_Delegate_LagCompProjectile_OnRewindHit& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_BallisticMotion
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_LagCompProjectile_OnRewindHit, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_LagCompProjectile_UE::
    UnbindFrom_OnRewindHit(
        FCk_Handle_BallisticMotion& InHandle,
        const FCk_Delegate_LagCompProjectile_OnRewindHit& InDelegate)
    -> FCk_Handle_BallisticMotion
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_LagCompProjectile_OnRewindHit, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_LagCompProjectile_UE::
    BindTo_OnLaunchCompensated(
        FCk_Handle_BallisticMotion& InHandle,
        const FCk_Delegate_LagCompProjectile_OnLaunchCompensated& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_BallisticMotion
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_LagCompProjectile_OnLaunchCompensated, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_LagCompProjectile_UE::
    UnbindFrom_OnLaunchCompensated(
        FCk_Handle_BallisticMotion& InHandle,
        const FCk_Delegate_LagCompProjectile_OnLaunchCompensated& InDelegate)
    -> FCk_Handle_BallisticMotion
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_LagCompProjectile_OnLaunchCompensated, InHandle, InDelegate);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------
