#pragma once

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

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
            FFragment_IskmProxy_CustomData& InCustomData) const -> void;

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
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
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
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FCk_Request_IskmProxy_PlayMontage&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FCk_Request_IskmProxy_StopMontage&) const -> void;
        auto DoHandleRequest(HandleType& InHandle, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&, FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&, FFragment_IskmProxy_CustomData&, const FCk_Request_IskmProxy_BeginRagdoll&) const -> void;
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

    class CKISKMRENDERER_API FProcessor_IskmProxy_EmitFinishedEvents : public ck_exp::TProcessor<
        FProcessor_IskmProxy_EmitFinishedEvents,
        FCk_Handle_IskmProxy,
        TReadWrite<FFragment_IskmProxy_Current>,
        TReadWrite<FFragment_IskmProxy_AnimState>,
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
