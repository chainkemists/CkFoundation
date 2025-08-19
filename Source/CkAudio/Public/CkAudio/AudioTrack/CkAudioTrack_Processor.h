#pragma once

#include "CkAudioTrack_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKAUDIO_API FProcessor_AudioTrack_Setup : public ck_exp::TProcessor<
            FProcessor_AudioTrack_Setup,
            FCk_Handle_AudioTrack,
            FFragment_AudioTrack_Params,
            FFragment_AudioTrack_Current,
            FTag_AudioTrack_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_AudioTrack_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioTrack_Params& InParams,
            FFragment_AudioTrack_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAUDIO_API FProcessor_AudioTrack_HandleRequests : public ck_exp::TProcessor<
            FProcessor_AudioTrack_HandleRequests,
            FCk_Handle_AudioTrack,
            FFragment_AudioTrack_Params,
            FFragment_AudioTrack_Current,
            FFragment_AudioTrack_Requests,
            TExclude<FTag_AudioTrack_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_AudioTrack_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioTrack_Params& InParams,
            FFragment_AudioTrack_Current& InCurrent,
            const FFragment_AudioTrack_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_AudioTrack_Params& InParams,
            FFragment_AudioTrack_Current& InCurrent,
            const FCk_Request_AudioTrack_Play& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_AudioTrack_Params& InParams,
            FFragment_AudioTrack_Current& InCurrent,
            const FCk_Request_AudioTrack_Stop& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_AudioTrack_Params& InParams,
            FFragment_AudioTrack_Current& InCurrent,
            const FCk_Request_AudioTrack_SetVolume& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAUDIO_API FProcessor_AudioTrack_Playback : public ck_exp::TProcessor<
            FProcessor_AudioTrack_Playback,
            FCk_Handle_AudioTrack,
            FFragment_AudioTrack_Params,
            FFragment_AudioTrack_Current,
            FTag_AudioTrack_IsFading,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_AudioTrack_IsFading;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioTrack_Params& InParams,
            FFragment_AudioTrack_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAUDIO_API FProcessor_AudioTrack_SpatialUpdate : public ck_exp::TProcessor<
            FProcessor_AudioTrack_SpatialUpdate,
            FCk_Handle_AudioTrack,
            FFragment_AudioTrack_Current,
            FFragment_Transform,
            FTag_Transform_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Transform_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioTrack_Current& InCurrent,
            const FFragment_Transform& InTransform) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAUDIO_API FProcessor_AudioTrack_Teardown : public ck_exp::TProcessor<
            FProcessor_AudioTrack_Teardown,
            FCk_Handle_AudioTrack,
            FFragment_AudioTrack_Params,
            FFragment_AudioTrack_Current,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioTrack_Params& InParams,
            FFragment_AudioTrack_Current& InCurrent) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKAUDIO_API FProcessor_AudioTrack_DebugDraw_Individual_Spatial : public ck_exp::TProcessor<
            FProcessor_AudioTrack_DebugDraw_Individual_Spatial,
            FCk_Handle_AudioTrack,
            FFragment_AudioTrack_Params,
            FFragment_AudioTrack_Current,
            FFragment_AudioTrack_Debug,
            FFragment_Transform,
            FTag_AudioTrack_DebugDraw,
            TExclude<FTag_AudioTrack_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            const HandleType& InHandle,
            const FFragment_AudioTrack_Params& InParams,
            const FFragment_AudioTrack_Current& InCurrent,
            FFragment_AudioTrack_Debug& InDebug,
            const FFragment_Transform& InTransform) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAUDIO_API FProcessor_AudioTrack_DebugDraw_Individual_NonSpatial : public ck_exp::TProcessor<
            FProcessor_AudioTrack_DebugDraw_Individual_NonSpatial,
            FCk_Handle_AudioTrack,
            FFragment_AudioTrack_Params,
            FFragment_AudioTrack_Current,
            FFragment_AudioTrack_Debug,
            FTag_AudioTrack_DebugDraw,
            TExclude<FFragment_Transform>,
            TExclude<FTag_AudioTrack_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            const HandleType& InHandle,
            const FFragment_AudioTrack_Params& InParams,
            const FFragment_AudioTrack_Current& InCurrent,
            FFragment_AudioTrack_Debug& InDebug) const -> void;

    private:
        mutable int32 _NonSpatialSlotCounter = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAUDIO_API FProcessor_AudioTrack_DebugDraw_All_Spatial : public ck_exp::TProcessor<
            FProcessor_AudioTrack_DebugDraw_All_Spatial,
            FCk_Handle_AudioTrack,
            FFragment_AudioTrack_Params,
            FFragment_AudioTrack_Current,
            FFragment_AudioTrack_Debug,
            FFragment_Transform,
            TExclude<FTag_AudioTrack_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            const HandleType& InHandle,
            const FFragment_AudioTrack_Params& InParams,
            const FFragment_AudioTrack_Current& InCurrent,
            FFragment_AudioTrack_Debug& InDebug,
            const FFragment_Transform& InTransform) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAUDIO_API FProcessor_AudioTrack_DebugDraw_All_NonSpatial : public ck_exp::TProcessor<
            FProcessor_AudioTrack_DebugDraw_All_NonSpatial,
            FCk_Handle_AudioTrack,
            FFragment_AudioTrack_Params,
            FFragment_AudioTrack_Current,
            FFragment_AudioTrack_Debug,
            TExclude<FFragment_Transform>,
            TExclude<FTag_AudioTrack_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioTrack_Params& InParams,
            const FFragment_AudioTrack_Current& InCurrent,
            FFragment_AudioTrack_Debug& InDebug) const -> void;

    private:
        mutable int32 _NonSpatialSlotCounter = 0;
        mutable TArray<FCk_Entity> _TracksToProcess;
    };
}

// --------------------------------------------------------------------------------------------------------------------
