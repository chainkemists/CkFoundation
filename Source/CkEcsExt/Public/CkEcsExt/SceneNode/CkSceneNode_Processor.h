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

    // Drives a scene-node that follows a foreign Unreal anchor (a USceneComponent or a mesh socket) at the
    // authored FFragment_SceneNode_Current offset: entity.world = offset * anchor.world, every tick, so the node
    // tracks the moving anchor. Read-only w.r.t. the anchor (never writes back — that's why these nodes do NOT
    // carry the Transform module's FFragment_Transform_RootComponent / _MeshSocket, which would engage
    // SyncFromActor/SyncToActor). Runs in the SyncFrom group so descendant TProcessor_SceneNode_Update layers
    // (Transform group) see this node's updated world + FTag_Transform_Updated.
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

    // Per-layer RunAfter list. Each layer depends on the anchor-follow processor,
    // FProcessor_Transform_HandleRequests, AND on the previous layer, so that
    // when a parent's transform changes (e.g. via a tween writing world on the
    // root) the FTag_Transform_Updated added by Transform_HandleRequests is
    // visible to layer 0's gate check, and the deferred tag added by layer N is
    // visible to layer N+1's gate check. Without Transform_HandleRequests in the
    // chain, layer 0 can run in parallel with request handling and miss the tag
    // on the root; without the layer-to-layer chain, descendants miss the tag
    // their parent just deferred. Either gap stops motion from propagating past
    // the first scene-node link.
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
