#pragma once

#include "CkSceneNode_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcs/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor_SceneNode_HandleRequests : public ck_exp::TProcessor<
            FProcessor_SceneNode_HandleRequests,
            FCk_Handle_SceneNode,
            FFragment_SceneNode_Current,
            FFragment_SceneNode_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
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

    template <typename T_Layer>
    class CKECS_API TProcessor_SceneNode_Update : public ck_exp::TProcessor<
            TProcessor_SceneNode_Update<T_Layer>,
            FCk_Handle_SceneNode,
            T_Layer,
            SceneNodeParent,
            FFragment_SceneNode_Current,
            TExclude<FFragment_Transform_MeshSocket>,
            CK_IGNORE_PENDING_KILL>
    {
        using Super = ck_exp::TProcessor<TProcessor_SceneNode_Update<T_Layer>, FCk_Handle_SceneNode, T_Layer,
            SceneNodeParent, FFragment_SceneNode_Current, TExclude<ck::FFragment_Transform_MeshSocket>, CK_IGNORE_PENDING_KILL>;
        using Super::TimeType;
        using Super::HandleType;

    public:
        explicit TProcessor_SceneNode_Update(
            const typename Super::RegistryType& InRegistry);

    public:
        static auto
        ForEachEntity(
            typename Super::TimeType InDeltaT,
            typename Super::HandleType InHandle,
            const SceneNodeParent& InParent,
            const FFragment_SceneNode_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor_SceneNode_UpdateLocal : public ck_exp::TProcessor<
            FProcessor_SceneNode_UpdateLocal,
            FCk_Handle_SceneNode,
            SceneNodeParent,
            FFragment_SceneNode_Current,
            FFragment_Transform_MeshSocket,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            typename Super::TimeType InDeltaT,
            typename Super::HandleType InHandle,
            const SceneNodeParent& InParent,
            FFragment_SceneNode_Current& InCurrent,
            FFragment_Transform_MeshSocket&) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------