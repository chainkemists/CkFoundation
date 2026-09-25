#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Utils.h"
#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Fragment.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_SurfaceMotion_UE, FCk_Handle_SurfaceMotion,
    ck::FFragment_SurfaceMotion_Params, ck::FFragment_SurfaceMotion_Current);

auto
    UCk_Utils_SurfaceMotion_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_SurfaceMotion_ParamsData& InParams)
    -> FCk_Handle_SurfaceMotion
{
    const auto CompositionValid = ck::IsValid(InHandle) && UCk_Utils_EntityLifetime_UE::Get_CanCreateEntity(InHandle)
        && UCk_Utils_Transform_UE::Has(InHandle) && NOT Has(InHandle);
    CK_ENSURE_IF_NOT(CompositionValid, TEXT("Surface motion needs a live transform entity with no existing surface motion feature."))
    { return {}; }
    if (NOT CompositionValid)
    { return {}; }
    const auto Valid = FMath::IsFinite(InParams.Get_Clearance()) && InParams.Get_Clearance() > 0.0f
        && FMath::IsFinite(InParams.Get_ProbeReach()) && InParams.Get_ProbeReach() > InParams.Get_Clearance()
        && FMath::IsFinite(InParams.Get_MaxSpeed()) && InParams.Get_MaxSpeed() > 0.0f
        && FMath::IsFinite(InParams.Get_SurfaceTurnRate()) && InParams.Get_SurfaceTurnRate() > 0.0f
        && FMath::IsFinite(InParams.Get_ClearanceSpeed()) && InParams.Get_ClearanceSpeed() > 0.0f
        && FMath::IsFinite(InParams.Get_ContactGrace().Get_Seconds()) && InParams.Get_ContactGrace() >= FCk_Time{}
        && NOT InParams.Get_Gravity().ContainsNaN();
    CK_ENSURE_IF_NOT(Valid, TEXT("Surface motion admission rejected invalid clearance, speed, gravity or probe configuration."))
    { return {}; }
    if (NOT Valid)
    { return {}; }
    InHandle.Add<ck::FFragment_SurfaceMotion_Params>(InParams);
    auto Current = ck::FFragment_SurfaceMotion_Current{};
    const auto Body = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(UCk_Utils_Transform_UE::CastChecked(InHandle));
    Current._SupportNormal = Body.GetRotation().GetAxisZ();
    Current._TravelTangent = Body.GetRotation().GetAxisX();
    Current._Direction = Current._TravelTangent;
    InHandle.Add<ck::FFragment_SurfaceMotion_Current>(MoveTemp(Current));
    return CastChecked(InHandle);
}

auto
    UCk_Utils_SurfaceMotion_UE::
    Get_IsReady(
        const FCk_Handle_SurfaceMotion& InHandle)
    -> bool
{
    return ck::IsValid(InHandle) && Has(InHandle) && InHandle.Get<ck::FFragment_SurfaceMotion_Current>()._Ready;
}

auto
    UCk_Utils_SurfaceMotion_UE::
    Get_IsGrounded(
        const FCk_Handle_SurfaceMotion& InHandle)
    -> bool
{
    return Get_IsReady(InHandle) && InHandle.Get<ck::FFragment_SurfaceMotion_Current>()._Grounded;
}

auto
    UCk_Utils_SurfaceMotion_UE::
    Get_SupportNormal(
        const FCk_Handle_SurfaceMotion& InHandle)
    -> FVector
{
    return Get_IsReady(InHandle) ? InHandle.Get<ck::FFragment_SurfaceMotion_Current>()._SupportNormal : FVector::UpVector;
}

auto
    UCk_Utils_SurfaceMotion_UE::
    Get_HasTrustedContact(
        const FCk_Handle_SurfaceMotion& InHandle)
    -> bool
{
    return Get_IsReady(InHandle) && InHandle.Get<ck::FFragment_SurfaceMotion_Current>()._TrustedContact;
}

auto
    UCk_Utils_SurfaceMotion_UE::
    Get_Velocity(
        const FCk_Handle_SurfaceMotion& InHandle)
    -> FVector
{
    return Get_IsReady(InHandle) ? InHandle.Get<ck::FFragment_SurfaceMotion_Current>()._Velocity : FVector::ZeroVector;
}

auto
    UCk_Utils_SurfaceMotion_UE::
    Request_Steering(
        FCk_Handle_SurfaceMotion& InHandle,
        FCk_Request_SurfaceMotion_Steering InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_SurfaceMotion
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }
    const auto Valid = ck::IsValid(InHandle) && Has(InHandle)
        && NOT InHandle.Has<ck::FTag_DestroyEntity_Initiate>()
        && NOT InRequest.Get_WorldDirection().ContainsNaN()
        && FMath::IsFinite(InRequest.Get_Speed()) && InRequest.Get_Speed() >= 0.0f
        && (InRequest.Get_Speed() == 0.0f || NOT InRequest.Get_WorldDirection().IsNearlyZero());
    if (NOT Valid)
    {
        InRequest.TryFireCompletion(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }
    InHandle.AddOrGet<ck::FFragment_SurfaceMotion_Requests>()._Requests.Emplace(MoveTemp(InRequest));
    return InHandle;
}
