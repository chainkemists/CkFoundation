#include "CkRotateTowards_Utils.h"

#include "CkRotateTowards/CkRotateTowards_Kernel.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_RotateTowards_UE, FCk_Handle_RotateTowards, ck::FFragment_RotateTowards, ck::FFragment_RotateTowards_Tunables);

auto
    UCk_Utils_RotateTowards_UE::
    Add(
        FCk_Handle_Transform& InHandle,
        const FCk_RotateTowards_Spec& InSpec)
    -> FCk_Handle_RotateTowards
{
    const auto IsValidTransform = ck::IsValid(InHandle) && UCk_Utils_Transform_UE::Has(InHandle);
    CK_ENSURE_IF_NOT(IsValidTransform, TEXT("RotateTowards Add rejected invalid transform [{}]"), InHandle)
    { return {}; }

    const auto IsAlreadyRotatingTowards = Has(InHandle);
    CK_ENSURE_IF_NOT(NOT IsAlreadyRotatingTowards, TEXT("RotateTowards Add rejected [{}]: already rotates towards a target"), InHandle)
    { return {}; }

    const auto IsValidTunables = ck::rotate_towards::Get_IsTunablesValid(InSpec.Get_Tunables());
    CK_ENSURE_IF_NOT(IsValidTunables, TEXT("RotateTowards Add rejected invalid tunables on [{}]"), InHandle)
    { return {}; }

    const auto IsValidRangeClamp = ck::rotate_towards::Get_IsRangeClampValid(InSpec.Get_RangeClamp());
    CK_ENSURE_IF_NOT(IsValidRangeClamp, TEXT("RotateTowards Add rejected invalid range clamp on [{}]"), InHandle)
    { return {}; }

    const auto HasAnyRange = ck::rotate_towards::Get_HasAnyRange(InSpec.Get_RangeClamp());
    const auto& RestReference = InSpec.Get_RangeClamp().Get_RestReferencePoint();
    const auto IsValidRestReference = NOT HasAnyRange || ck::IsValid(RestReference);
    CK_ENSURE_IF_NOT(IsValidRestReference, TEXT("RotateTowards Add rejected [{}]: a range clamp needs a valid rest reference point"), InHandle)
    { return {}; }

    const auto IsRestReferenceOther = NOT HasAnyRange || NOT (RestReference.Get_Entity() == InHandle.Get_Entity());
    CK_ENSURE_IF_NOT(IsRestReferenceOther, TEXT("RotateTowards Add rejected [{}]: the rest reference point cannot be the entity itself"), InHandle)
    { return {}; }

    const auto& Target = InSpec.Get_Target();
    const auto IsTargetSelf = ck::IsValid(Target) && Target.Get_Entity() == InHandle.Get_Entity();
    CK_ENSURE_IF_NOT(NOT IsTargetSelf, TEXT("RotateTowards Add rejected [{}]: an entity cannot target itself"), InHandle)
    { return {}; }

    auto Entity = FCk_Handle{InHandle};
    Entity.Add<ck::FFragment_RotateTowards_Tunables>(InSpec.Get_Tunables());
    Entity.Add<ck::FFragment_RotateTowards>(Target);
    if (ck::IsValid(Target))
    {
        Entity.Add<ck::FTag_RotateTowards_HasTarget>();
        Entity.Add<ck::FFragment_RotateTowards_SolveResult>();
    }
    if (HasAnyRange)
    { Entity.Add<ck::FFragment_RotateTowards_RangeClamp>(InSpec.Get_RangeClamp()); }
    if (InSpec.Get_StartingState() == ECk_EnableDisable::Disable)
    { Entity.Add<ck::FTag_RotateTowards_Disabled>(); }
    return CastChecked(Entity);
}

auto
    UCk_Utils_RotateTowards_UE::
    Get_Target(
        const FCk_Handle_RotateTowards& InHandle)
    -> FCk_Handle_Transform
{
    return InHandle.Get<ck::FFragment_RotateTowards>().Get_Target();
}

auto
    UCk_Utils_RotateTowards_UE::
    Get_HasTarget(
        const FCk_Handle_RotateTowards& InHandle)
    -> bool
{
    return InHandle.Has<ck::FTag_RotateTowards_HasTarget>();
}

auto
    UCk_Utils_RotateTowards_UE::
    Get_IsAtTarget(
        const FCk_Handle_RotateTowards& InHandle)
    -> bool
{
    return InHandle.Has<ck::FTag_RotateTowards_AtTarget>();
}

auto
    UCk_Utils_RotateTowards_UE::
    Get_IsEnabled(
        const FCk_Handle_RotateTowards& InHandle)
    -> bool
{
    return NOT InHandle.Has<ck::FTag_RotateTowards_Disabled>();
}

auto
    UCk_Utils_RotateTowards_UE::
    Get_Tunables(
        const FCk_Handle_RotateTowards& InHandle)
    -> FCk_RotateTowards_Tunables
{
    return InHandle.Get<ck::FFragment_RotateTowards_Tunables>();
}

