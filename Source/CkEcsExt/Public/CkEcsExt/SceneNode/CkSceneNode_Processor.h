#pragma once

#include "CkSceneNode_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkParallelProcessor.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Processor.h"

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

    // entity.world = offset * anchor.world, every tick. Read-only w.r.t. the anchor (never writes back).
    // Runs in the SyncFrom group so descendant TProcessor_SceneNode_Update layers (Transform group) see this
    // node's updated world + FTag_Transform_Updated.
    class CKECSEXT_API FProcessor_SceneNode_FollowUnrealAnchor : public TParallelProcessor<
            FProcessor_SceneNode_FollowUnrealAnchor,
            FCk_Handle_SceneNode,
            TReadOnly<FFragment_SceneNode_UnrealAnchor>,
            TReadOnly<FFragment_SceneNode_Current>,
            TReadWrite<FFragment_Transform>,
            TReadWrite<FFragment_Transform_Previous>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_SyncFrom;

    public:
        using TParallelProcessor::TParallelProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_SceneNode_UnrealAnchor& InAnchor,
            const FFragment_SceneNode_Current& InCurrent,
            FFragment_Transform& InTransform,
            FFragment_Transform_Previous& InPrevTransform) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_Layer>
    class TProcessor_SceneNode_Update;

    // Per-layer RunAfter list. Dropping either dependency — Transform_HandleRequests for layer 0, or the
    // layer N-1 chain for the rest — stops motion propagating past the first scene-node link.
    // Full rationale: CkEcsExt/CLAUDE.md § "SceneNode layer ordering".
    template <typename T_Layer>
    struct TSceneNode_Update_RunAfter
    {
        using type = TDepList<
            FProcessor_SceneNode_FollowUnrealAnchor,
            FProcessor_Transform_HandleRequests>;
    };

    template <>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer1>
    {
        using type = TDepList<
            FProcessor_SceneNode_FollowUnrealAnchor,
            TProcessor_SceneNode_Update<FTag_SceneNode_Layer0>>;
    };

    template <>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer2>
    {
        using type = TDepList<
            FProcessor_SceneNode_FollowUnrealAnchor,
            TProcessor_SceneNode_Update<FTag_SceneNode_Layer1>>;
    };

    template <>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer3>
    {
        using type = TDepList<
            FProcessor_SceneNode_FollowUnrealAnchor,
            TProcessor_SceneNode_Update<FTag_SceneNode_Layer2>>;
    };

    template <>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer4>
    {
        using type = TDepList<
            FProcessor_SceneNode_FollowUnrealAnchor,
            TProcessor_SceneNode_Update<FTag_SceneNode_Layer3>>;
    };

    template <>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer5>
    {
        using type = TDepList<
            FProcessor_SceneNode_FollowUnrealAnchor,
            TProcessor_SceneNode_Update<FTag_SceneNode_Layer4>>;
    };

    template <>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer6>
    {
        using type = TDepList<
            FProcessor_SceneNode_FollowUnrealAnchor,
            TProcessor_SceneNode_Update<FTag_SceneNode_Layer5>>;
    };

    template <>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer7>
    {
        using type = TDepList<
            FProcessor_SceneNode_FollowUnrealAnchor,
            TProcessor_SceneNode_Update<FTag_SceneNode_Layer6>>;
    };

    template <>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer8>
    {
        using type = TDepList<
            FProcessor_SceneNode_FollowUnrealAnchor,
            TProcessor_SceneNode_Update<FTag_SceneNode_Layer7>>;
    };

    template <>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer9>
    {
        using type = TDepList<
            FProcessor_SceneNode_FollowUnrealAnchor,
            TProcessor_SceneNode_Update<FTag_SceneNode_Layer8>>;
    };

    template <typename T_Layer>
    class CKECSEXT_API TProcessor_SceneNode_Update : public TParallelProcessor<
            TProcessor_SceneNode_Update<T_Layer>,
            FCk_Handle_SceneNode,
            T_Layer,
            TReadOnly<SceneNodeParent>,
            TReadOnly<FFragment_SceneNode_Current>,
            TReadWrite<FFragment_Transform>,
            TReadWrite<FFragment_Transform_Previous>,
            TExclude<FFragment_SceneNode_UnrealAnchor>,
            CK_IGNORE_PENDING_KILL>
    {
        using Super = TParallelProcessor<TProcessor_SceneNode_Update<T_Layer>, FCk_Handle_SceneNode, T_Layer,
            TReadOnly<SceneNodeParent>, TReadOnly<FFragment_SceneNode_Current>,
            TReadWrite<FFragment_Transform>, TReadWrite<FFragment_Transform_Previous>,
            TExclude<FFragment_SceneNode_UnrealAnchor>,
            CK_IGNORE_PENDING_KILL>;
        using Super::TimeType;
        using Super::HandleType;

    public:
        using Group = FGroup_Transform;
        using RunAfter = typename TSceneNode_Update_RunAfter<T_Layer>::type;

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
