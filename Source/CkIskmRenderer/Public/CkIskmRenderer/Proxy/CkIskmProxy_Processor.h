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

        // B7: cache the world pointer once per tick so ForEachEntity doesn't re-resolve
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
            FFragment_IskmProxy_Requests& InRequests) const -> void;

    public:
        // One DoHandleRequest overload per request type. Declarations added incrementally
        // in Phases F–K alongside each request struct. C++ overload resolution dispatches
        // from the visitor lambda in ForEachEntity. Mirrors CkIsmProxy_Processor.cpp:396-411.
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FCk_Request_IskmProxy_PlayAnimation&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FCk_Request_IskmProxy_StopAnimation&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FCk_Request_IskmProxy_SetPlayRate&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FCk_Request_IskmProxy_SetVisibility&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FCk_Request_IskmProxy_SetCustomDataFloat&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FCk_Request_IskmProxy_SetMaterialOverride&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FCk_Request_IskmProxy_ClearMaterialOverrides&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FCk_Request_IskmProxy_SetMorphTarget&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FCk_Request_IskmProxy_ClearMorphTargets&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FCk_Request_IskmProxy_AttachSubmesh&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FCk_Request_IskmProxy_DetachSubmesh&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FCk_Request_IskmProxy_DetachAllSubmeshes&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FCk_Request_IskmProxy_SetAnimInstanceClass&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FCk_Request_IskmProxy_PlayMontage&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FCk_Request_IskmProxy_StopMontage&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FCk_Request_IskmProxy_BeginRagdoll&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FCk_Request_IskmProxy_EndRagdoll&) const -> void;
    };

    // A3: gated by FTag_IskmProxy_Movable AND FTag_Transform_Updated. Static proxies
    // (no _Movable tag) are skipped entirely. Movable proxies that didn't change
    // transform this frame (no _Transform_Updated tag set by CkEcsExt's transform
    // system) are also skipped. Plan-1's old per-frame Equals() guard is gone.
    class CKISKMRENDERER_API FProcessor_IskmProxy_UpdateTransform : public ck_exp::TProcessor<
        FProcessor_IskmProxy_UpdateTransform,
        FCk_Handle_IskmProxy,
        TReadWrite<FFragment_IskmProxy_Current>,
        FTag_IskmProxy_Movable,
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
            FFragment_IskmProxy_Current& InCurrent) const -> void;
    };

    // Runs AFTER the Transform request pass (same group, explicit dep) so the
    // leader's current-frame movement is already applied when the follower's
    // world transform is composed — see FFragment_IskmProxy_SocketFollower for
    // why a SyncFrom-group processor would trail by one frame of velocity.
    // Marks FTag_Transform_Updated so the renderer flushes (FGroup_PostTransform)
    // pick the new transform up the same frame.
    class CKISKMRENDERER_API FProcessor_IskmProxy_SocketFollower_SyncTransform : public ck_exp::TProcessor<
        FProcessor_IskmProxy_SocketFollower_SyncTransform,
        FCk_Handle_Transform,
        TReadWrite<FFragment_Transform>,
        TReadWrite<FFragment_Transform_Previous>,
        TReadOnly<FFragment_IskmProxy_SocketFollower>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_Transform_HandleRequests>;
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

    class CKISKMRENDERER_API FProcessor_IskmProxy_EndPlay : public ck_exp::TProcessor<
        FProcessor_IskmProxy_EndPlay,
        FCk_Handle_IskmProxy,
        TReadWrite<FFragment_IskmProxy_Current>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
    public:
        using TProcessor::TProcessor;
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_IskmProxy_Current& InCurrent) const -> void;
    };
}
