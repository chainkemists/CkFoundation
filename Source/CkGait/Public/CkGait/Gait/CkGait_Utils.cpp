#include "CkGait_Utils.h"

#include "CkGait/Gait/CkGait_Kernel.h"
#include "CkGait/CkGait_Log.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Gait_UE, FCk_Handle_Gait, ck::FFragment_Gait, ck::FFragment_Gait_Tunables);

auto
    UCk_Utils_Gait_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Gait_Spec& InSpec)
    -> FCk_Handle_Gait
{
    const auto IsValidHandle = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(IsValidHandle, TEXT("Gait Add rejected invalid handle [{}]"), InHandle)
    { return {}; }

    const auto IsAlreadyGait = Has(InHandle);
    CK_ENSURE_IF_NOT(NOT IsAlreadyGait, TEXT("Gait Add rejected [{}]: already a gait"), InHandle)
    { return {}; }

    const auto AreTunablesValid = ck::gait::Get_AreTunablesValid(InSpec);
    CK_ENSURE_IF_NOT(AreTunablesValid, TEXT("Gait Add rejected invalid spec on [{}]"), InHandle)
    { return {}; }

    const auto IsMovementComponentValid = InSpec.Get_MovementComponent().IsValid();
    CK_ENSURE_IF_NOT(IsMovementComponentValid, TEXT("Gait Add rejected [{}]: the spec's movement component is not valid"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_Gait_Tunables>(InSpec);
    InHandle.Add<ck::FFragment_Gait>();
    if (InSpec.Get_StartingState() == ECk_EnableDisable::Disable)
    { InHandle.Add<ck::FTag_Gait_Disabled>(); }

    return CastChecked(InHandle);
}

auto
    UCk_Utils_Gait_UE::
    Get_Spec(
        const FCk_Handle_Gait& InGait)
    -> FCk_Gait_Spec
{
    return InGait.Get<ck::FFragment_Gait_Tunables>();
}

auto
    UCk_Utils_Gait_UE::
    Get_IsEnabled(
        const FCk_Handle_Gait& InGait)
    -> bool
{
    return NOT InGait.Has<ck::FTag_Gait_Disabled>();
}

auto
    UCk_Utils_Gait_UE::
    Get_Phase(
        const FCk_Handle_Gait& InGait)
    -> float
{
    return InGait.Get<ck::FFragment_Gait>().Get_Clock()._Phase;
}

auto
    UCk_Utils_Gait_UE::
    Get_Amount(
        const FCk_Handle_Gait& InGait)
    -> float
{
    return InGait.Get<ck::FFragment_Gait>().Get_Clock()._Amount;
}

auto
    UCk_Utils_Gait_UE::
    Get_SpeedRatio(
        const FCk_Handle_Gait& InGait)
    -> float
{
    return InGait.Get<ck::FFragment_Gait>().Get_Clock()._SpeedRatio;
}

auto
    UCk_Utils_Gait_UE::
    Get_BreathPhase(
        const FCk_Handle_Gait& InGait)
    -> float
{
    return InGait.Get<ck::FFragment_Gait>().Get_Clock()._BreathPhase;
}

auto
    UCk_Utils_Gait_UE::
    Get_LastMotion(
        const FCk_Handle_Gait& InGait)
    -> FCk_Gait_Motion
{
    return InGait.Get<ck::FFragment_Gait>().Get_LastMotion();
}

auto
    UCk_Utils_Gait_UE::
    Get_LandingCount(
        const FCk_Handle_Gait& InGait)
    -> int32
{
    return InGait.Get<ck::FFragment_Gait>().Get_LandingCount();
}

auto
    UCk_Utils_Gait_UE::
    Get_LastLandImpactSpeed(
        const FCk_Handle_Gait& InGait)
    -> float
{
    return InGait.Get<ck::FFragment_Gait>().Get_LastLandImpactSpeed();
}

auto
    UCk_Utils_Gait_UE::
    Request_UpdateSpec(
        FCk_Handle_Gait& InGait,
        const FCk_Request_Gait_UpdateSpec& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Gait
{
    if (ck::IsValid(InGait) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InGait,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InGait, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InGait;
    }

    const auto IsValidGait = ck::IsValid(InGait) && Has(InGait);
    CK_ENSURE_IF_NOT(IsValidGait, TEXT("Gait UpdateSpec rejected invalid gait [{}]"), InGait)
    {
        InDelegate.ExecuteIfBound(InGait, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InGait;
    }

    const auto IsValidSpec = ck::gait::Get_IsSpecValid(InRequest.Get_Spec());
    CK_ENSURE_IF_NOT(IsValidSpec, TEXT("Gait UpdateSpec rejected invalid spec on [{}]"), InGait)
    {
        InDelegate.ExecuteIfBound(InGait, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InGait;
    }

    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_Gait_Requests, InGait);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InGait.AddOrGet<ck::FFragment_Gait_Requests>()._Requests.Emplace(Request);
    return InGait;
}

auto
    UCk_Utils_Gait_UE::
    Request_EnableDisable(
        FCk_Handle_Gait& InGait,
        const FCk_Request_Gait_EnableDisable& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Gait
{
    if (ck::IsValid(InGait) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InGait,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InGait, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InGait;
    }

    const auto IsValidGait = ck::IsValid(InGait) && Has(InGait);
    CK_ENSURE_IF_NOT(IsValidGait, TEXT("Gait EnableDisable rejected invalid gait [{}]"), InGait)
    {
        InDelegate.ExecuteIfBound(InGait, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InGait;
    }

    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_Gait_Requests, InGait);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InGait.AddOrGet<ck::FFragment_Gait_Requests>()._Requests.Emplace(Request);
    return InGait;
}

auto
    UCk_Utils_Gait_UE::
    Request_Reset(
        FCk_Handle_Gait& InGait,
        const FCk_Request_Gait_Reset& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Gait
{
    if (ck::IsValid(InGait) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InGait,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InGait, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InGait;
    }

    const auto IsValidGait = ck::IsValid(InGait) && Has(InGait);
    CK_ENSURE_IF_NOT(IsValidGait, TEXT("Gait Reset rejected invalid gait [{}]"), InGait)
    {
        InDelegate.ExecuteIfBound(InGait, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InGait;
    }

    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_Gait_Requests, InGait);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InGait.AddOrGet<ck::FFragment_Gait_Requests>()._Requests.Emplace(Request);
    return InGait;
}
