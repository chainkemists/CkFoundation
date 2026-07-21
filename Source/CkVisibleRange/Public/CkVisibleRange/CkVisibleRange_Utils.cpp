#include "CkVisibleRange_Utils.h"

#include "CkCore/Chrono/CkChrono.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VisibleRange_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_VisibleRange_ParamsData& InParams)
    -> FCk_Handle_VisibleRange
{
    InHandle.Add<ck::FFragment_VisibleRange_Params>(InParams);

    // Chrono seeded as already-Done (not fresh/zero) so the FIRST Update tick always evaluates the
    // just-supplied distance immediately, regardless of _UpdateInterval — matching the proven
    // Compass/Minimap precedent (their Setup processor force-primes _TimeSinceUpdate the same way).
    // A fresh Chrono would leave the entity showing the default (visible) state for up to one full
    // interval before its true range state is ever computed.
    auto InitialChrono = FCk_Chrono{InParams.Get_UpdateInterval()};
    InitialChrono.Complete();
    InHandle.Add<ck::FFragment_VisibleRange_Current>(InitialChrono);

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_VisibleRange_UE, FCk_Handle_VisibleRange, ck::FFragment_VisibleRange_Current, ck::FFragment_VisibleRange_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VisibleRange_UE::
    Get_MinRange(
        const FCk_Handle_VisibleRange& InHandle)
    -> float
{
    return InHandle.Get<ck::FFragment_VisibleRange_Params>().Get_MinRange();
}

auto
    UCk_Utils_VisibleRange_UE::
    Get_MaxRange(
        const FCk_Handle_VisibleRange& InHandle)
    -> float
{
    return InHandle.Get<ck::FFragment_VisibleRange_Params>().Get_MaxRange();
}

auto
    UCk_Utils_VisibleRange_UE::
    Get_FadeBandCm(
        const FCk_Handle_VisibleRange& InHandle)
    -> float
{
    return InHandle.Get<ck::FFragment_VisibleRange_Params>().Get_FadeBandCm();
}

auto
    UCk_Utils_VisibleRange_UE::
    Get_FadeAlpha(
        const FCk_Handle_VisibleRange& InHandle)
    -> float
{
    return InHandle.Get<ck::FFragment_VisibleRange_Current>().Get_FadeAlpha();
}

auto
    UCk_Utils_VisibleRange_UE::
    Get_IsHidden(
        const FCk_Handle_VisibleRange& InHandle)
    -> bool
{
    return InHandle.Has<ck::FTag_VisibleRange_Hidden>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VisibleRange_UE::
    Update_Distance(
        FCk_Handle_VisibleRange& InHandle,
        float InDistance)
    -> FCk_Handle_VisibleRange
{
    InHandle.Get<ck::FFragment_VisibleRange_Current>()._Distance = InDistance;

    return InHandle;
}

auto
    UCk_Utils_VisibleRange_UE::
    Request_SetVisibility(
        FCk_Handle_VisibleRange& InHandle,
        ECk_VisibleRange_ShowHide InShowHide)
    -> FCk_Handle_VisibleRange
{
    CK_CALLSTACK_RECORD(ck::FFragment_VisibleRange_Requests, InHandle);

    InHandle.AddOrGet<ck::FFragment_VisibleRange_Requests>()._Requests.Emplace(
        FCk_Request_VisibleRange_SetVisibility{InShowHide});

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VisibleRange_UE::
    Compute_IsInRange(
        float InMinRange,
        float InMaxRange,
        float InDistance)
    -> bool
{
    if (InMinRange > 0.0f && InDistance < InMinRange)
    { return false; }

    if (InMaxRange > 0.0f && InDistance > InMaxRange)
    { return false; }

    return true;
}

auto
    UCk_Utils_VisibleRange_UE::
    Compute_FadeAlpha(
        float InMinRange,
        float InMaxRange,
        float InFadeBandCm,
        float InDistance)
    -> float
{
    if (NOT Compute_IsInRange(InMinRange, InMaxRange, InDistance))
    { return 0.0f; }

    if (InFadeBandCm <= 0.0f)
    { return 1.0f; }

    auto Alpha = 1.0f;

    if (InMinRange > 0.0f && InDistance < InMinRange + InFadeBandCm)
    { Alpha = FMath::Min(Alpha, (InDistance - InMinRange) / InFadeBandCm); }

    if (InMaxRange > 0.0f && InDistance > InMaxRange - InFadeBandCm)
    { Alpha = FMath::Min(Alpha, (InMaxRange - InDistance) / InFadeBandCm); }

    return FMath::Clamp(Alpha, 0.0f, 1.0f);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VisibleRange_UE::
    BindTo_OnHiddenChanged(
        FCk_Handle_VisibleRange& InHandle,
        const FCk_Delegate_VisibleRange_HiddenChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_VisibleRange
{
    ck::UUtils_Signal_OnVisibleRange_HiddenChanged::Bind(InHandle, InDelegate, InBindingPolicy);

    return InHandle;
}

auto
    UCk_Utils_VisibleRange_UE::
    UnbindFrom_OnHiddenChanged(
        FCk_Handle_VisibleRange& InHandle,
        const FCk_Delegate_VisibleRange_HiddenChanged& InDelegate)
    -> FCk_Handle_VisibleRange
{
    ck::UUtils_Signal_OnVisibleRange_HiddenChanged::Unbind(InHandle, InDelegate);

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------
