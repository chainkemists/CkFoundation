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

    class CKECSEXT_API FProcessor_SceneNode_QueueRootChildren : public ck_exp::TProcessor<
            FProcessor_SceneNode_QueueRootChildren,
            FCk_Handle_Transform,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_RecordOfSceneNodes>,
            FTag_Transform_Updated,
            TExclude<FTag_SceneNode_Layer0>,
            TExclude<FTag_SceneNode_Layer1>,
            TExclude<FTag_SceneNode_Layer2>,
            TExclude<FTag_SceneNode_Layer3>,
            TExclude<FTag_SceneNode_Layer4>,
            TExclude<FTag_SceneNode_Layer5>,
            TExclude<FTag_SceneNode_Layer6>,
            TExclude<FTag_SceneNode_Layer7>,
            TExclude<FTag_SceneNode_Layer8>,
            TExclude<FTag_SceneNode_Layer9>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<
            FProcessor_SceneNode_FollowUnrealAnchor,
            FProcessor_Transform_HandleRequests>;
        using LocalSettleAfter = FGroup_Transform_Derived;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_RecordOfSceneNodes& InChildren) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_Layer>
    class TProcessor_SceneNode_Update;

    template <typename T_Layer>
    class TProcessor_SceneNode_QueueChildren;

    // Per-layer RunAfter list. Dropping either dependency — Transform_HandleRequests for layer 0, or the
    // layer N-1 chain for the rest — stops motion propagating past the first scene-node link.
    // Full rationale: CkEcsExt/CLAUDE.md § "SceneNode layer ordering".
    template <typename T_Layer>
    struct TSceneNode_Update_RunAfter
    {
        using type = TDepList<
            FProcessor_SceneNode_HandleRequests,
            FProcessor_SceneNode_QueueRootChildren>;
    };

    template <>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer1>
    {
        using type = TDepList<TProcessor_SceneNode_QueueChildren<FTag_SceneNode_Layer0>>;
    };

    template <>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer2>
    {
        using type = TDepList<TProcessor_SceneNode_QueueChildren<FTag_SceneNode_Layer1>>;
    };

    template <>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer3>
    {
        using type = TDepList<TProcessor_SceneNode_QueueChildren<FTag_SceneNode_Layer2>>;
    };

    template <>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer4>
    {
        using type = TDepList<TProcessor_SceneNode_QueueChildren<FTag_SceneNode_Layer3>>;
    };

    template <>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer5>
    {
        using type = TDepList<TProcessor_SceneNode_QueueChildren<FTag_SceneNode_Layer4>>;
    };

    template <>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer6>
    {
        using type = TDepList<TProcessor_SceneNode_QueueChildren<FTag_SceneNode_Layer5>>;
    };

    template <>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer7>
    {
        using type = TDepList<TProcessor_SceneNode_QueueChildren<FTag_SceneNode_Layer6>>;
    };

    template <>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer8>
    {
        using type = TDepList<TProcessor_SceneNode_QueueChildren<FTag_SceneNode_Layer7>>;
    };

    template <>
    struct TSceneNode_Update_RunAfter<FTag_SceneNode_Layer9>
    {
        using type = TDepList<TProcessor_SceneNode_QueueChildren<FTag_SceneNode_Layer8>>;
    };

    // The canonical depth chain runs in FGroup_Transform, then may be replayed in the same order by the
    // scheduler's local settle barrier after derived transform producers.
    template <typename T_Layer>
    class CKECSEXT_API TProcessor_SceneNode_Update : public TParallelProcessor<
            TProcessor_SceneNode_Update<T_Layer>,
            FCk_Handle_SceneNode,
            T_Layer,
            TReadOnly<SceneNodeParent>,
            TReadOnly<FFragment_SceneNode_Current>,
            TReadWrite<FFragment_Transform>,
            TReadWrite<FFragment_Transform_Previous>,
            TReadOnly<FFragment_SceneNode_PropagationState>,
            FTag_SceneNode_PropagationQueued,
            TExclude<FFragment_SceneNode_UnrealAnchor>,
            CK_IGNORE_PENDING_KILL>
    {
        using Super = TParallelProcessor<TProcessor_SceneNode_Update<T_Layer>, FCk_Handle_SceneNode, T_Layer,
            TReadOnly<SceneNodeParent>, TReadOnly<FFragment_SceneNode_Current>,
            TReadWrite<FFragment_Transform>, TReadWrite<FFragment_Transform_Previous>,
            TReadOnly<FFragment_SceneNode_PropagationState>, FTag_SceneNode_PropagationQueued,
            TExclude<FFragment_SceneNode_UnrealAnchor>,
            CK_IGNORE_PENDING_KILL>;

    public:
        using Group = FGroup_Transform;
        using RunAfter = typename TSceneNode_Update_RunAfter<T_Layer>::type;
        using MarkedDirtyBy = FTag_SceneNode_PropagationQueued;
        using LocalSettleAfter = FGroup_Transform_Derived;

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
            FFragment_Transform_Previous& InPrevTransform,
            const FFragment_SceneNode_PropagationState& InPropagationState) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_Layer>
    class CKECSEXT_API TProcessor_SceneNode_QueueChildren : public ck_exp::TProcessor<
            TProcessor_SceneNode_QueueChildren<T_Layer>,
            FCk_Handle_Transform,
            T_Layer,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_RecordOfSceneNodes>,
            FTag_Transform_Updated,
            CK_IGNORE_PENDING_KILL>
    {
        using Super = ck_exp::TProcessor<
            TProcessor_SceneNode_QueueChildren<T_Layer>, FCk_Handle_Transform, T_Layer,
            ck::TReadOnly<FFragment_Transform>, ck::TReadOnly<FFragment_RecordOfSceneNodes>,
            FTag_Transform_Updated, CK_IGNORE_PENDING_KILL>;

    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<TProcessor_SceneNode_Update<T_Layer>>;
        using LocalSettleAfter = FGroup_Transform_Derived;

    public:
        using Super::Super;

    public:
        static auto
        ForEachEntity(
            typename Super::TimeType InDeltaT,
            typename Super::HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_RecordOfSceneNodes& InChildren) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
