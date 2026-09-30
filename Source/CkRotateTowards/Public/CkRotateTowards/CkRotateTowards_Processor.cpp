#include "CkRotateTowards_Processor.h"
#include "CkRotateTowards/CkRotateTowards_Kernel.h"
#include "CkRotateTowards/CkRotateTowards_Log.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_RotateTowards_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_RotateTowards_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_RotateTowards_CancelPendingRequests);

auto
    ck::FProcessor_RotateTowards_HandleRequests::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        FFragment_RotateTowards_Tunables& InTunables,
        FFragment_RotateTowards& InRotateTowards,
        FFragment_RotateTowards_Requests& InRequests)
    -> void
{
    const auto RequestsCopy = InRequests._Requests;
    InRequests._Requests.Reset();
    algo::ForEachRequest(RequestsCopy, ck::Visitor([&](const auto& InRequest)
    {
        auto Result = ECk_Request_OperationResult::Failed;
        const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);
        if (InHandle.Has<FTag_DestroyEntity_Initiate>())
        {
            Result = ECk_Request_OperationResult::Failed_Cancelled;
            return;
        }
        rotate_towards::VeryVerbose(TEXT("Handling RotateTowards request on [{}]"), InHandle);
        Result = DoHandleRequest(InHandle, InTunables, InRotateTowards, InRequest);
    }), policy::DontResetContainer{});
    if (InRequests._Requests.IsEmpty())
    { InHandle.Remove<MarkedDirtyBy>(); }
}

