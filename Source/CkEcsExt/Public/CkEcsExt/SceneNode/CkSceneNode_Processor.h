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
            TExclude<FTag_DestroyEntity_Initiate>,
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

    // HandleRequests excludes owners already tagged for destruction, so a destroyed scene-node's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKECSEXT_API FProcessor_SceneNode_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_SceneNode_CancelPendingRequests,
        FCk_Handle_SceneNode,
        ck::TReadOnly<FFragment_SceneNode_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_SceneNode_Requests& InRequestsComp)
            -> void;
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

    template <typename T_Layer, typename T_Group = FGroup_Transform>
    class TProcessor_SceneNode_Update;

    // Per-layer RunAfter list. Dropping either dependency — Transform_HandleRequests for layer 0, or the
    // layer N-1 chain for the rest — stops motion propagating past the first scene-node link.
    // Full rationale: CkEcsExt/CLAUDE.md § "SceneNode layer ordering".
    template <typename T_Layer, typename T_Group = FGroup_Transform>
    struct TSceneNode_Update_RunAfter
    {
        using type = TDepList<
            FProcessor_SceneNode_FollowUnrealAnchor,
            FProcessor_Transform_HandleRequests>;
    };

    // The late-resolve band's first layer waits on the late drain instead: its parents' poses are the
    // requests that drain landed, not anything the anchor follower produced.
    template <>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer0, FGroup_Transform_LateResolve>
    {
        using type = TDepList<FProcessor_Transform_HandleRequests_LateResolve>;
    };

    template <typename T_Group>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer1, T_Group>
    {
        using type = TDepList<
            FProcessor_SceneNode_FollowUnrealAnchor,
            TProcessor_SceneNode_Update<FTag_SceneNode_Layer0, T_Group>>;
    };

    template <typename T_Group>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer2, T_Group>
    {
        using type = TDepList<
            FProcessor_SceneNode_FollowUnrealAnchor,
            TProcessor_SceneNode_Update<FTag_SceneNode_Layer1, T_Group>>;
    };

    template <typename T_Group>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer3, T_Group>
    {
        using type = TDepList<
            FProcessor_SceneNode_FollowUnrealAnchor,
            TProcessor_SceneNode_Update<FTag_SceneNode_Layer2, T_Group>>;
    };

    template <typename T_Group>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer4, T_Group>
    {
        using type = TDepList<
            FProcessor_SceneNode_FollowUnrealAnchor,
            TProcessor_SceneNode_Update<FTag_SceneNode_Layer3, T_Group>>;
    };

    template <typename T_Group>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer5, T_Group>
    {
        using type = TDepList<
            FProcessor_SceneNode_FollowUnrealAnchor,
            TProcessor_SceneNode_Update<FTag_SceneNode_Layer4, T_Group>>;
    };

    template <typename T_Group>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer6, T_Group>
    {
        using type = TDepList<
            FProcessor_SceneNode_FollowUnrealAnchor,
            TProcessor_SceneNode_Update<FTag_SceneNode_Layer5, T_Group>>;
    };

    template <typename T_Group>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer7, T_Group>
    {
        using type = TDepList<
            FProcessor_SceneNode_FollowUnrealAnchor,
            TProcessor_SceneNode_Update<FTag_SceneNode_Layer6, T_Group>>;
    };

    template <typename T_Group>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer8, T_Group>
    {
        using type = TDepList<
            FProcessor_SceneNode_FollowUnrealAnchor,
            TProcessor_SceneNode_Update<FTag_SceneNode_Layer7, T_Group>>;
    };

    template <typename T_Group>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer9, T_Group>
    {
        using type = TDepList<
            FProcessor_SceneNode_FollowUnrealAnchor,
            TProcessor_SceneNode_Update<FTag_SceneNode_Layer8, T_Group>>;
    };

    // Registered once per layer in FGroup_Transform (the main band) and once per layer in
    // FGroup_Transform_LateResolve (the frame's second resolution point) through the group parameter;
    // both bands share this one body. An entity's layer tag is its depth, so the same node is visited by
    // both bands and the parent-unchanged early-out makes the second visit free wherever nothing moved.
    template <typename T_Layer, typename T_Group>
    class CKECSEXT_API TProcessor_SceneNode_Update : public TParallelProcessor<
            TProcessor_SceneNode_Update<T_Layer, T_Group>,
            FCk_Handle_SceneNode,
            T_Layer,
            TReadOnly<SceneNodeParent>,
            TReadOnly<FFragment_SceneNode_Current>,
            TReadWrite<FFragment_Transform>,
            TReadWrite<FFragment_Transform_Previous>,
            TExclude<FFragment_SceneNode_UnrealAnchor>,
            CK_IGNORE_PENDING_KILL>
    {
        using Super = TParallelProcessor<TProcessor_SceneNode_Update<T_Layer, T_Group>, FCk_Handle_SceneNode, T_Layer,
            TReadOnly<SceneNodeParent>, TReadOnly<FFragment_SceneNode_Current>,
            TReadWrite<FFragment_Transform>, TReadWrite<FFragment_Transform_Previous>,
            TExclude<FFragment_SceneNode_UnrealAnchor>,
            CK_IGNORE_PENDING_KILL>;

    public:
        using Group = T_Group;
        using RunAfter = typename TSceneNode_Update_RunAfter<T_Layer, T_Group>::type;

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