auto
    UCk_Utils_RotateTowards_UE::
    Get_HasRangeClamp(
        const FCk_Handle_RotateTowards& InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_RotateTowards_RangeClamp>();
}

auto
    UCk_Utils_RotateTowards_UE::
    Get_RangeClamp(
        const FCk_Handle_RotateTowards& InHandle)
    -> FCk_RotateTowards_RangeClamp
{
    if (NOT InHandle.Has<ck::FFragment_RotateTowards_RangeClamp>())
    { return {}; }
    return InHandle.Get<ck::FFragment_RotateTowards_RangeClamp>();
}

auto
    UCk_Utils_RotateTowards_UE::
    Get_DesiredRotation(
        const FCk_Handle_RotateTowards& InHandle)
    -> FRotator
{
    if (NOT InHandle.Has<ck::FFragment_RotateTowards_SolveResult>())
    { return FRotator::ZeroRotator; }
    return InHandle.Get<ck::FFragment_RotateTowards_SolveResult>().Get_DesiredRotation();
}

auto
    UCk_Utils_RotateTowards_UE::
    Get_RemainingRotation(
        const FCk_Handle_RotateTowards& InHandle)
    -> FRotator
{
    if (NOT InHandle.Has<ck::FFragment_RotateTowards_SolveResult>())
    { return FRotator::ZeroRotator; }
    return InHandle.Get<ck::FFragment_RotateTowards_SolveResult>().Get_RemainingDelta();
}