auto
    ck::FProcessor_RotateTowards_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        FFragment_RotateTowards_Tunables& InTunables,
        FFragment_RotateTowards& InRotateTowards,
        const FCk_Request_RotateTowards_SetTarget& InRequest)
    -> ECk_Request_OperationResult
{
    const auto& Target = InRequest.Get_Target();
    if (Target == InRotateTowards._Target)
    { return ECk_Request_OperationResult::Succeeded; }

    // A target may be destroyed between enqueue and drain; that is not a caller error.
    if (ck::Is_NOT_Valid(Target))
    {
        rotate_towards::VeryVerbose(TEXT("SetTarget on [{}] found its target already gone"), InHandle);
        return ECk_Request_OperationResult::Failed;
    }

    const auto Previous = InRotateTowards._Target;
    InRotateTowards._Target = Target;
    InHandle.AddOrGet<FTag_RotateTowards_HasTarget>();
    InHandle.Try_Remove<FTag_RotateTowards_AtTarget>();
    InHandle.AddOrGet<FFragment_RotateTowards_SolveResult>() = {};
    UUtils_Signal_OnRotateTowardsTargetChanged::Broadcast(InHandle, MakePayload(InHandle, Target, Previous));
    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_RotateTowards_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        FFragment_RotateTowards_Tunables& InTunables,
        FFragment_RotateTowards& InRotateTowards,
        const FCk_Request_RotateTowards_ClearTarget& InRequest)
    -> ECk_Request_OperationResult
{
    if (ck::Is_NOT_Valid(InRotateTowards._Target) && NOT InHandle.Has<FTag_RotateTowards_HasTarget>())
    { return ECk_Request_OperationResult::Succeeded; }

    DoClearTarget(InHandle, InRotateTowards, ECk_RotateTowards_ClearReason::Requested);
    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_RotateTowards_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        FFragment_RotateTowards_Tunables& InTunables,
        FFragment_RotateTowards& InRotateTowards,
        const FCk_Request_RotateTowards_UpdateTunables& InRequest)
    -> ECk_Request_OperationResult
{
    const auto IsValidTunables = rotate_towards::Get_IsTunablesValid(InRequest.Get_Tunables());
    CK_ENSURE_IF_NOT(IsValidTunables, TEXT("RotateTowards UpdateTunables rejected invalid tunables on [{}]"), InHandle)
    { return ECk_Request_OperationResult::Failed; }

    InTunables = InRequest.Get_Tunables();
    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_RotateTowards_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        FFragment_RotateTowards_Tunables& InTunables,
        FFragment_RotateTowards& InRotateTowards,
        const FCk_Request_RotateTowards_SetRangeClamp& InRequest)
    -> ECk_Request_OperationResult
{
    const auto& RangeClamp = InRequest.Get_RangeClamp();

    const auto HasAnyRange = rotate_towards::Get_HasAnyRange(RangeClamp);
    CK_ENSURE_IF_NOT(HasAnyRange, TEXT("RotateTowards SetRangeClamp rejected [{}]: no axis enabled; use Request_ClearRangeClamp"), InHandle)
    { return ECk_Request_OperationResult::Failed; }

    const auto IsValidRangeClamp = rotate_towards::Get_IsRangeClampValid(RangeClamp);
    CK_ENSURE_IF_NOT(IsValidRangeClamp, TEXT("RotateTowards SetRangeClamp rejected invalid range clamp on [{}]"), InHandle)
    { return ECk_Request_OperationResult::Failed; }

    const auto IsValidRestReference = ck::IsValid(RangeClamp.Get_RestReferencePoint());
    CK_ENSURE_IF_NOT(IsValidRestReference, TEXT("RotateTowards SetRangeClamp rejected [{}]: a range clamp needs a valid rest reference point"), InHandle)
    { return ECk_Request_OperationResult::Failed; }

    InHandle.AddOrGet<FFragment_RotateTowards_RangeClamp>() = RangeClamp;
    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_RotateTowards_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        FFragment_RotateTowards_Tunables& InTunables,
        FFragment_RotateTowards& InRotateTowards,
        const FCk_Request_RotateTowards_ClearRangeClamp& InRequest)
    -> ECk_Request_OperationResult
{
    InHandle.Try_Remove<FFragment_RotateTowards_RangeClamp>();
    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_RotateTowards_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        FFragment_RotateTowards_Tunables& InTunables,
        FFragment_RotateTowards& InRotateTowards,
        const FCk_Request_RotateTowards_EnableDisable& InRequest)
    -> ECk_Request_OperationResult
{
    switch (InRequest.Get_EnableDisable())
    {
        case ECk_EnableDisable::Enable:
        {
            InHandle.Try_Remove<FTag_RotateTowards_Disabled>();
            break;
        }
        case ECk_EnableDisable::Disable:
        {
            InHandle.AddOrGet<FTag_RotateTowards_Disabled>();
            break;
        }
        default:
        {
            CK_INVALID_ENUM(InRequest.Get_EnableDisable());
            return ECk_Request_OperationResult::Failed;
        }
    }
    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_RotateTowards_HandleRequests::
    DoClearTarget(
        HandleType InHandle,
        FFragment_RotateTowards& InRotateTowards,
        ECk_RotateTowards_ClearReason InReason)
    -> void
{
    const auto Previous = InRotateTowards._Target;
    InRotateTowards._Target = {};
    InHandle.Try_Remove<FTag_RotateTowards_HasTarget>();
    InHandle.Try_Remove<FTag_RotateTowards_AtTarget>();
    InHandle.Try_Remove<FFragment_RotateTowards_SolveResult>();
    UUtils_Signal_OnRotateTowardsTargetCleared::Broadcast(InHandle, MakePayload(InHandle, Previous, InReason));
}

auto
    ck::FProcessor_RotateTowards_CancelPendingRequests::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        FFragment_RotateTowards_Requests& InRequests)
    -> void
{
    const auto RequestsCopy = InRequests._Requests;
    InRequests._Requests.Reset();
    request::FireCancelledForPending(InHandle, RequestsCopy);
}

