#include "CkSceneNode_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkEcsExt/Transform/CkTransform_RestoreRebase.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "Components/SceneComponent.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_SceneNode_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_SceneNode_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_SceneNode_FollowUnrealAnchor);
CK_REGISTER_PROCESSOR(ck::TProcessor_SceneNode_Update<ck::FTag_SceneNode_Layer0>);
CK_REGISTER_PROCESSOR(ck::TProcessor_SceneNode_Update<ck::FTag_SceneNode_Layer1>);
CK_REGISTER_PROCESSOR(ck::TProcessor_SceneNode_Update<ck::FTag_SceneNode_Layer2>);
CK_REGISTER_PROCESSOR(ck::TProcessor_SceneNode_Update<ck::FTag_SceneNode_Layer3>);
CK_REGISTER_PROCESSOR(ck::TProcessor_SceneNode_Update<ck::FTag_SceneNode_Layer4>);
CK_REGISTER_PROCESSOR(ck::TProcessor_SceneNode_Update<ck::FTag_SceneNode_Layer5>);
CK_REGISTER_PROCESSOR(ck::TProcessor_SceneNode_Update<ck::FTag_SceneNode_Layer6>);
CK_REGISTER_PROCESSOR(ck::TProcessor_SceneNode_Update<ck::FTag_SceneNode_Layer7>);
CK_REGISTER_PROCESSOR(ck::TProcessor_SceneNode_Update<ck::FTag_SceneNode_Layer8>);
CK_REGISTER_PROCESSOR(ck::TProcessor_SceneNode_Update<ck::FTag_SceneNode_Layer9>);

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
                // DoHandleRequest is void and has no rejection path, so reaching the line after the
                // call IS the success condition.
                auto Result = ECk_Request_OperationResult::Failed;
                const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

                DoHandleRequest(InHandle, InCurrent, InRequest);

                Result = ECk_Request_OperationResult::Succeeded;

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
        FProcessor_SceneNode_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_SceneNode_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_SceneNode_FollowUnrealAnchor::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_SceneNode_UnrealAnchor& InAnchor,
            const FFragment_SceneNode_Current& InCurrent,
            FFragment_Transform& InTransform,
            FFragment_Transform_Previous& InPrevTransform)
        -> void
    {
        // This node is excluded from TProcessor_SceneNode_Update (which is where bare/parent nodes strip
        // this tag), so clean it up here — a permanently-set tag would block downstream probe setup.
        if (InHandle.template Has<FTag_SceneNode_RelativeTransformUpdated>())
        { InHandle.template DeferRemove<FTag_SceneNode_RelativeTransformUpdated>(); }

        const auto& Component = InAnchor.Get_Component();

        if (ck::Is_NOT_Valid(Component))
        { return; }

        const auto AnchorWorld = InAnchor.Get_Socket().IsNone()
            ? Component->GetComponentTransform()
            : Component->GetSocketTransform(InAnchor.Get_Socket());

        const auto NewTransform = InCurrent.Get_RelativeTransform() * AnchorWorld;

        if (InTransform.Get_Transform().Equals(NewTransform))
        { return; }

        const auto ComponentsModified = UCk_Utils_Transform_UE::Apply_SetTransform_DirectWrite(
            InTransform, InPrevTransform, NewTransform);

        if (EnumHasAnyFlags(ComponentsModified,
            ECk_TransformComponents::Location |
            ECk_TransformComponents::Rotation |
            ECk_TransformComponents::Scale))
        {
            InHandle.template DeferAddOrGet<FTag_Transform_Updated>();
        }
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
        const auto ParentHasTransformUpdated = ReadOnlyParent.template Has<FTag_Transform_Updated>();
        const auto ParentHasRestoreRebase = ReadOnlyParent.template Has<FTag_Transform_RestoreRebase>();

        if (NOT (ParentHasTransformUpdated || HadRelativeTransformUpdatedTag))
        { return; }

        // Cleared BEFORE the early return below, so root-component/mesh-socket entities don't keep it
        // permanently (which would block probe setup)
        if (HadRelativeTransformUpdatedTag)
        {
            InHandle.template DeferRemove<FTag_SceneNode_RelativeTransformUpdated>();
        }

        // Anchored nodes are anchor-authoritative unless FTag_Transform_ExternallyDriven flips the contract.
        // Not expressible as a view include/exclude: bare entities (no anchor, no tag) must still process.
        if (InHandle.template Has_Any<FFragment_Transform_MeshSocket, FFragment_Transform_RootComponent>() &&
            NOT InHandle.template Has<FTag_Transform_ExternallyDriven>())
        { return; }

        const auto& ParentTransform = ReadOnlyParent.template Get<FFragment_Transform>().Get_Transform();
        const auto NewTransform = InCurrent.Get_RelativeTransform() * ParentTransform;

        const auto ComponentsModified = UCk_Utils_Transform_UE::Apply_SetTransform_DirectWrite(
            InTransform, InPrevTransform, NewTransform);

        if (EnumHasAnyFlags(ComponentsModified,
            ECk_TransformComponents::Location |
            ECk_TransformComponents::Rotation |
            ECk_TransformComponents::Scale))
        {
            InHandle.template DeferAddOrGet<FTag_Transform_Updated>();

            if (ParentHasRestoreRebase)
            {
                InHandle.template DeferAddOrGet<FTag_Transform_RestoreRebase>();
            }
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
