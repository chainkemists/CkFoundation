#include "CkHoming_Utils.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkPhysics/Acceleration/CkAcceleration_Utils.h"
#include "CkPhysics/Velocity/CkVelocity_Utils.h"

#include "CkProjectile/Homing/CkHoming_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Homing_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Homing_ParamsData& InParams)
    -> FCk_Handle_Homing
{
    CK_ENSURE_IF_NOT(UCk_Utils_Velocity_UE::Has(InHandle) && UCk_Utils_Acceleration_UE::Has(InHandle),
        TEXT("Homing on [{}] requires Velocity AND Acceleration features — add the Projectile feature (or both) first"),
        InHandle)
    { return {}; }

    CK_ENSURE_IF_NOT(UCk_Utils_Transform_UE::Has(InHandle),
        TEXT("Homing on [{}] requires a Transform feature — there is no pursuer location without one"),
        InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_Homing_Params>(InParams);

    auto& Current = InHandle.Add<ck::FFragment_Homing_Current>();

    const auto AccelerationHandle = UCk_Utils_Acceleration_UE::CastChecked(InHandle);
    Current._BaseAcceleration = UCk_Utils_Acceleration_UE::Get_CurrentAcceleration(AccelerationHandle);

    const auto& GuidanceSettings = InParams.Get_GuidanceSettings();
    Current._ImpactTiming = GuidanceSettings.Get_ImpactTiming();
    Current._DesiredTimeToImpactRemaining = GuidanceSettings.Get_DesiredTimeToImpact();

    auto HomingHandle = Cast(InHandle);

    if (InParams.Get_StartingState() == ECk_EnableDisable::Disable)
    {
        HomingHandle.Add<ck::FTag_Homing_Disabled>();
    }

    return HomingHandle;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Homing_UE, FCk_Handle_Homing, ck::FFragment_Homing_Params, ck::FFragment_Homing_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Homing_UE::
    Request_SetTargetEntity(
        FCk_Handle_Homing& InHandle,
        const FCk_Request_Homing_SetTargetEntity& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Homing
{
    CK_CALLSTACK_RECORD(ck::FFragment_Homing_Requests, InHandle);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_Homing_Requests>()._Requests.Emplace(InRequest);

    return InHandle;
}

auto
    UCk_Utils_Homing_UE::
    Request_SetTargetLocation(
        FCk_Handle_Homing& InHandle,
        const FCk_Request_Homing_SetTargetLocation& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Homing
{
    CK_CALLSTACK_RECORD(ck::FFragment_Homing_Requests, InHandle);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_Homing_Requests>()._Requests.Emplace(InRequest);

    return InHandle;
}

auto
    UCk_Utils_Homing_UE::
    Request_ClearTarget(
        FCk_Handle_Homing& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Homing
{
    CK_CALLSTACK_RECORD(ck::FFragment_Homing_Requests, InHandle);

    const auto Request = FCk_Request_Homing_ClearTarget{};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_Homing_Requests>()._Requests.Emplace(Request);

    return InHandle;
}

auto
    UCk_Utils_Homing_UE::
    Request_SetDesiredTimeToImpact(
        FCk_Handle_Homing& InHandle,
        const FCk_Request_Homing_SetDesiredTimeToImpact& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Homing
{
    CK_CALLSTACK_RECORD(ck::FFragment_Homing_Requests, InHandle);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_Homing_Requests>()._Requests.Emplace(InRequest);

    return InHandle;
}

auto
    UCk_Utils_Homing_UE::
    Request_EnableDisable(
        FCk_Handle_Homing& InHandle,
        const FCk_Request_Homing_EnableDisable& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Homing
{
    CK_CALLSTACK_RECORD(ck::FFragment_Homing_Requests, InHandle);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_Homing_Requests>()._Requests.Emplace(InRequest);

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Homing_UE::
    Get_IsActive(
        const FCk_Handle_Homing& InHandle)
    -> bool
{
    return InHandle.Has<ck::FTag_Homing_Active>();
}

auto
    UCk_Utils_Homing_UE::
    Get_TargetMode(
        const FCk_Handle_Homing& InHandle)
    -> ECk_Homing_TargetMode
{
    return InHandle.Get<ck::FFragment_Homing_Current>().Get_TargetMode();
}

auto
    UCk_Utils_Homing_UE::
    Get_TargetEntity(
        const FCk_Handle_Homing& InHandle)
    -> FCk_Handle
{
    return InHandle.Get<ck::FFragment_Homing_Current>().Get_TargetEntity();
}

auto
    UCk_Utils_Homing_UE::
    Get_LastGuidanceState(
        const FCk_Handle_Homing& InHandle)
    -> FCk_Homing_GuidanceState
{
    return InHandle.Get<ck::FFragment_Homing_Current>().Get_LastGuidanceState();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Homing_UE::
    Get_FiringSolution(
        const FVector& InShooterLocation,
        const FVector& InShooterVelocity,
        const FVector& InTargetLocation,
        const FVector& InTargetVelocity,
        float InProjectileSpeed,
        ECk_Homing_InterceptPreference InPreference)
    -> FCk_Homing_FiringSolution
{
    return ck::pronav::Compute_FiringSolution(
        InShooterLocation, InShooterVelocity, InTargetLocation, InTargetVelocity, InProjectileSpeed, InPreference);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Homing_UE::
    BindTo_OnTargetMissed(
        FCk_Handle_Homing& InHandle,
        const FCk_Delegate_Homing_OnTargetMissed& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Homing
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_Homing_OnTargetMissed, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_Homing_UE::
    UnbindFrom_OnTargetMissed(
        FCk_Handle_Homing& InHandle,
        const FCk_Delegate_Homing_OnTargetMissed& InDelegate)
    -> FCk_Handle_Homing
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_Homing_OnTargetMissed, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_Homing_UE::
    BindTo_OnTargetLost(
        FCk_Handle_Homing& InHandle,
        const FCk_Delegate_Homing_OnTargetLost& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Homing
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_Homing_OnTargetLost, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_Homing_UE::
    UnbindFrom_OnTargetLost(
        FCk_Handle_Homing& InHandle,
        const FCk_Delegate_Homing_OnTargetLost& InDelegate)
    -> FCk_Handle_Homing
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_Homing_OnTargetLost, InHandle, InDelegate);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------
