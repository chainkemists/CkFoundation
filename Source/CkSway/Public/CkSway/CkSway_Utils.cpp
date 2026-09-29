#include "CkSway_Utils.h"

#include "CkSway/CkSway_Kernel.h"
#include "CkSway/CkSway_Log.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Sway_UE, FCk_Handle_Sway, ck::FFragment_Sway, ck::FFragment_Sway_Tunables);

auto
    UCk_Utils_Sway_UE::
    Add(
        FCk_Handle_SceneNode& InSceneNode,
        const FCk_Sway_Spec& InSpec)
    -> FCk_Handle_Sway
{
    const auto IsValidSceneNode = ck::IsValid(InSceneNode) && UCk_Utils_SceneNode_UE::Has(InSceneNode);
    CK_ENSURE_IF_NOT(IsValidSceneNode, TEXT("Sway Add rejected invalid scene node [{}]"), InSceneNode)
    { return {}; }
    const auto IsAlreadySway = Has(InSceneNode);
    CK_ENSURE_IF_NOT(NOT IsAlreadySway, TEXT("Sway Add rejected [{}]: already a sway node"), InSceneNode)
    { return {}; }
    const auto IsValidSpec = ck::sway::Get_IsSpecValid(InSpec);
    CK_ENSURE_IF_NOT(IsValidSpec, TEXT("Sway Add rejected invalid spec on [{}]"), InSceneNode)
    { return {}; }

    auto Entity = FCk_Handle{InSceneNode};
    Entity.Add<ck::FFragment_Sway_Tunables>(InSpec);
    Entity.Add<ck::FFragment_Sway>(UCk_Utils_SceneNode_UE::Get_Offset(InSceneNode));
    Entity.Add<ck::FTag_Sway_NeedsSetup>();
    return CastChecked(Entity);
}

auto
    UCk_Utils_Sway_UE::
    Create(
        FCk_Handle_Transform& InParent,
        FTransform InLocalRest,
        const FCk_Sway_Spec& InSpec)
    -> FCk_Handle_Sway
{
    const auto IsValidParent = ck::IsValid(InParent) && UCk_Utils_Transform_UE::Has(InParent);
    CK_ENSURE_IF_NOT(IsValidParent, TEXT("Sway Create rejected invalid parent [{}]"), InParent)
    { return {}; }
    const auto IsValidSpec = ck::sway::Get_IsSpecValid(InSpec);
    CK_ENSURE_IF_NOT(IsValidSpec, TEXT("Sway Create rejected invalid spec for parent [{}]"), InParent)
    { return {}; }
    const auto IsFiniteRest = NOT InLocalRest.ContainsNaN();
    CK_ENSURE_IF_NOT(IsFiniteRest, TEXT("Sway Create rejected non-finite rest offset for parent [{}]"), InParent)
    { return {}; }
    const auto CanCreate = UCk_Utils_EntityLifetime_UE::Get_CanCreateEntity(InParent);
    CK_ENSURE_IF_NOT(CanCreate, TEXT("Sway Create cannot create a child of parent [{}]"), InParent)
    { return {}; }

    auto Node = UCk_Utils_SceneNode_UE::Create(InParent, InLocalRest);
    const auto IsCreated = ck::IsValid(Node);
    CK_ENSURE_IF_NOT(IsCreated, TEXT("Sway Create failed to create the scene node under [{}]"), InParent)
    { return {}; }

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
    auto NodeEntity = FCk_Handle{Node};
    UCk_Utils_Handle_UE::Set_DebugName(NodeEntity, *ck::Format_UE(TEXT("Sway({})"), InParent));
#endif
    return Add(Node, InSpec);
}

auto
    UCk_Utils_Sway_UE::
    Get_Spec(
        const FCk_Handle_Sway& InSway)
    -> FCk_Sway_Spec
{
    return InSway.Get<ck::FFragment_Sway_Tunables>();
}

auto
    UCk_Utils_Sway_UE::
    Get_IsEnabled(
        const FCk_Handle_Sway& InSway)
    -> bool
{
    return NOT InSway.Has<ck::FTag_Sway_Disabled>();
}

auto
    UCk_Utils_Sway_UE::
    Get_IsSettled(
        const FCk_Handle_Sway& InSway)
    -> bool
{
    const auto& Current = InSway.Get<ck::FFragment_Sway>();
    return ck::sway::Get_IsSettled(Current.Get_Location()) && ck::sway::Get_IsSettled(Current.Get_Rotation());
}

auto
    UCk_Utils_Sway_UE::
    Get_RestOffset(
        const FCk_Handle_Sway& InSway)
    -> FTransform
{
    return InSway.Get<ck::FFragment_Sway>().Get_RestOffset();
}

auto
    UCk_Utils_Sway_UE::
    Get_LocationOffset(
        const FCk_Handle_Sway& InSway)
    -> FVector
{
    return InSway.Get<ck::FFragment_Sway>().Get_Location()._Value;
}

