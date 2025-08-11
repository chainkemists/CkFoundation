#include "CkSceneNode_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkEcs/Net/CkNet_Utils.h"

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
            Super::TimeType InDeltaT,
            Super::HandleType InHandle,
            const SceneNodeParent& InParent,
            FFragment_SceneNode_Current& InCurrent,
            const FFragment_Transform& InSceneNodeTransformComp,
            const FFragment_Transform_RootComponent&)
        -> void
    {
        const auto& ParentTransform = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(InParent.Get_Entity());
        const auto& MyTransform = InSceneNodeTransformComp.Get_Transform();

        const auto& RelativeTransform = MyTransform.GetRelativeTransform(ParentTransform);

        if (InCurrent.Get_RelativeTransform().Equals(RelativeTransform))
        { return; }

        InCurrent._RelativeTransform = RelativeTransform;
        InHandle.AddOrGet<FTag_SceneNode_RelativeTransformUpdated>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_SceneNode_UpdateLocal_FromMeshSocket::
        ForEachEntity(
            Super::TimeType InDeltaT,
            Super::HandleType InHandle,
            const SceneNodeParent& InParent,
            FFragment_SceneNode_Current& InCurrent,
            const FFragment_Transform& InSceneNodeTransformComp,
            const FFragment_Transform_MeshSocket&)
        -> void
    {
        const auto& ParentTransform = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(InParent.Get_Entity());
        const auto& MyTransform = InSceneNodeTransformComp.Get_Transform();

        const auto& RelativeTransform = MyTransform.GetRelativeTransform(ParentTransform);

        if (InCurrent.Get_RelativeTransform().Equals(RelativeTransform))
        { return; }

        InCurrent._RelativeTransform = RelativeTransform;
        InHandle.AddOrGet<FTag_SceneNode_RelativeTransformUpdated>();
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
            const FFragment_SceneNode_Current& InCurrent)
        -> void
    {
        const auto& HadRelativeTransformUpdatedTag = InHandle.template Try_Remove<FTag_SceneNode_RelativeTransformUpdated>();
        if (NOT (InParent.Get_Entity().Has<FTag_Transform_Updated>() || HadRelativeTransformUpdatedTag))
        { return; }

        // SceneNodes attached to MeshSockets or RootComponents have their transforms managed directly by those processors,
        // so they don't need hierarchical transform updates from the scene graph
        if (InHandle.template Has_Any<FFragment_Transform_MeshSocket, FFragment_Transform_RootComponent>())
        { return; }

        const auto& ParentTransform = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(InParent.Get_Entity());
        const auto NewTransform = InCurrent.Get_RelativeTransform() * ParentTransform;

        UCk_Utils_Transform_TypeUnsafe_UE::Request_SetTransform(InHandle,
            FCk_Request_Transform_SetTransform{NewTransform});

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