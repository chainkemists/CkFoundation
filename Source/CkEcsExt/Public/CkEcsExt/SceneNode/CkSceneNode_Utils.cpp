#include "CkSceneNode_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEcsExt/SceneNode/CkSceneNode_Fragment.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SceneNode_UE::
    Add(
        FCk_Handle_Transform& InHandle,
        FCk_Handle_Transform& InAttachTo,
        FTransform InLocalTransform)
    -> FCk_Handle_SceneNode
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("InHandle [{}] is INVALID. Unable to proceed with SceneNode attaching to [{}]"), InHandle, InAttachTo)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InAttachTo),
        TEXT("InAttachTo [{}] is INVALID. Unable to proceed with adding the SceneNode feature to [{}]"), InAttachTo, InHandle)
    { return {}; }

    const auto ParentLayerIndex = Get_LayerIndex(InAttachTo);
    const auto MyLayerIndex = ParentLayerIndex.IsSet() ? ParentLayerIndex.GetValue() + 1 : 0;

    if (NOT AssignLayerByIndex(InHandle, MyLayerIndex))
    { return {}; }

    // If this entity already has children attached to it, their layers need to be updated
    // to maintain proper hierarchy ordering
    PropagateLayerToChildren(InHandle, MyLayerIndex);

    InHandle.Add<ck::FFragment_SceneNode_Current>(InLocalTransform);

    ck::USceneNodeParent_Utils::AddOrReplace(InHandle, InAttachTo);

    auto SceneNodeHandle = Cast(InHandle);

    ck::FUtils_RecordOfSceneNodes::AddIfMissing(InAttachTo);
    ck::FUtils_RecordOfSceneNodes::Request_Connect(InAttachTo, SceneNodeHandle, ECk_Record_LabelRequirementPolicy::Optional);

    return SceneNodeHandle;
}

auto
    UCk_Utils_SceneNode_UE::
    Create(
        FCk_Handle_Transform& InOwner,
        FTransform InLocalTransform)
    -> FCk_Handle_SceneNode
{
    auto SceneNodeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    UCk_Utils_Handle_UE::Set_DebugName(SceneNodeEntity, *ck::Format_UE(TEXT("SCENE NODE: [{}]"), InOwner));

    const auto& OwnerTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InOwner);

    auto SceneNodeWithTransform = UCk_Utils_Transform_UE::Add(SceneNodeEntity, OwnerTransform, ECk_Replication::DoesNotReplicate);

    return Add(SceneNodeWithTransform, InOwner, InLocalTransform);
}

auto
    UCk_Utils_SceneNode_UE::
    CreateAndAttachToUnrealComponent(
        FCk_Handle_Transform& InAttachTo,
        USceneComponent* InSceneComponent)
    -> FCk_Handle_SceneNode
{
    auto SceneNodeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttachTo);
    UCk_Utils_Handle_UE::Set_DebugName(SceneNodeEntity, *ck::Format_UE(TEXT("SCENE NODE: [{} > {}]"), InAttachTo, InSceneComponent));

    auto SceneNodeWithTransform = UCk_Utils_Transform_UE::AddAndAttachToUnrealComponent(SceneNodeEntity, InSceneComponent, ECk_Replication::DoesNotReplicate);

    return Add(SceneNodeWithTransform, InAttachTo, FTransform::Identity);
}

auto
    UCk_Utils_SceneNode_UE::
    CreateAndAttachToUnrealMesh(
        FCk_Handle_Transform& InAttachTo,
        const UMeshComponent* InMeshComponent,
        FName InSocketName)
    -> FCk_Handle_SceneNode
{
    auto SceneNodeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttachTo);
    UCk_Utils_Handle_UE::Set_DebugName(SceneNodeEntity, *ck::Format_UE(TEXT("SCENE NODE: [{} > {} > {}]"), InAttachTo, InMeshComponent, InSocketName));

    auto SceneNodeWithTransform = UCk_Utils_Transform_UE::AddAndAttachToUnrealMesh(SceneNodeEntity, InMeshComponent, InSocketName, ECk_Replication::DoesNotReplicate);

    return Add(SceneNodeWithTransform, InAttachTo, FTransform::Identity);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_SceneNode_UE, FCk_Handle_SceneNode,
    ck::SceneNodeParent, ck::FFragment_SceneNode_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SceneNode_UE::
    Get_Offset(
        const FCk_Handle_SceneNode& InSceneNode)
    -> FTransform
{
    return InSceneNode.Get<ck::FFragment_SceneNode_Current>().Get_RelativeTransform();
}

auto
    UCk_Utils_SceneNode_UE::
    Get_Parent(
        const FCk_Handle_SceneNode& InSceneNode)
    -> FCk_Handle_Transform
{
    return ck::USceneNodeParent_Utils::Get_StoredEntity_AsTypeSafe<FCk_Handle_Transform>(InSceneNode);
}

auto
    UCk_Utils_SceneNode_UE::
    Request_UpdateOffset(
        FCk_Handle_SceneNode& InSceneNode,
        const FCk_Request_SceneNode_UpdateRelativeTransform& InRequest)
    -> FCk_Handle_SceneNode
{
    InSceneNode.AddOrGet<ck::FFragment_SceneNode_Requests>()._Requests.Emplace(InRequest);
    return InSceneNode;
}