auto
    UCk_Utils_Sway_UE::
    Get_RotationOffset(
        const FCk_Handle_Sway& InSway)
    -> FRotator
{
    const auto& Rotation = InSway.Get<ck::FFragment_Sway>().Get_Rotation()._Value;
    // FRotator ctor order is (Pitch, Yaw, Roll); the channel vector is (Roll, Pitch, Yaw).
    return FRotator{Rotation.Y, Rotation.Z, Rotation.X};
}

auto
    UCk_Utils_Sway_UE::
    Get_SwayOffset(
        const FCk_Handle_Sway& InSway)
    -> FTransform
{
    const auto& Sway = InSway.Get<ck::FFragment_Sway>();
    return ck::sway::Compose_Offset(Sway.Get_Location()._Value, Sway.Get_Rotation()._Value);
}

auto
    UCk_Utils_Sway_UE::
    Get_LastStimulus(
        const FCk_Handle_Sway& InSway)
    -> FCk_Sway_Stimulus
{
    return InSway.Get<ck::FFragment_Sway>().Get_LastStimulus();
}

auto
    UCk_Utils_Sway_UE::
    Request_UpdateSpec(
        FCk_Handle_Sway& InSway,
        const FCk_Request_Sway_UpdateSpec& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Sway
{
    if (ck::IsValid(InSway) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InSway,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InSway, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InSway;
    }
    const auto IsValidSway = ck::IsValid(InSway) && Has(InSway);
    CK_ENSURE_IF_NOT(IsValidSway, TEXT("Sway UpdateSpec rejected invalid sway [{}]"), InSway)
    {
        InDelegate.ExecuteIfBound(InSway, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InSway;
    }
    const auto IsValidSpec = ck::sway::Get_IsSpecValid(InRequest.Get_Spec());
    CK_ENSURE_IF_NOT(IsValidSpec, TEXT("Sway UpdateSpec rejected invalid spec on [{}]"), InSway)
    {
        InDelegate.ExecuteIfBound(InSway, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InSway;
    }
    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_Sway_Requests, InSway);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }
    InSway.AddOrGet<ck::FFragment_Sway_Requests>()._Requests.Emplace(Request);
    return InSway;
}

auto
    UCk_Utils_Sway_UE::
    Request_EnableDisable(
        FCk_Handle_Sway& InSway,
        const FCk_Request_Sway_EnableDisable& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Sway
{
    if (ck::IsValid(InSway) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InSway,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InSway, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InSway;
    }
    const auto IsValidSway = ck::IsValid(InSway) && Has(InSway);
    CK_ENSURE_IF_NOT(IsValidSway, TEXT("Sway EnableDisable rejected invalid sway [{}]"), InSway)
    {
        InDelegate.ExecuteIfBound(InSway, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InSway;
    }
    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_Sway_Requests, InSway);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }
    InSway.AddOrGet<ck::FFragment_Sway_Requests>()._Requests.Emplace(Request);
    return InSway;
}

auto
    UCk_Utils_Sway_UE::
    Request_Reset(
        FCk_Handle_Sway& InSway,
        const FCk_Request_Sway_Reset& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Sway
{
    if (ck::IsValid(InSway) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InSway,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InSway, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InSway;
    }
    const auto IsValidSway = ck::IsValid(InSway) && Has(InSway);
    CK_ENSURE_IF_NOT(IsValidSway, TEXT("Sway Reset rejected invalid sway [{}]"), InSway)
    {
        InDelegate.ExecuteIfBound(InSway, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InSway;
    }
    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_Sway_Requests, InSway);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }
    InSway.AddOrGet<ck::FFragment_Sway_Requests>()._Requests.Emplace(Request);
    return InSway;
}

auto
    UCk_Utils_Sway_UE::
    Request_SetRestOffset(
        FCk_Handle_Sway& InSway,
        const FCk_Request_Sway_SetRestOffset& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Sway
{
    if (ck::IsValid(InSway) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InSway,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InSway, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InSway;
    }
    const auto IsValidSway = ck::IsValid(InSway) && Has(InSway);
    CK_ENSURE_IF_NOT(IsValidSway, TEXT("Sway SetRestOffset rejected invalid sway [{}]"), InSway)
    {
        InDelegate.ExecuteIfBound(InSway, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InSway;
    }
    const auto IsFiniteRest = NOT InRequest.Get_RestOffset().ContainsNaN();
    CK_ENSURE_IF_NOT(IsFiniteRest, TEXT("Sway SetRestOffset rejected non-finite rest offset on [{}]"), InSway)
    {
        InDelegate.ExecuteIfBound(InSway, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InSway;
    }
    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_Sway_Requests, InSway);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }
    InSway.AddOrGet<ck::FFragment_Sway_Requests>()._Requests.Emplace(Request);
    return InSway;
}
