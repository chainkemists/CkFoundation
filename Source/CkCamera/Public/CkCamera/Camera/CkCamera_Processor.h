#pragma once

#include "CkCamera/Camera/CkCamera_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ----------------------------------------------------------------------------------------------------------------
    class CKCAMERA_API FProcessor_Camera_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Camera_HandleRequests,
            FCk_Handle_Camera,
            TReadOnly<FFragment_Camera_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Camera;
        using MarkedDirtyBy = FFragment_Camera_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Camera_Requests& InRequestsComp) const -> void;

    private:
        static auto DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_Camera_AddLayer& InRequest) -> bool;

        static auto DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_Camera_RemoveLayer& InRequest) -> bool;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed camera's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKCAMERA_API FProcessor_Camera_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_Camera_CancelPendingRequests,
        FCk_Handle_Camera,
        ck::TReadOnly<FFragment_Camera_Requests>,
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
            const FFragment_Camera_Requests& InRequestsComp)
            -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // Does NOT advance the blend alpha — FProcessor_CameraLayer_Blend (FGroup_Gameplay_TimeDelta) owns that, running
    // earlier in the frame so the tuner attributes are recomputed before the POV reads them.
    // ----------------------------------------------------------------------------------------------------------------
    class CKCAMERA_API FProcessor_CameraLayer_Lifecycle : public ck_exp::TProcessor<
            FProcessor_CameraLayer_Lifecycle,
            FCk_Handle_Camera,
            TReadWrite<FFragment_Camera_Current>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group    = FGroup_Gameplay_Camera;
        using RunAfter = TDepList<FProcessor_Camera_HandleRequests>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Camera_Current& InCurrent) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------
    class CKCAMERA_API FProcessor_Camera_UpdatePOV : public ck_exp::TProcessor<
            FProcessor_Camera_UpdatePOV,
            FCk_Handle_Camera,
            TReadWrite<FFragment_Camera_Current>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group    = FGroup_Gameplay_Camera;
        using RunAfter = TDepList<FProcessor_CameraLayer_Lifecycle>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Camera_Current& InCurrent) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