auto
    ck::FProcessor_RotateTowards_Update::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_RotateTowards_Tunables& InTunables,
        FFragment_RotateTowards& InRotateTowards,
        FFragment_RotateTowards_SolveResult& InResult)
    -> void
{
    const auto Target = InRotateTowards.Get_Target();
    if (ck::Is_NOT_Valid(Target))
    {
        FProcessor_RotateTowards_HandleRequests::DoClearTarget(InHandle, InRotateTowards, ECk_RotateTowards_ClearReason::TargetLost);
        return;
    }

    auto Self = UCk_Utils_Transform_UE::CastChecked(InHandle);
    const auto Current = UCk_Utils_Transform_UE::Get_EntityCurrentRotation(Self);
    const auto From = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(Self);
    const auto To = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(Target);

    // The look direction is undefined for a coincident point: hold this frame and leave the AtTarget tag as is.
    const auto Hold = [&]()
    {
        InResult._DesiredRotation = Current;
        InResult._RemainingDelta = FRotator::ZeroRotator;
    };

    const auto LookAt = rotate_towards::Compute_LookAtRotation(From, To);
    if (NOT LookAt.IsSet())
    {
        Hold();
        return;
    }

    // Unset = hold this frame: falling back to unclamped motion at a coincident rest point could violate the range.
    const auto ClampToRange = [&](const FRotator& InDesired) -> TOptional<FRotator>
    {
        if (NOT InHandle.Has<FFragment_RotateTowards_RangeClamp>())
        { return InDesired; }

        const auto RangeClamp = InHandle.Get<FFragment_RotateTowards_RangeClamp>();
        const auto IsRestReferenceValid = ck::IsValid(RangeClamp.Get_RestReferencePoint());
        CK_ENSURE_IF_NOT(IsRestReferenceValid, TEXT("RotateTowards [{}] lost its rest reference point; range clamping removed"), InHandle)
        {
            InHandle.Remove<FFragment_RotateTowards_RangeClamp>();
            return InDesired;
        }

        const auto Rest = rotate_towards::Compute_LookAtRotation(From,
            UCk_Utils_Transform_UE::Get_EntityCurrentLocation(RangeClamp.Get_RestReferencePoint()));
        if (NOT Rest.IsSet())
        { return {}; }

        return rotate_towards::Apply_RangeClamp(InDesired, Rest.GetValue(), RangeClamp);
    };

    const auto MaybeDesired = ClampToRange(rotate_towards::Apply_AxisLocks(LookAt.GetValue(), Current, InTunables));
    if (NOT MaybeDesired.IsSet())
    {
        Hold();
        return;
    }
    const auto Desired = MaybeDesired.GetValue();

    const auto Dt = static_cast<float>(InDeltaT.Get_Seconds());
    const auto New = rotate_towards::Step_Rotation(Current, Desired, InTunables, Dt);

    InResult._DesiredRotation = Desired;
    InResult._RemainingDelta = rotate_towards::Compute_RemainingDelta(New, Desired);

    if (NOT New.Equals(Current, rotate_towards::kApplyEpsilonDeg))
    {
        DoApplyRotation(InHandle, Self, Current, New);
        InResult._LastAppliedRotation = New;
    }

    // Judged after the step, so the signal fires the frame the final step is enqueued.
    const auto WasAtTarget = InHandle.Has<FTag_RotateTowards_AtTarget>();
    const auto IsAtTarget = rotate_towards::Get_IsAtTarget(New, Desired, InTunables.Get_ReachedToleranceDeg());
    if (IsAtTarget && NOT WasAtTarget)
    {
        InHandle.AddOrGet<FTag_RotateTowards_AtTarget>();
        UUtils_Signal_OnRotateTowardsTargetReached::Broadcast(InHandle, MakePayload(InHandle, InRotateTowards.Get_Target()));
    }
    else if (NOT IsAtTarget && WasAtTarget)
    { InHandle.Remove<FTag_RotateTowards_AtTarget>(); }
}

auto
    ck::FProcessor_RotateTowards_Update::
    DoApplyRotation(
        HandleType InHandle,
        FCk_Handle_Transform& InTransform,
        const FRotator& InCurrentWorld,
        const FRotator& InNewWorld)
    -> void
{
    // World-rotation requests on a parent-driven scene node are rejected by the Transform feature; turn its offset
    // by the same world delta instead.
    if (UCk_Utils_SceneNode_UE::Has(InHandle))
    {
        auto Node = UCk_Utils_SceneNode_UE::CastChecked(InHandle);
        const auto Offset = UCk_Utils_SceneNode_UE::Get_Offset(Node);
        const auto NewOffsetRotation = rotate_towards::Compose_OffsetRotation(
            Offset.GetRotation(), InCurrentWorld.Quaternion(), InNewWorld.Quaternion());
        UCk_Utils_SceneNode_UE::Request_UpdateOffset(Node,
            FCk_Request_SceneNode_UpdateRelativeTransform{FTransform{NewOffsetRotation, Offset.GetLocation(), Offset.GetScale3D()}}, {});
        return;
    }

    UCk_Utils_Transform_UE::Request_SetRotation(InTransform, FCk_Request_Transform_SetRotation{InNewWorld}, {});
}
