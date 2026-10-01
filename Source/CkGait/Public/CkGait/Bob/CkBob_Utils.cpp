#include "CkBob_Utils.h"

#include "CkGait/Bob/CkBob_Kernel.h"
#include "CkGait/Gait/CkGait_Utils.h"
#include "CkGait/CkGait_Log.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Bob_UE, FCk_Handle_Bob, ck::FFragment_Bob, ck::FFragment_Bob_Tunables);

auto
    UCk_Utils_Bob_UE::
    Add(
        FCk_Handle_SceneNode& InSceneNode,
        const FCk_Bob_Spec& InSpec)
    -> FCk_Handle_Bob
{
    const auto IsValidSceneNode = ck::IsValid(InSceneNode) && UCk_Utils_SceneNode_UE::Has(InSceneNode);
    CK_ENSURE_IF_NOT(IsValidSceneNode, TEXT("Bob Add rejected invalid scene node [{}]"), InSceneNode)
    { return {}; }

    const auto IsAlreadyBob = Has(InSceneNode);
    CK_ENSURE_IF_NOT(NOT IsAlreadyBob, TEXT("Bob Add rejected [{}]: already a bob node"), InSceneNode)
    { return {}; }

    const auto& Gait = InSpec.Get_Gait();
    const auto IsValidGait = ck::IsValid(Gait) && UCk_Utils_Gait_UE::Has(Gait);
    CK_ENSURE_IF_NOT(IsValidGait, TEXT("Bob Add rejected invalid gait [{}] for node [{}]"), Gait, InSceneNode)
    { return {}; }

    const auto IsValidSpec = ck::bob::Get_IsSpecValid(InSpec);
    CK_ENSURE_IF_NOT(IsValidSpec, TEXT("Bob Add rejected invalid spec on [{}]"), InSceneNode)
    { return {}; }

    // The node's current offset becomes the rest AND the last written offset, so adopting a node raises no foreign-write
    // ensure on the first Update.
    const auto CurrentOffset = UCk_Utils_SceneNode_UE::Get_Offset(InSceneNode);

    auto Entity = FCk_Handle{InSceneNode};
    Entity.Add<ck::FFragment_Bob_Tunables>(InSpec);
    Entity.Add<ck::FFragment_Bob>(CurrentOffset, UCk_Utils_Gait_UE::Get_LandingCount(Gait), CurrentOffset);
    if (InSpec.Get_StartingState() == ECk_EnableDisable::Disable)
    { Entity.Add<ck::FTag_Bob_Disabled>(); }

    return CastChecked(Entity);
}

auto
    UCk_Utils_Bob_UE::
    Create(
        FCk_Handle_Transform& InParent,
        FTransform InLocalRest,
        const FCk_Bob_Spec& InSpec)
    -> FCk_Handle_Bob
{
    const auto IsValidParent = ck::IsValid(InParent) && UCk_Utils_Transform_UE::Has(InParent);
    CK_ENSURE_IF_NOT(IsValidParent, TEXT("Bob Create rejected invalid parent [{}]"), InParent)
    { return {}; }

    const auto IsValidSpec = ck::bob::Get_IsSpecValid(InSpec);
    CK_ENSURE_IF_NOT(IsValidSpec, TEXT("Bob Create rejected invalid spec for parent [{}]"), InParent)
    { return {}; }

    const auto& Gait = InSpec.Get_Gait();
    const auto IsValidGait = ck::IsValid(Gait) && UCk_Utils_Gait_UE::Has(Gait);
    CK_ENSURE_IF_NOT(IsValidGait, TEXT("Bob Create rejected invalid gait [{}] for parent [{}]"), Gait, InParent)
    { return {}; }

    const auto IsFiniteRest = NOT InLocalRest.ContainsNaN();
    CK_ENSURE_IF_NOT(IsFiniteRest, TEXT("Bob Create rejected non-finite rest offset for parent [{}]"), InParent)
    { return {}; }

    const auto CanCreate = UCk_Utils_EntityLifetime_UE::Get_CanCreateEntity(InParent);
    CK_ENSURE_IF_NOT(CanCreate, TEXT("Bob Create cannot create a child of parent [{}]"), InParent)
    { return {}; }

    auto Node = UCk_Utils_SceneNode_UE::Create(InParent, InLocalRest);
    const auto IsCreated = ck::IsValid(Node);
    CK_ENSURE_IF_NOT(IsCreated, TEXT("Bob Create failed to create the scene node under [{}]"), InParent)
    { return {}; }

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
    auto NodeEntity = FCk_Handle{Node};
    UCk_Utils_Handle_UE::Set_DebugName(NodeEntity, *ck::Format_UE(TEXT("Bob({})"), InParent));
#endif
    return Add(Node, InSpec);
}

auto
    UCk_Utils_Bob_UE::
    Get_Spec(
        const FCk_Handle_Bob& InBob)
    -> FCk_Bob_Spec
{
    return InBob.Get<ck::FFragment_Bob_Tunables>();
}

auto
    UCk_Utils_Bob_UE::
    Get_Gait(
        const FCk_Handle_Bob& InBob)
    -> FCk_Handle_Gait
{
    return InBob.Get<ck::FFragment_Bob_Tunables>().Get_Gait();
}

