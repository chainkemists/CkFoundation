#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Utils.h"

#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Fragment.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_SurfaceMotion_UE, FCk_Handle_SurfaceMotion, ck::FFragment_SurfaceMotion);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SurfaceMotion_UE::
    Add(
        FCk_Handle_Transform& InBody,
        const FCk_SurfaceMotion_Spec& InParams)
    -> FCk_Handle_SurfaceMotion
{
    const auto BodyValid = ck::IsValid(InBody)
        && UCk_Utils_EntityLifetime_UE::Get_CanCreateEntity(InBody)
        && NOT Has(InBody);
    CK_ENSURE_IF_NOT(BodyValid,
        TEXT("Surface motion Add rejected body [{}]: it must be a live transform entity with no existing surface motion."),
        InBody)
    { return {}; }

    const auto ParamsValid = ck::IsValid(InParams);
    CK_ENSURE_IF_NOT(ParamsValid,
        TEXT("Surface motion Add rejected body [{}]: invalid clearance, probe reach, contact grace, speed, turn rate or gravity."),
        InBody)
    { return {}; }

    const auto Body = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InBody);
    auto MotionComp = ck::FFragment_SurfaceMotion{};
    MotionComp._SupportNormal = Body.GetRotation().GetAxisZ();
    MotionComp._TravelTangent = Body.GetRotation().GetAxisX();
    MotionComp._Direction = MotionComp._TravelTangent;

    InBody.Add<ck::FFragment_SurfaceMotion_Params>(InParams);
    InBody.Add<ck::FFragment_SurfaceMotion>(MoveTemp(MotionComp));

    return CastChecked(InBody);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SurfaceMotion_UE::
    Get_IsReady(
        const FCk_Handle_SurfaceMotion& InHandle)
    -> bool
{
    return ck::IsValid(InHandle) && Has(InHandle) && InHandle.Get<ck::FFragment_SurfaceMotion>()._Ready;
}

auto
    UCk_Utils_SurfaceMotion_UE::
    Get_IsGrounded(
        const FCk_Handle_SurfaceMotion& InHandle)
    -> bool
{
    return Get_IsReady(InHandle) && InHandle.Get<ck::FFragment_SurfaceMotion>()._Grounded;
}

auto
    UCk_Utils_SurfaceMotion_UE::
    Get_SupportNormal(
        const FCk_Handle_SurfaceMotion& InHandle)
    -> FVector
{
    return Get_IsReady(InHandle) ? InHandle.Get<ck::FFragment_SurfaceMotion>()._SupportNormal : FVector::UpVector;
}

auto
    UCk_Utils_SurfaceMotion_UE::
    Get_HasTrustedContact(
        const FCk_Handle_SurfaceMotion& InHandle)
    -> bool
{
    return Get_IsReady(InHandle) && InHandle.Get<ck::FFragment_SurfaceMotion>()._TrustedContact;
}

auto
    UCk_Utils_SurfaceMotion_UE::
    Get_Velocity(
        const FCk_Handle_SurfaceMotion& InHandle)
    -> FVector
{
    return Get_IsReady(InHandle) ? InHandle.Get<ck::FFragment_SurfaceMotion>()._Velocity : FVector::ZeroVector;
}

// --------------------------------------------------------------------------------------------------------------------

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

    const auto RequestValid = ck::IsValid(InHandle)
        && Has(InHandle)
        && NOT InHandle.Has<ck::FTag_DestroyEntity_Initiate>()
        && ck::IsValid(InRequest);
    CK_ENSURE_IF_NOT(RequestValid,
        TEXT("Surface motion Request_Steering rejected [{}]: the entity must be live with surface motion, and the request needs a "
             "finite direction, a finite speed >= 0 and a non-zero direction when moving."),
        InHandle)
    {
        InRequest.TryFireCompletion(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    InHandle.AddOrGet<ck::FFragment_SurfaceMotion_Requests>()._Requests.Emplace(MoveTemp(InRequest));

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------
