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

        // cache the world pointer once per tick so ForEachEntity doesn't re-resolve
        // it 100× per frame. Refreshed at the top of each Setup pass.
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
        // Setup-before-consumer guarantee: skip entities that haven't completed
        // Setup yet. Combined with registration order (Setup is registered
        // first in the same group), the SKMC is always valid when a request
        // handler runs.
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
        // One DoHandleRequest overload per request type. C++ overload resolution dispatches
        // from the visitor lambda in ForEachEntity. Mirrors CkIsmProxy_Processor.cpp:396-411.
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

    // gated by FTag_IskmProxy_Movable AND FTag_Transform_Updated. Static proxies
    // (no _Movable tag) are skipped entirely at runtime. Movable proxies that didn't
    // change transform this frame (no _Transform_Updated tag set by CkEcsExt's transform
    // system) are also skipped. Plan-1's old per-frame Equals() guard is gone.
    // TIgnoreInEditor<FTag_IskmProxy_Movable>: in editor worlds the _Movable gate is
    // dropped so editor-placed proxies (which aren't tagged _Movable) still get their
    // transform pushed when moved — mirrors FProcessor_IsmProxy_TransformInstance.
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

    // Runs in FGroup_Transform_Finalize so the follower is ordered after the ENTIRE FGroup_Transform
    // group — not only FProcessor_Transform_HandleRequests (leaders that move via their own transform
    // Request) but also TProcessor_SceneNode_Update, which moves scene-node-CHILD leaders each frame
    // (e.g. a promoted NPC's proxy inherits the agent's per-frame movement through the scene-node
    // hierarchy). The follower reads the leader transform via a runtime lookup the scheduler cannot
    // see, so it has NO view-dependency ordering on either mover; a plain RunAfter HandleRequests (the
    // original lag-free fix) left it racing SceneNode_Update within the group, so a scene-node-driven
    // leader was read one frame stale and the cosmetic trailed the moving body. Finalize still precedes
    // FGroup_PostTransform, so the renderer flush still picks up FTag_Transform_Updated the same frame —
    // see FFragment_IskmProxy_SocketFollower for the composition rationale.
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

    // Companion to SyncTransform (above). That pass runs in Finalize so scene-node-driven LEADERS are
    // fresh — but Finalize is after TProcessor_SceneNode_Update (FGroup_Transform), so scene-node
    // CHILDREN parented under a follower's output (e.g. a held item under a hand attach-point, plus
    // that item's own probe children) would read the follower's FTag_Transform_Updated one group too
    // late: Transform_Cleanup wipes the tag before the next frame's gate check, so after their
    // construct-time one-shot they freeze at the follower's equip-time pose. This pass recomputes the
    // follower's scene-node descendant subtree in place (same composition + anchor-skip contract as
    // TProcessor_SceneNode_Update) right after the follower writes, and runs before SyncToActor so
    // the recomputed poses land on their actors the same frame.
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