auto
    UCk_Utils_SceneNode_UE::
    ForEach_SceneNode(
        FCk_Handle_Transform& InHandle,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_SceneNode>
{
    auto Abilities = TArray<FCk_Handle_SceneNode>{};

    ForEach_SceneNode
    (
        InHandle,
        [&](const FCk_Handle_SceneNode& InSceneNode)
        {
            if (InDelegate.IsBound())
            { InDelegate.Execute(InSceneNode, InOptionalPayload); }
            else
            { Abilities.Emplace(InSceneNode); }
        }
    );

    return Abilities;
}

auto
    UCk_Utils_SceneNode_UE::
    ForEach_SceneNode(
        FCk_Handle_Transform InHandle,
        const TFunction<void(FCk_Handle_SceneNode)>& InFunc)
    -> void
{
    ck::FUtils_RecordOfSceneNodes::ForEach_ValidEntry(InHandle, InFunc);
}

auto
    UCk_Utils_SceneNode_UE::
    Get_LayerIndex(
        const FCk_Handle& InHandle)
    -> TOptional<int32>
{
    if (InHandle.Has<ck::FTag_SceneNode_Layer0>()) { return 0; }
    if (InHandle.Has<ck::FTag_SceneNode_Layer1>()) { return 1; }
    if (InHandle.Has<ck::FTag_SceneNode_Layer2>()) { return 2; }
    if (InHandle.Has<ck::FTag_SceneNode_Layer3>()) { return 3; }
    if (InHandle.Has<ck::FTag_SceneNode_Layer4>()) { return 4; }
    if (InHandle.Has<ck::FTag_SceneNode_Layer5>()) { return 5; }
    if (InHandle.Has<ck::FTag_SceneNode_Layer6>()) { return 6; }
    if (InHandle.Has<ck::FTag_SceneNode_Layer7>()) { return 7; }
    if (InHandle.Has<ck::FTag_SceneNode_Layer8>()) { return 8; }
    if (InHandle.Has<ck::FTag_SceneNode_Layer9>()) { return 9; }
    return {};
}

auto
    UCk_Utils_SceneNode_UE::
    RemoveExistingLayerTag(
        FCk_Handle_Transform& InHandle)
    -> void
{
    InHandle.Try_Remove<ck::FTag_SceneNode_RelativeTransformUpdated>();

    InHandle.Try_Remove<ck::FTag_SceneNode_Layer0>();
    InHandle.Try_Remove<ck::FTag_SceneNode_Layer1>();
    InHandle.Try_Remove<ck::FTag_SceneNode_Layer2>();
    InHandle.Try_Remove<ck::FTag_SceneNode_Layer3>();
    InHandle.Try_Remove<ck::FTag_SceneNode_Layer4>();
    InHandle.Try_Remove<ck::FTag_SceneNode_Layer5>();
    InHandle.Try_Remove<ck::FTag_SceneNode_Layer6>();
    InHandle.Try_Remove<ck::FTag_SceneNode_Layer7>();
    InHandle.Try_Remove<ck::FTag_SceneNode_Layer8>();
    InHandle.Try_Remove<ck::FTag_SceneNode_Layer9>();
}

auto
    UCk_Utils_SceneNode_UE::
    AssignLayerByIndex(
        FCk_Handle_Transform& InHandle,
        int32 InLayerIndex)
    -> bool
{
    InHandle.AddOrGet<ck::FTag_SceneNode_RelativeTransformUpdated>();

    switch (InLayerIndex)
    {
        case 0: InHandle.Add<ck::FTag_SceneNode_Layer0>(); return true;
        case 1: InHandle.Add<ck::FTag_SceneNode_Layer1>(); return true;
        case 2: InHandle.Add<ck::FTag_SceneNode_Layer2>(); return true;
        case 3: InHandle.Add<ck::FTag_SceneNode_Layer3>(); return true;
        case 4: InHandle.Add<ck::FTag_SceneNode_Layer4>(); return true;
        case 5: InHandle.Add<ck::FTag_SceneNode_Layer5>(); return true;
        case 6: InHandle.Add<ck::FTag_SceneNode_Layer6>(); return true;
        case 7: InHandle.Add<ck::FTag_SceneNode_Layer7>(); return true;
        case 8: InHandle.Add<ck::FTag_SceneNode_Layer8>(); return true;
        case 9: InHandle.Add<ck::FTag_SceneNode_Layer9>(); return true;
        default:
        {
            CK_TRIGGER_ENSURE(TEXT("Layer index [{}] exceeds maximum supported SceneNode layers"), InLayerIndex);
            return false;
        }
    }
}

auto
    UCk_Utils_SceneNode_UE::
    PropagateLayerToChildren(
        FCk_Handle_Transform& InParent,
        int32 InParentLayerIndex)
    -> void
{
    const auto ChildLayerIndex = InParentLayerIndex + 1;

    ForEach_SceneNode(InParent, [ChildLayerIndex](FCk_Handle_SceneNode InChild)
    {
        if (const auto& CurrentLayerIndex = Get_LayerIndex(InChild);
            CurrentLayerIndex.IsSet() && CurrentLayerIndex.GetValue() == ChildLayerIndex)
        { return; }

        auto ChildTransform = UCk_Utils_Transform_UE::Cast(InChild);

        RemoveExistingLayerTag(ChildTransform);

        if (NOT AssignLayerByIndex(ChildTransform, ChildLayerIndex))
        { return; }

        PropagateLayerToChildren(ChildTransform, ChildLayerIndex);
    });
}

// --------------------------------------------------------------------------------------------------------------------
