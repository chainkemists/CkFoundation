#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Utils.h"

#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Fragment.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_SurfaceMotion_UE, FCk_Handle_SurfaceMotion, ck::FFragment_SurfaceMotion);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_surface_motion_utils
{
    auto
        DoGet_ContactSource(
            ck::EProceduralSurfaceContactSource InSource)
        -> ECk_SurfaceMotion_ContactSource
    {
        switch (InSource)
        {
            case ck::EProceduralSurfaceContactSource::Forward:
            { return ECk_SurfaceMotion_ContactSource::Forward; }
            case ck::EProceduralSurfaceContactSource::Down:
            { return ECk_SurfaceMotion_ContactSource::Down; }
            case ck::EProceduralSurfaceContactSource::LookAhead:
            { return ECk_SurfaceMotion_ContactSource::LookAhead; }
            case ck::EProceduralSurfaceContactSource::Fan:
            { return ECk_SurfaceMotion_ContactSource::Fan; }
            case ck::EProceduralSurfaceContactSource::Fall:
            { return ECk_SurfaceMotion_ContactSource::Fall; }
            default:
            { return ECk_SurfaceMotion_ContactSource::None; }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SurfaceMotion_UE::
    Add(
        FCk_Handle_Transform& InBody,
        const FCk_SurfaceMotion_Spec& InParams)
    -> FCk_Handle_SurfaceMotion
{
    const auto BodyValid = ck::IsValid(InBody)
        && NOT InBody.Has<ck::FTag_DestroyEntity_Initiate>()
        && UCk_Utils_EntityLifetime_UE::Get_CanCreateEntity(InBody)
        && NOT Has(InBody);
    CK_ENSURE_IF_NOT(BodyValid,
        TEXT("Surface motion Add rejected body [{}]: it must be a live transform entity with no existing surface motion."),
        InBody)
    { return {}; }

    const auto ParamsValid = ck::IsValid(InParams);
    CK_ENSURE_IF_NOT(ParamsValid,
        TEXT("Surface motion Add rejected body [{}]: invalid clearance, probe reach, contact grace, confirm angle, confirm time, "
             "speed, turn rate, gravity or steer floor."),
        InBody)
    { return {}; }

    InBody.Add<ck::FFragment_SurfaceMotion_Params>(InParams);
    InBody.Add<ck::FFragment_SurfaceMotion>();
    InBody.Add<ck::FFragment_SurfaceMotion_Support>();
    InBody.Add<ck::FTag_SurfaceMotion_NeedsSetup>();

    return CastChecked(InBody);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SurfaceMotion_UE::
    Get_Status(
        const FCk_Handle_SurfaceMotion& InHandle)
    -> ECk_ProceduralAnimation_Status
{
    if (ck::Is_NOT_Valid(InHandle) || NOT Has(InHandle) || InHandle.Has<ck::FFragment_SurfaceMotion_Failure>())
    { return ECk_ProceduralAnimation_Status::Failed; }

    if (InHandle.Has<ck::FTag_SurfaceMotion_NeedsSetup>())
    { return ECk_ProceduralAnimation_Status::PendingSetup; }

    return ECk_ProceduralAnimation_Status::Ready;
}

auto
    UCk_Utils_SurfaceMotion_UE::
    Get_Failure(
        const FCk_Handle_SurfaceMotion& InHandle)
    -> ECk_SurfaceMotion_Failure
{
    return ck::IsValid(InHandle) && Has(InHandle) && InHandle.Has<ck::FFragment_SurfaceMotion_Failure>()
        ? InHandle.Get<ck::FFragment_SurfaceMotion_Failure>().Get_Reason()
        : ECk_SurfaceMotion_Failure::None;
}

auto
    UCk_Utils_SurfaceMotion_UE::
    Get_Support(
        const FCk_Handle_SurfaceMotion& InHandle)
    -> ECk_SurfaceMotion_Support
{
    return Get_Status(InHandle) == ECk_ProceduralAnimation_Status::Ready
        && InHandle.Get<ck::FFragment_SurfaceMotion_Support>()._State.Get_Grounded()
        ? ECk_SurfaceMotion_Support::Grounded
        : ECk_SurfaceMotion_Support::Airborne;
}

auto
    UCk_Utils_SurfaceMotion_UE::
    Get_SupportNormal(
        const FCk_Handle_SurfaceMotion& InHandle)
    -> FVector
{
    return Get_Status(InHandle) == ECk_ProceduralAnimation_Status::Ready
        ? InHandle.Get<ck::FFragment_SurfaceMotion_Support>()._State.Get_SupportNormal()
        : FVector::UpVector;
}

auto
    UCk_Utils_SurfaceMotion_UE::
    Get_ContactQuery(
        const FCk_Handle_SurfaceMotion& InHandle)
    -> ECk_SurfaceMotion_ContactQuery
{
    return Get_Status(InHandle) == ECk_ProceduralAnimation_Status::Ready
        && InHandle.Get<ck::FFragment_SurfaceMotion_Support>()._State.Get_ContactTrusted()
        ? ECk_SurfaceMotion_ContactQuery::Trusted
        : ECk_SurfaceMotion_ContactQuery::Missed;
}

auto
    UCk_Utils_SurfaceMotion_UE::
    Get_ContactSource(
        const FCk_Handle_SurfaceMotion& InHandle)
    -> ECk_SurfaceMotion_ContactSource
{
    return Get_Status(InHandle) == ECk_ProceduralAnimation_Status::Ready
        ? ck_surface_motion_utils::DoGet_ContactSource(InHandle.Get<ck::FFragment_SurfaceMotion_Support>()._State.Get_ContactSource())
        : ECk_SurfaceMotion_ContactSource::None;
}

auto
    UCk_Utils_SurfaceMotion_UE::
    Get_Velocity(
        const FCk_Handle_SurfaceMotion& InHandle)
    -> FVector
{
    return Get_Status(InHandle) == ECk_ProceduralAnimation_Status::Ready
        ? InHandle.Get<ck::FFragment_SurfaceMotion_Support>()._State.Get_Velocity()
        : FVector::ZeroVector;
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