auto
    UCk_Utils_RotateTowards_UE::
    Request_SetTarget(
        FCk_Handle_RotateTowards& InHandle,
        const FCk_Request_RotateTowards_SetTarget& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_RotateTowards
{
    if (ck::IsValid(InHandle) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    const auto IsValidHandle = ck::IsValid(InHandle) && Has(InHandle);
    CK_ENSURE_IF_NOT(IsValidHandle, TEXT("RotateTowards SetTarget rejected invalid handle [{}]"), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    const auto IsValidTarget = ck::IsValid(InRequest.Get_Target());
    CK_ENSURE_IF_NOT(IsValidTarget, TEXT("RotateTowards SetTarget rejected invalid target on [{}]; use Request_ClearTarget to clear"), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    const auto IsTargetOther = NOT (InRequest.Get_Target().Get_Entity() == InHandle.Get_Entity());
    CK_ENSURE_IF_NOT(IsTargetOther, TEXT("RotateTowards SetTarget rejected [{}]: an entity cannot target itself"), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_RotateTowards_Requests, InHandle);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }
    InHandle.AddOrGet<ck::FFragment_RotateTowards_Requests>()._Requests.Emplace(Request);
    return InHandle;
}

auto
    UCk_Utils_RotateTowards_UE::
    Request_ClearTarget(
        FCk_Handle_RotateTowards& InHandle,
        const FCk_Request_RotateTowards_ClearTarget& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_RotateTowards
{
    if (ck::IsValid(InHandle) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    const auto IsValidHandle = ck::IsValid(InHandle) && Has(InHandle);
    CK_ENSURE_IF_NOT(IsValidHandle, TEXT("RotateTowards ClearTarget rejected invalid handle [{}]"), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_RotateTowards_Requests, InHandle);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }
    InHandle.AddOrGet<ck::FFragment_RotateTowards_Requests>()._Requests.Emplace(Request);
    return InHandle;
}

auto
    UCk_Utils_RotateTowards_UE::
    Request_UpdateTunables(
        FCk_Handle_RotateTowards& InHandle,
        const FCk_Request_RotateTowards_UpdateTunables& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_RotateTowards
{
    if (ck::IsValid(InHandle) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    const auto IsValidHandle = ck::IsValid(InHandle) && Has(InHandle);
    CK_ENSURE_IF_NOT(IsValidHandle, TEXT("RotateTowards UpdateTunables rejected invalid handle [{}]"), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    const auto IsValidTunables = ck::rotate_towards::Get_IsTunablesValid(InRequest.Get_Tunables());
    CK_ENSURE_IF_NOT(IsValidTunables, TEXT("RotateTowards UpdateTunables rejected invalid tunables on [{}]"), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_RotateTowards_Requests, InHandle);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }
    InHandle.AddOrGet<ck::FFragment_RotateTowards_Requests>()._Requests.Emplace(Request);
    return InHandle;
}

auto
    UCk_Utils_RotateTowards_UE::
    Request_SetRangeClamp(
        FCk_Handle_RotateTowards& InHandle,
        const FCk_Request_RotateTowards_SetRangeClamp& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_RotateTowards
{
    if (ck::IsValid(InHandle) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    const auto IsValidHandle = ck::IsValid(InHandle) && Has(InHandle);
    CK_ENSURE_IF_NOT(IsValidHandle, TEXT("RotateTowards SetRangeClamp rejected invalid handle [{}]"), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    const auto& RangeClamp = InRequest.Get_RangeClamp();
    const auto HasAnyRange = ck::rotate_towards::Get_HasAnyRange(RangeClamp);
    CK_ENSURE_IF_NOT(HasAnyRange, TEXT("RotateTowards SetRangeClamp rejected [{}]: no axis enabled; use Request_ClearRangeClamp"), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    const auto IsValidRangeClamp = ck::rotate_towards::Get_IsRangeClampValid(RangeClamp);
    CK_ENSURE_IF_NOT(IsValidRangeClamp, TEXT("RotateTowards SetRangeClamp rejected invalid range clamp on [{}]"), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    const auto IsValidRestReference = ck::IsValid(RangeClamp.Get_RestReferencePoint());
    CK_ENSURE_IF_NOT(IsValidRestReference, TEXT("RotateTowards SetRangeClamp rejected [{}]: a range clamp needs a valid rest reference point"), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    const auto IsRestReferenceOther = NOT (RangeClamp.Get_RestReferencePoint().Get_Entity() == InHandle.Get_Entity());
    CK_ENSURE_IF_NOT(IsRestReferenceOther, TEXT("RotateTowards SetRangeClamp rejected [{}]: the rest reference point cannot be the entity itself"), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_RotateTowards_Requests, InHandle);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }
    InHandle.AddOrGet<ck::FFragment_RotateTowards_Requests>()._Requests.Emplace(Request);
    return InHandle;
}

auto
    UCk_Utils_RotateTowards_UE::
    Request_ClearRangeClamp(
        FCk_Handle_RotateTowards& InHandle,
        const FCk_Request_RotateTowards_ClearRangeClamp& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_RotateTowards
{
    if (ck::IsValid(InHandle) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    const auto IsValidHandle = ck::IsValid(InHandle) && Has(InHandle);
    CK_ENSURE_IF_NOT(IsValidHandle, TEXT("RotateTowards ClearRangeClamp rejected invalid handle [{}]"), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_RotateTowards_Requests, InHandle);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }
    InHandle.AddOrGet<ck::FFragment_RotateTowards_Requests>()._Requests.Emplace(Request);
    return InHandle;
}

auto
    UCk_Utils_RotateTowards_UE::
    Request_EnableDisable(
        FCk_Handle_RotateTowards& InHandle,
        const FCk_Request_RotateTowards_EnableDisable& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_RotateTowards
{
    if (ck::IsValid(InHandle) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    const auto IsValidHandle = ck::IsValid(InHandle) && Has(InHandle);
    CK_ENSURE_IF_NOT(IsValidHandle, TEXT("RotateTowards EnableDisable rejected invalid handle [{}]"), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_RotateTowards_Requests, InHandle);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }
    InHandle.AddOrGet<ck::FFragment_RotateTowards_Requests>()._Requests.Emplace(Request);
    return InHandle;
}

auto
    UCk_Utils_RotateTowards_UE::
    BindTo_OnTargetChanged(
        FCk_Handle_RotateTowards& InHandle,
        const FCk_Delegate_RotateTowards_OnTargetChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_RotateTowards
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnRotateTowardsTargetChanged, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);

    return InHandle;
}

auto
    UCk_Utils_RotateTowards_UE::
    UnbindFrom_OnTargetChanged(
        FCk_Handle_RotateTowards& InHandle,
        const FCk_Delegate_RotateTowards_OnTargetChanged& InDelegate)
    -> FCk_Handle_RotateTowards
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnRotateTowardsTargetChanged, InHandle, InDelegate);

    return InHandle;
}

auto
    UCk_Utils_RotateTowards_UE::
    BindTo_OnTargetReached(
        FCk_Handle_RotateTowards& InHandle,
        const FCk_Delegate_RotateTowards_OnTargetReached& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_RotateTowards
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnRotateTowardsTargetReached, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);

    return InHandle;
}

auto
    UCk_Utils_RotateTowards_UE::
    UnbindFrom_OnTargetReached(
        FCk_Handle_RotateTowards& InHandle,
        const FCk_Delegate_RotateTowards_OnTargetReached& InDelegate)
    -> FCk_Handle_RotateTowards
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnRotateTowardsTargetReached, InHandle, InDelegate);

    return InHandle;
}

auto
    UCk_Utils_RotateTowards_UE::
    BindTo_OnTargetCleared(
        FCk_Handle_RotateTowards& InHandle,
        const FCk_Delegate_RotateTowards_OnTargetCleared& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_RotateTowards
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnRotateTowardsTargetCleared, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);

    return InHandle;
}

auto
    UCk_Utils_RotateTowards_UE::
    UnbindFrom_OnTargetCleared(
        FCk_Handle_RotateTowards& InHandle,
        const FCk_Delegate_RotateTowards_OnTargetCleared& InDelegate)
    -> FCk_Handle_RotateTowards
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnRotateTowardsTargetCleared, InHandle, InDelegate);

    return InHandle;
}
