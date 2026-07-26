#pragma once

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"
#include "CkEcsExt/Transform/CkTransform_Processor.h"

#include "CkIskmRenderer/Proxy/CkIskmProxy_Fragment.h"
#include "CkIskmRenderer/Proxy/CkIskmProxy_Fragment_Data.h"

namespace ck
{
    class CKISKMRENDERER_API FProcessor_IskmProxy_Setup : public ck_exp::TProcessor<
        FProcessor_IskmProxy_Setup,
        FCk_Handle_IskmProxy,
        TReadOnly<FFragment_IskmProxy_Params>,
        TReadWrite<FFragment_IskmProxy_Current>,
        TReadWrite<FFragment_IskmProxy_AnimState>,
        TReadWrite<FFragment_IskmProxy_PoseSource>,
        TReadWrite<FFragment_IskmProxy_CustomData>,
        TReadWrite<FFragment_IskmProxy_MaterialOverrides>,
        TReadWrite<FFragment_IskmProxy_MorphTargets>,
        FTag_IskmProxy_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using MarkedDirtyBy = FTag_IskmProxy_NeedsSetup;
    public:
        using TProcessor::TProcessor;

        auto
        DoTick(TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IskmProxy_Params& InParams,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& InAnimState,
            FFragment_IskmProxy_PoseSource& InPoseSource,
            FFragment_IskmProxy_CustomData& InCustomData,
            FFragment_IskmProxy_MaterialOverrides& InMaterialOverrides,
            FFragment_IskmProxy_MorphTargets& InMorphTargets) const -> void;

