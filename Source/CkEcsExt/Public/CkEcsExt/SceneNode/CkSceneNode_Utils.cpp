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
    if (InAttachTo.Has<ck::FTag_SceneNode_Layer0>()) { InHandle.Add<ck::FTag_SceneNode_Layer1>(); }
    else if (InAttachTo.Has<ck::FTag_SceneNode_Layer1>()) { InHandle.Add<ck::FTag_SceneNode_Layer2>(); }
    else if (InAttachTo.Has<ck::FTag_SceneNode_Layer2>()) { InHandle.Add<ck::FTag_SceneNode_Layer3>(); }
    else if (InAttachTo.Has<ck::FTag_SceneNode_Layer3>()) { InHandle.Add<ck::FTag_SceneNode_Layer4>(); }
    else if (InAttachTo.Has<ck::FTag_SceneNode_Layer4>()) { InHandle.Add<ck::FTag_SceneNode_Layer5>(); }
    else if (InAttachTo.Has<ck::FTag_SceneNode_Layer5>()) { InHandle.Add<ck::FTag_SceneNode_Layer6>(); }
    else if (InAttachTo.Has<ck::FTag_SceneNode_Layer6>()) { InHandle.Add<ck::FTag_SceneNode_Layer7>(); }
    else if (InAttachTo.Has<ck::FTag_SceneNode_Layer7>()) { InHandle.Add<ck::FTag_SceneNode_Layer8>(); }
    else if (InAttachTo.Has<ck::FTag_SceneNode_Layer8>()) { InHandle.Add<ck::FTag_SceneNode_Layer9>(); }
    else if (InAttachTo.Has<ck::FTag_SceneNode_Layer9>()) { CK_TRIGGER_ENSURE(TEXT("We are the maximum number of SceneNode layers")); }
    else { InHandle.Add<ck::FTag_SceneNode_Layer0>(); }

    InHandle.Add<ck::FFragment_SceneNode_Current>(InLocalTransform);
    InHandle.Add<ck::FTag_SceneNode_RelativeTransformUpdated>();

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

// --------------------------------------------------------------------------------------------------------------------
