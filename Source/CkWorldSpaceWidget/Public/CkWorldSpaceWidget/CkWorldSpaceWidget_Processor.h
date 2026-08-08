#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkWorldSpaceWidget/CkWorldSpaceWidget_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKWORLDSPACEWIDGET_API FProcessor_WorldSpaceWidget_UpdateLocation : public ck_exp::TProcessor<
            FProcessor_WorldSpaceWidget_UpdateLocation,
            FCk_Handle_WorldSpaceWidget,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_WorldSpaceWidget_Params>,
            ck::TReadOnly<FFragment_WorldSpaceWidget_Current>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_WorldSpaceWidget_Params& InParams,
            const FFragment_WorldSpaceWidget_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKWORLDSPACEWIDGET_API FProcessor_WorldSpaceWidget_UpdateScaling : public ck_exp::TProcessor<
            FProcessor_WorldSpaceWidget_UpdateScaling,
            FCk_Handle_WorldSpaceWidget,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_WorldSpaceWidget_Params>,
            ck::TReadOnly<FFragment_WorldSpaceWidget_Current>,
            FTag_WorldSpaceWidget_NeedsUpdateScaling,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using RunAfter = TDepList<FProcessor_WorldSpaceWidget_UpdateLocation>;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_WorldSpaceWidget_Params& InParams,
            const FFragment_WorldSpaceWidget_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKWORLDSPACEWIDGET_API FProcessor_WorldSpaceWidget_HandleRequests : public ck_exp::TProcessor<
            FProcessor_WorldSpaceWidget_HandleRequests,
            FCk_Handle_WorldSpaceWidget,
            ck::TReadWrite<FFragment_WorldSpaceWidget_Current>,
            ck::TReadWrite<FFragment_WorldSpaceWidget_Params>,
            ck::TReadWrite<FFragment_WorldSpaceWidget_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using RunBefore = TDepList<FProcessor_WorldSpaceWidget_UpdateLocation, FProcessor_WorldSpaceWidget_UpdateScaling>;
        using MarkedDirtyBy = FFragment_WorldSpaceWidget_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_WorldSpaceWidget_Current& InCurrent,
            FFragment_WorldSpaceWidget_Params& InParams,
            FFragment_WorldSpaceWidget_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_WorldSpaceWidget_Current& InCurrent,
            FFragment_WorldSpaceWidget_Params& InParams,
            const FCk_Request_WorldSpaceWidget_SetLocationInfo& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_WorldSpaceWidget_Current& InCurrent,
            FFragment_WorldSpaceWidget_Params& InParams,
            const FCk_Request_WorldSpaceWidget_SetScalingInfo& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_WorldSpaceWidget_Current& InCurrent,
            FFragment_WorldSpaceWidget_Params& InParams,
            const FCk_Request_WorldSpaceWidget_SetFadingInfo& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_WorldSpaceWidget_Current& InCurrent,
            FFragment_WorldSpaceWidget_Params& InParams,
            const FCk_Request_WorldSpaceWidget_SetOcclusionInfo& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed widget's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKWORLDSPACEWIDGET_API FProcessor_WorldSpaceWidget_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_WorldSpaceWidget_CancelPendingRequests,
        FCk_Handle_WorldSpaceWidget,
        ck::TReadOnly<FFragment_WorldSpaceWidget_Requests>,
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
            const FFragment_WorldSpaceWidget_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKWORLDSPACEWIDGET_API FProcessor_WorldSpaceWidget_EndPlay : public ck_exp::TProcessor<
            FProcessor_WorldSpaceWidget_EndPlay,
            FCk_Handle_WorldSpaceWidget,
            ck::TReadOnly<FFragment_WorldSpaceWidget_Params>,
            ck::TReadWrite<FFragment_WorldSpaceWidget_Current>,
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
            const FFragment_WorldSpaceWidget_Params& InParams,
            FFragment_WorldSpaceWidget_Current& InCurrent) -> void;
    };
}
