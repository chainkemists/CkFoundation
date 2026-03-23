#include "CkSceneNode_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkEcs/Net/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_scene_node
{
    template <typename T_Handle>
    auto TryUpdateRelativeTransform(
        T_Handle& InHandle,
        ck::FFragment_SceneNode_Current& InCurrent,
        const FTransform& InNewRelativeTransform)
        -> void
    {
        if (InCurrent.Get_RelativeTransform().Equals(InNewRelativeTransform))
        { return; }

        InCurrent.Set_RelativeTransform(InNewRelativeTransform);

        InHandle.template DeferAddOrGet<ck::FTag_SceneNode_RelativeTransformUpdated>();
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_SceneNode_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_SceneNode_Current& InCurrent,
            const FFragment_SceneNode_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_SceneNode_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InCurrent, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });
    }

    auto
        FProcessor_SceneNode_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_SceneNode_Current& InCurrent,
            const FCk_Request_SceneNode_UpdateRelativeTransform& InRequest)
        -> void
    {
        if (InCurrent.Get_RelativeTransform().Equals(InRequest.Get_NewRelativeTransform()))
        { return; }

        InCurrent._RelativeTransform = InRequest.Get_NewRelativeTransform();

        InHandle.AddOrGet<FTag_SceneNode_RelativeTransformUpdated>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_SceneNode_UpdateLocal_FromRootComponent::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const SceneNodeParent& InParent,
            FFragment_SceneNode_Current& InCurrent,
            const FFragment_Transform& InSceneNodeTransformComp,
            const FFragment_Transform_RootComponent&)
        -> void
    {
        const auto ParentEntity = InParent.Get_Entity().Get_Entity();
        const auto& ParentTransform = InHandle.ReadEntity(ParentEntity).Get<FFragment_Transform>().Get_Transform();
        const auto& MyTransform = InSceneNodeTransformComp.Get_Transform();

        const auto RelativeTransform = MyTransform.GetRelativeTransform(ParentTransform);

        ck_scene_node::TryUpdateRelativeTransform(InHandle, InCurrent, RelativeTransform);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_SceneNode_UpdateLocal_FromMeshSocket::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const SceneNodeParent& InParent,
            FFragment_SceneNode_Current& InCurrent,
            const FFragment_Transform& InSceneNodeTransformComp,
            const FFragment_Transform_MeshSocket&)
        -> void
    {
        const auto ParentEntity = InParent.Get_Entity().Get_Entity();
        const auto& ParentTransform = InHandle.ReadEntity(ParentEntity).Get<FFragment_Transform>().Get_Transform();
        const auto& MyTransform = InSceneNodeTransformComp.Get_Transform();

        const auto RelativeTransform = MyTransform.GetRelativeTransform(ParentTransform);

        ck_scene_node::TryUpdateRelativeTransform(InHandle, InCurrent, RelativeTransform);
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_Layer>
    TProcessor_SceneNode_Update<T_Layer>::
        TProcessor_SceneNode_Update(
        const typename Super::RegistryType& InRegistry)
        : Super(InRegistry)
    { }

    template <typename T_Layer>
    auto
        TProcessor_SceneNode_Update<T_Layer>::
        ForEachEntity(
            typename Super::TimeType InDeltaT,
            typename Super::HandleType InHandle,
            const SceneNodeParent& InParent,
            const FFragment_SceneNode_Current& InCurrent,
            FFragment_Transform& InTransform,
            FFragment_Transform_Previous& InPrevTransform)
        -> void
    {
        const auto HadRelativeTransformUpdatedTag = InHandle.template Has<FTag_SceneNode_RelativeTransformUpdated>();

        const auto ParentEntity = InParent.Get_Entity().Get_Entity();
        auto ReadOnlyParent = InHandle.ReadEntity(ParentEntity);
        const auto ParentHasTransformUpdated = ReadOnlyParent.Has<FTag_Transform_Updated>();

        if (NOT (ParentHasTransformUpdated || HadRelativeTransformUpdatedTag))
        { return; }

        // Clean up the relative-transform-updated tag before the early return below,
        // so root-component/mesh-socket entities don't keep it permanently (which would block probe setup)
        if (HadRelativeTransformUpdatedTag)
        {
            InHandle.template DeferRemove<FTag_SceneNode_RelativeTransformUpdated>();
        }

        // SceneNodes attached to MeshSockets or RootComponents have their transforms managed directly by those processors,
        // so they don't need hierarchical transform updates from the scene graph
        if (InHandle.template Has_Any<FFragment_Transform_MeshSocket, FFragment_Transform_RootComponent>())
        { return; }

        const auto& ParentTransform = ReadOnlyParent.Get<FFragment_Transform>().Get_Transform();
        const auto NewTransform = InCurrent.Get_RelativeTransform() * ParentTransform;

        // PARALLEL-SAFE: direct data write to owned fragments (no structural mutations)
        const auto ComponentsModified = UCk_Utils_Transform_UE::Apply_SetTransform_DirectWrite(
            InTransform, InPrevTransform, NewTransform);

        // Only the tag addition needs deferring (lightweight — empty tag, no data capture)
        if (EnumHasAnyFlags(ComponentsModified,
            ECk_TransformComponents::Location |
            ECk_TransformComponents::Rotation |
            ECk_TransformComponents::Scale))
        {
            InHandle.template DeferAddOrGet<FTag_Transform_Updated>();
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    template TProcessor_SceneNode_Update<FTag_SceneNode_Layer0>;
    template TProcessor_SceneNode_Update<FTag_SceneNode_Layer1>;
    template TProcessor_SceneNode_Update<FTag_SceneNode_Layer2>;
    template TProcessor_SceneNode_Update<FTag_SceneNode_Layer3>;
    template TProcessor_SceneNode_Update<FTag_SceneNode_Layer4>;
    template TProcessor_SceneNode_Update<FTag_SceneNode_Layer5>;
    template TProcessor_SceneNode_Update<FTag_SceneNode_Layer6>;
    template TProcessor_SceneNode_Update<FTag_SceneNode_Layer7>;
    template TProcessor_SceneNode_Update<FTag_SceneNode_Layer8>;
    template TProcessor_SceneNode_Update<FTag_SceneNode_Layer9>;
}

// --------------------------------------------------------------------------------------------------------------------