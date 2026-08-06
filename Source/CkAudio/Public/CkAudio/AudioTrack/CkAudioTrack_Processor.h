#pragma once

#include "CkAudioTrack_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKAUDIO_API FProcessor_AudioTrack_Setup : public ck_exp::TProcessor<
            FProcessor_AudioTrack_Setup,
            FCk_Handle_AudioTrack,
            TReadOnly<FFragment_AudioTrack_Params>,
            TReadOnly<FFragment_AudioTrack_PendingSetup>,
            TReadWrite<FFragment_AudioTrack_Current>,
            TReadWrite<FFragment_AudioTrack_ComponentBindings>,
            FTag_AudioTrack_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using MarkedDirtyBy = FTag_AudioTrack_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioTrack_Params& InParams,
            const FFragment_AudioTrack_PendingSetup& InPendingSetup,
            FFragment_AudioTrack_Current& InCurrent,
            FFragment_AudioTrack_ComponentBindings& InBindings)
            -> void;

    private:
        static auto
        DoBindAudioComponentDelegates(
            HandleType InHandle,
            FFragment_AudioTrack_Current& InCurrent,
            FFragment_AudioTrack_ComponentBindings& InBindings) -> void;

        static auto
        ConvertToAudioTrackState(
            EAudioComponentPlayState InAudioComponentState) -> ECk_AudioTrack_State;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAUDIO_API FProcessor_AudioTrack_HandleRequests : public ck_exp::TProcessor<
            FProcessor_AudioTrack_HandleRequests,
            FCk_Handle_AudioTrack,
            TReadOnly<FFragment_AudioTrack_Params>,
            TReadWrite<FFragment_AudioTrack_Current>,
            TReadOnly<FFragment_AudioTrack_Requests>,
            TExclude<FTag_AudioTrack_NeedsSetup>,
            TExclude<FTag_Transform_Updated>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using RunAfter = TDepList<FProcessor_AudioTrack_Setup>;
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

    // HandleRequests excludes owners already tagged for destruction, so a destroyed track's still-
    // queued requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKAUDIO_API FProcessor_AudioTrack_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_AudioTrack_CancelPendingRequests,
        FCk_Handle_AudioTrack,
        ck::TReadOnly<FFragment_AudioTrack_Requests>,
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
            const FFragment_AudioTrack_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAUDIO_API FProcessor_AudioTrack_Playback : public ck_exp::TProcessor<
            FProcessor_AudioTrack_Playback,
            FCk_Handle_AudioTrack,
            TReadOnly<FFragment_AudioTrack_Params>,
            TReadWrite<FFragment_AudioTrack_Current>,
            FTag_AudioTrack_IsFading,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using RunAfter = TDepList<FProcessor_AudioTrack_HandleRequests>;
        using MarkedDirtyBy = FTag_AudioTrack_IsFading;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioTrack_Params& InParams,
            FFragment_AudioTrack_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAUDIO_API FProcessor_AudioTrack_SpatialUpdate : public ck_exp::TProcessor<
            FProcessor_AudioTrack_SpatialUpdate,
            FCk_Handle_AudioTrack,
            TReadOnly<FFragment_AudioTrack_Current>,
            TReadOnly<FFragment_Transform>,
            FTag_Transform_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using RunAfter = TDepList<FProcessor_AudioTrack_HandleRequests>;
        using MarkedDirtyBy = FTag_Transform_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioTrack_Current& InCurrent,
            const FFragment_Transform& InTransform)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAUDIO_API FProcessor_AudioTrack_EndPlay : public ck_exp::TProcessor<
            FProcessor_AudioTrack_EndPlay,
            FCk_Handle_AudioTrack,
            TReadOnly<FFragment_AudioTrack_Params>,
            TReadWrite<FFragment_AudioTrack_Current>,
            TReadWrite<FFragment_AudioTrack_ComponentBindings>,
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
            const FFragment_AudioTrack_Params& InParams,
            FFragment_AudioTrack_Current& InCurrent,
            FFragment_AudioTrack_ComponentBindings& InBindings)
            -> void;

        static auto
        DoUnbindAudioComponentDelegates(
            FFragment_AudioTrack_Current& InCurrent,
            FFragment_AudioTrack_ComponentBindings& InBindings) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKAUDIO_API FProcessor_AudioTrack_DebugDraw_Individual_Spatial : public ck_exp::TProcessor<
            FProcessor_AudioTrack_DebugDraw_Individual_Spatial,
            FCk_Handle_AudioTrack,
            TReadOnly<FFragment_AudioTrack_Params>,
            TReadOnly<FFragment_AudioTrack_Current>,
            TReadWrite<FFragment_AudioTrack_Debug>,
            TReadOnly<FFragment_Transform>,
            FTag_AudioTrack_DebugDraw,
            TExclude<FTag_AudioTrack_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            const HandleType& InHandle,
            const FFragment_AudioTrack_Params& InParams,
            const FFragment_AudioTrack_Current& InCurrent,
            FFragment_AudioTrack_Debug& InDebug,
            const FFragment_Transform& InTransform)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAUDIO_API FProcessor_AudioTrack_DebugDraw_Individual_NonSpatial : public ck_exp::TProcessor<
            FProcessor_AudioTrack_DebugDraw_Individual_NonSpatial,
            FCk_Handle_AudioTrack,
            TReadOnly<FFragment_AudioTrack_Params>,
            TReadOnly<FFragment_AudioTrack_Current>,
            TReadWrite<FFragment_AudioTrack_Debug>,
            FTag_AudioTrack_DebugDraw,
            TExclude<FFragment_Transform>,
            TExclude<FTag_AudioTrack_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        // The 4 DebugDraw processors all write FFragment_AudioTrack_Debug (disjoint entity sets).
        // Chain them in declaration order to make the deterministic write-ordering explicit.
        using RunAfter = TDepList<FProcessor_AudioTrack_DebugDraw_Individual_Spatial>;

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
            TReadOnly<FFragment_AudioTrack_Params>,
            TReadOnly<FFragment_AudioTrack_Current>,
            TReadWrite<FFragment_AudioTrack_Debug>,
            TReadOnly<FFragment_Transform>,
            TExclude<FTag_AudioTrack_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using RunAfter = TDepList<FProcessor_AudioTrack_DebugDraw_Individual_NonSpatial>;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            const HandleType& InHandle,
            const FFragment_AudioTrack_Params& InParams,
            const FFragment_AudioTrack_Current& InCurrent,
            FFragment_AudioTrack_Debug& InDebug,
            const FFragment_Transform& InTransform)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAUDIO_API FProcessor_AudioTrack_DebugDraw_All_NonSpatial : public ck_exp::TProcessor<
            FProcessor_AudioTrack_DebugDraw_All_NonSpatial,
            FCk_Handle_AudioTrack,
            TReadOnly<FFragment_AudioTrack_Params>,
            TReadOnly<FFragment_AudioTrack_Current>,
            TReadWrite<FFragment_AudioTrack_Debug>,
            TExclude<FFragment_Transform>,
            TExclude<FTag_AudioTrack_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using RunAfter = TDepList<FProcessor_AudioTrack_DebugDraw_All_Spatial>;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioTrack_Params& InParams,
            const FFragment_AudioTrack_Current& InCurrent,
            FFragment_AudioTrack_Debug& InDebug)
            -> void;

    private:
        mutable int32 _NonSpatialSlotCounter = 0;
        mutable TArray<FCk_Entity> _TracksToProcess;
    };
}

// --------------------------------------------------------------------------------------------------------------------