auto
    UCk_Utils_Bob_UE::
    Get_IsEnabled(
        const FCk_Handle_Bob& InBob)
    -> bool
{
    return NOT InBob.Has<ck::FTag_Bob_Disabled>();
}

auto
    UCk_Utils_Bob_UE::
    Get_RestOffset(
        const FCk_Handle_Bob& InBob)
    -> FTransform
{
    return InBob.Get<ck::FFragment_Bob>().Get_RestOffset();
}

auto
    UCk_Utils_Bob_UE::
    Get_BobOffset(
        const FCk_Handle_Bob& InBob)
    -> FTransform
{
    auto Clamped = InBob.Get<ck::FFragment_Bob>().Get_Smoothed();
    Clamped._LocationCm = ck::bob::Clamp_Location(Clamped._LocationCm, InBob.Get<ck::FFragment_Bob_Tunables>().Get_MaxOffsetCm());
    return ck::bob::Compose_Offset(Clamped);
}

auto
    UCk_Utils_Bob_UE::
    Get_SpringOffset(
        const FCk_Handle_Bob& InBob)
    -> float
{
    return InBob.Get<ck::FFragment_Bob>().Get_Spring()._Offset;
}

auto
    UCk_Utils_Bob_UE::
    Request_UpdateSpec(
        FCk_Handle_Bob& InBob,
        const FCk_Request_Bob_UpdateSpec& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Bob
{
    if (ck::IsValid(InBob) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InBob,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InBob, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InBob;
    }

    const auto IsValidBob = ck::IsValid(InBob) && Has(InBob);
    CK_ENSURE_IF_NOT(IsValidBob, TEXT("Bob UpdateSpec rejected invalid bob [{}]"), InBob)
    {
        InDelegate.ExecuteIfBound(InBob, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InBob;
    }

    const auto IsValidSpec = ck::bob::Get_IsSpecValid(InRequest.Get_Spec());
    CK_ENSURE_IF_NOT(IsValidSpec, TEXT("Bob UpdateSpec rejected invalid spec on [{}]"), InBob)
    {
        InDelegate.ExecuteIfBound(InBob, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InBob;
    }

    const auto& Gait = InRequest.Get_Spec().Get_Gait();
    const auto IsValidGait = ck::IsValid(Gait) && UCk_Utils_Gait_UE::Has(Gait);
    CK_ENSURE_IF_NOT(IsValidGait, TEXT("Bob UpdateSpec rejected invalid gait [{}] on [{}]"), Gait, InBob)
    {
        InDelegate.ExecuteIfBound(InBob, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InBob;
    }

    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_Bob_Requests, InBob);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InBob.AddOrGet<ck::FFragment_Bob_Requests>()._Requests.Emplace(Request);
    return InBob;
}

auto
    UCk_Utils_Bob_UE::
    Request_EnableDisable(
        FCk_Handle_Bob& InBob,
        const FCk_Request_Bob_EnableDisable& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Bob
{
    if (ck::IsValid(InBob) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InBob,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InBob, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InBob;
    }

    const auto IsValidBob = ck::IsValid(InBob) && Has(InBob);
    CK_ENSURE_IF_NOT(IsValidBob, TEXT("Bob EnableDisable rejected invalid bob [{}]"), InBob)
    {
        InDelegate.ExecuteIfBound(InBob, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InBob;
    }

    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_Bob_Requests, InBob);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InBob.AddOrGet<ck::FFragment_Bob_Requests>()._Requests.Emplace(Request);
    return InBob;
}

auto
    UCk_Utils_Bob_UE::
    Request_Reset(
        FCk_Handle_Bob& InBob,
        const FCk_Request_Bob_Reset& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Bob
{
    if (ck::IsValid(InBob) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InBob,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InBob, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InBob;
    }

    const auto IsValidBob = ck::IsValid(InBob) && Has(InBob);
    CK_ENSURE_IF_NOT(IsValidBob, TEXT("Bob Reset rejected invalid bob [{}]"), InBob)
    {
        InDelegate.ExecuteIfBound(InBob, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InBob;
    }

    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_Bob_Requests, InBob);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InBob.AddOrGet<ck::FFragment_Bob_Requests>()._Requests.Emplace(Request);
    return InBob;
}

auto
    UCk_Utils_Bob_UE::
    Request_SetRestOffset(
        FCk_Handle_Bob& InBob,
        const FCk_Request_Bob_SetRestOffset& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Bob
{
    if (ck::IsValid(InBob) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InBob,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InBob, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InBob;
    }

    const auto IsValidBob = ck::IsValid(InBob) && Has(InBob);
    CK_ENSURE_IF_NOT(IsValidBob, TEXT("Bob SetRestOffset rejected invalid bob [{}]"), InBob)
    {
        InDelegate.ExecuteIfBound(InBob, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InBob;
    }

    const auto IsFiniteRest = NOT InRequest.Get_RestOffset().ContainsNaN();
    CK_ENSURE_IF_NOT(IsFiniteRest, TEXT("Bob SetRestOffset rejected non-finite rest offset on [{}]"), InBob)
    {
        InDelegate.ExecuteIfBound(InBob, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InBob;
    }

    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_Bob_Requests, InBob);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InBob.AddOrGet<ck::FFragment_Bob_Requests>()._Requests.Emplace(Request);
    return InBob;
}