    private:
        mutable TWeakObjectPtr<UWorld> _World;
    };

    class CKISKMRENDERER_API FProcessor_IskmProxy_HandleRequests : public ck_exp::TProcessor<
        FProcessor_IskmProxy_HandleRequests,
        FCk_Handle_IskmProxy,
        TReadOnly<FFragment_IskmProxy_Params>,
        TReadWrite<FFragment_IskmProxy_Current>,
        TReadWrite<FFragment_IskmProxy_AnimState>,
        TReadWrite<FFragment_IskmProxy_PoseSource>,
        TReadWrite<FFragment_IskmProxy_CustomData>,
        TReadWrite<FFragment_IskmProxy_Requests>,
        TReadOnly<FFragment_Transform>,
        // Setup-before-consumer: with Setup registered first in the same group, excluding
        // not-yet-setup entities guarantees a valid SKMC in every request handler.
        TExclude<FTag_IskmProxy_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_IskmProxy_Setup>;
        using MarkedDirtyBy = FFragment_IskmProxy_Requests;
    public:
        using TProcessor::TProcessor;
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IskmProxy_Params& InParams,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& InAnimState,
            FFragment_IskmProxy_PoseSource& InPoseSource,
            FFragment_IskmProxy_CustomData& InCustomData,
            FFragment_IskmProxy_Requests& InRequests,
            const FFragment_Transform& InTransform) const -> void;

    public:
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FFragment_Transform&, const FCk_Request_IskmProxy_PlayAnimation&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FFragment_Transform&, const FCk_Request_IskmProxy_StopAnimation&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FFragment_Transform&, const FCk_Request_IskmProxy_SetPlayRate&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FFragment_Transform&, const FCk_Request_IskmProxy_SetVisibility&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FFragment_Transform&, const FCk_Request_IskmProxy_SetCustomDataFloat&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FFragment_Transform&, const FCk_Request_IskmProxy_SetMaterialOverride&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FFragment_Transform&, const FCk_Request_IskmProxy_ClearMaterialOverrides&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FFragment_Transform&, const FCk_Request_IskmProxy_SetMorphTarget&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FFragment_Transform&, const FCk_Request_IskmProxy_ClearMorphTargets&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FFragment_Transform&, const FCk_Request_IskmProxy_SetSkeletalMesh&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FFragment_Transform&, const FCk_Request_IskmProxy_AttachSubmesh&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FFragment_Transform&, const FCk_Request_IskmProxy_DetachSubmesh&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FFragment_Transform&, const FCk_Request_IskmProxy_DetachAllSubmeshes&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FFragment_Transform&, const FCk_Request_IskmProxy_SetAnimInstanceClass&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FFragment_Transform&, const FCk_Request_IskmProxy_PlayMontage&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FFragment_Transform&, const FCk_Request_IskmProxy_StopMontage&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FFragment_Transform&, const FCk_Request_IskmProxy_BeginRagdoll&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FFragment_Transform&, const FCk_Request_IskmProxy_EndRagdoll&) const -> void;
    };

    // TIgnoreInEditor<FTag_IskmProxy_Movable>: editor worlds drop the _Movable gate so
    // editor-placed proxies (never tagged _Movable) still get their transform pushed.
    class CKISKMRENDERER_API FProcessor_IskmProxy_UpdateTransform : public ck_exp::TProcessor<
        FProcessor_IskmProxy_UpdateTransform,
        FCk_Handle_IskmProxy,
        TReadWrite<FFragment_IskmProxy_Current>,
        TReadOnly<FFragment_Transform>,
        TIgnoreInEditor<FTag_IskmProxy_Movable>,
        FTag_Transform_Updated,
        TExclude<FTag_IskmProxy_Ragdolling>,
        TExclude<FTag_IskmProxy_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
    public:
        using TProcessor::TProcessor;
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_IskmProxy_Current& InCurrent,
            const FFragment_Transform& InTransform) const -> void;
    };

    // The leader transform is read through a runtime lookup the scheduler cannot see, so this pass has
    // NO view-dependency ordering on any mover: it must sit in FGroup_Transform_Finalize, after the
    // WHOLE FGroup_Transform (both request-driven and scene-node-driven leaders). See CkIskmRenderer/CLAUDE.md.
    class CKISKMRENDERER_API FProcessor_IskmProxy_SocketFollower_SyncTransform : public ck_exp::TProcessor<
        FProcessor_IskmProxy_SocketFollower_SyncTransform,
        FCk_Handle_Transform,
        TReadWrite<FFragment_Transform>,
        TReadWrite<FFragment_Transform_Previous>,
        TReadOnly<FFragment_IskmProxy_SocketFollower>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_Finalize;
    public:
        using TProcessor::TProcessor;
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform& InTransform,
            FFragment_Transform_Previous& InPrevTransform,
            const FFragment_IskmProxy_SocketFollower& InFollower) const -> void;
    };

    // Companion to SyncTransform: it writes AFTER TProcessor_SceneNode_Update has run, so scene-node
    // children parented under a follower would miss that frame's tag entirely. This pass recomputes the
    // follower's descendant subtree in place (same contract as TProcessor_SceneNode_Update).
    class CKISKMRENDERER_API FProcessor_IskmProxy_SocketFollower_SyncDescendants : public ck_exp::TProcessor<
        FProcessor_IskmProxy_SocketFollower_SyncDescendants,
        FCk_Handle_Transform,
        TReadOnly<FFragment_Transform>,
        TReadOnly<FFragment_IskmProxy_SocketFollower>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_Finalize;
        using RunAfter = TDepList<FProcessor_IskmProxy_SocketFollower_SyncTransform>;
        using RunBefore = TDepList<FProcessor_Transform_SyncToActor>;
    public:
        using TProcessor::TProcessor;
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_IskmProxy_SocketFollower& InFollower) const -> void;

    private:
        static auto
        DoSync_Descendants(
            FCk_Handle_Transform InParent,
            const FTransform& InParentWorld) -> void;
    };

    class CKISKMRENDERER_API FProcessor_IskmProxy_EmitFinishedEvents : public ck_exp::TProcessor<
        FProcessor_IskmProxy_EmitFinishedEvents,
        FCk_Handle_IskmProxy,
        TReadWrite<FFragment_IskmProxy_Current>,
        TReadWrite<FFragment_IskmProxy_AnimState>,
        TExclude<FTag_IskmProxy_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using RunAfter = TDepList<FProcessor_IskmProxy_UpdateTransform>;
    public:
        using TProcessor::TProcessor;
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& InAnimState) const -> void;
    };

    class FProcessor_IskmProxy_Outline_EndPlay;

    class CKISKMRENDERER_API FProcessor_IskmProxy_EndPlay : public ck_exp::TProcessor<
        FProcessor_IskmProxy_EndPlay,
        FCk_Handle_IskmProxy,
        TReadWrite<FFragment_IskmProxy_Current>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        // The entity-outline teardown must clear custom depth while this proxy still owns its SKMC —
        // release it to the pool only after (Release_BaseSKMC also strips the flags defensively).
        using RunAfter = TDepList<FProcessor_IskmProxy_Outline_EndPlay>;
    public:
        using TProcessor::TProcessor;
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_IskmProxy_Current& InCurrent) const -> void;
    };
}
