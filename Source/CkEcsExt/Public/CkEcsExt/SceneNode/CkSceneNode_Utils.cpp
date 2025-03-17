#include "CkSceneNode_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEcsExt/SceneNode/CkSceneNode_Fragment.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SceneNode_UE::
    Create(
        FCk_Handle_Transform& InOwner,
        FTransform InLocalTransform)
    -> FCk_Handle_SceneNode
{
    auto SceneNodeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    UCk_Utils_Handle_UE::Set_DebugName(SceneNodeEntity, *ck::Format_UE(TEXT("SCENE NODE: [{}]"), InOwner));

    if (InOwner.Has<ck::FTag_SceneNode_Layer0>()) { SceneNodeEntity.Add<ck::FTag_SceneNode_Layer1>(); }
    else if (InOwner.Has<ck::FTag_SceneNode_Layer1>()) { SceneNodeEntity.Add<ck::FTag_SceneNode_Layer2>(); }
    else if (InOwner.Has<ck::FTag_SceneNode_Layer2>()) { SceneNodeEntity.Add<ck::FTag_SceneNode_Layer3>(); }
    else if (InOwner.Has<ck::FTag_SceneNode_Layer3>()) { SceneNodeEntity.Add<ck::FTag_SceneNode_Layer4>(); }
    else if (InOwner.Has<ck::FTag_SceneNode_Layer4>()) { SceneNodeEntity.Add<ck::FTag_SceneNode_Layer5>(); }
    else if (InOwner.Has<ck::FTag_SceneNode_Layer5>()) { SceneNodeEntity.Add<ck::FTag_SceneNode_Layer6>(); }
    else if (InOwner.Has<ck::FTag_SceneNode_Layer6>()) { SceneNodeEntity.Add<ck::FTag_SceneNode_Layer7>(); }
    else if (InOwner.Has<ck::FTag_SceneNode_Layer7>()) { SceneNodeEntity.Add<ck::FTag_SceneNode_Layer8>(); }
    else if (InOwner.Has<ck::FTag_SceneNode_Layer8>()) { SceneNodeEntity.Add<ck::FTag_SceneNode_Layer9>(); }
    else if (InOwner.Has<ck::FTag_SceneNode_Layer9>()) { CK_TRIGGER_ENSURE(TEXT("We are the maximum number of SceneNode layers")); }
    else { SceneNodeEntity.Add<ck::FTag_SceneNode_Layer0>(); }

    const auto MaybeOwnerTransform = [&]
    {
        if (UCk_Utils_Transform_UE::Has(InOwner))
        {
            return UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InOwner);
        }

        return FTransform::Identity;
    }();

    UCk_Utils_Transform_UE::Add(SceneNodeEntity, MaybeOwnerTransform, ECk_Replication::DoesNotReplicate);

    auto& Current = SceneNodeEntity.Add<ck::FFragment_SceneNode_Current>();
    Current._RelativeTransform = InLocalTransform;

    ck::USceneNodeParent_Utils::AddOrReplace(SceneNodeEntity, InOwner);

    return Cast(SceneNodeEntity);
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

// --------------------------------------------------------------------------------------------------------------------