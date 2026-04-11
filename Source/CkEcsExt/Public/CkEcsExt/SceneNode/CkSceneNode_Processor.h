#pragma once

#include "CkSceneNode_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkParallelProcessor.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKECSEXT_API FProcessor_SceneNode_HandleRequests : public ck_exp::TProcessor<
            FProcessor_SceneNode_HandleRequests,
            FCk_Handle_SceneNode,
            ck::TReadWrite<FFragment_SceneNode_Current>,
            ck::TReadOnly<FFragment_SceneNode_Requests>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using MarkedDirtyBy = FFragment_SceneNode_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
    	ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_SceneNode_Current& InCurrent,
            const FFragment_SceneNode_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_SceneNode_Current& InCurrent,
            const FCk_Request_SceneNode_UpdateRelativeTransform& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // This processor is tasked with keeping the RelativeTransform data inside the SceneNode fragment up-to-date as the MeshSocket
    // that the SceneNode is attached to moves in the world. The transform processor takes care of the transformation updates
    class CKECSEXT_API FProcessor_SceneNode_UpdateLocal_FromMeshSocket : public TParallelProcessor<
            FProcessor_SceneNode_UpdateLocal_FromMeshSocket,
            FCk_Handle_SceneNode,
            TReadOnly<SceneNodeParent>,
            TReadWrite<FFragment_SceneNode_Current>,
            TReadOnly<FFragment_Transform>,
            TReadOnly<FFragment_Transform_MeshSocket>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_SceneNode_HandleRequests>;

    public:
        using TParallelProcessor::TParallelProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const SceneNodeParent& InParent,
            FFragment_SceneNode_Current& InCurrent,
            const FFragment_Transform& InSceneNodeTransformComp,
            const FFragment_Transform_MeshSocket&) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // This processor is tasked with keeping the RelativeTransform data inside the SceneNode fragment up-to-date as the RootComponent
    // that the SceneNode is attached to moves in the world. The transform processor takes care of the transformation updates
    class CKECSEXT_API FProcessor_SceneNode_UpdateLocal_FromRootComponent : public TParallelProcessor<
            FProcessor_SceneNode_UpdateLocal_FromRootComponent,
            FCk_Handle_SceneNode,
            TReadOnly<SceneNodeParent>,
            TReadWrite<FFragment_SceneNode_Current>,
            TReadOnly<FFragment_Transform>,
            TReadOnly<FFragment_Transform_RootComponent>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_SceneNode_HandleRequests>;

    public:
        using TParallelProcessor::TParallelProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const SceneNodeParent& InParent,
            FFragment_SceneNode_Current& InCurrent,
            const FFragment_Transform& InSceneNodeTransformComp,
            const FFragment_Transform_RootComponent&) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_Layer>
    class CKECSEXT_API TProcessor_SceneNode_Update : public TParallelProcessor<
            TProcessor_SceneNode_Update<T_Layer>,
            FCk_Handle_SceneNode,
            T_Layer,
            TReadOnly<SceneNodeParent>,
            TReadOnly<FFragment_SceneNode_Current>,
            TReadWrite<FFragment_Transform>,
            TReadWrite<FFragment_Transform_Previous>,
            CK_IGNORE_PENDING_KILL>
    {
        using Super = TParallelProcessor<TProcessor_SceneNode_Update<T_Layer>, FCk_Handle_SceneNode, T_Layer,
            TReadOnly<SceneNodeParent>, TReadOnly<FFragment_SceneNode_Current>,
            TReadWrite<FFragment_Transform>, TReadWrite<FFragment_Transform_Previous>,
            CK_IGNORE_PENDING_KILL>;
        using Super::TimeType;
        using Super::HandleType;

    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_SceneNode_UpdateLocal_FromMeshSocket, FProcessor_SceneNode_UpdateLocal_FromRootComponent>;

    public:
        explicit TProcessor_SceneNode_Update(
            const typename Super::RegistryType& InRegistry);

    public:
        static auto
        ForEachEntity(
            typename Super::TimeType InDeltaT,
            typename Super::HandleType InHandle,
            const SceneNodeParent& InParent,
            const FFragment_SceneNode_Current& InCurrent,
            FFragment_Transform& InTransform,
            FFragment_Transform_Previous& InPrevTransform) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
