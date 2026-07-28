#pragma once

#include "CkCameraShake_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKCAMERA_API FProcessor_CameraShake_HandleRequests : public ck_exp::TProcessor<
            FProcessor_CameraShake_HandleRequests,
            FCk_Handle_CameraShake,
            TReadWrite<FFragment_CameraShake_Params>,
            TReadOnly<FFragment_CameraShake_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using MarkedDirtyBy = FFragment_CameraShake_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_CameraShake_Params& InParams,
            const FFragment_CameraShake_Requests& InRequestsComp) const -> void;

    private:
        auto DoHandleRequest(
            HandleType InHandle,
            const FFragment_CameraShake_Params& InParams,
            const FCk_Request_CameraShake_PlayOnTarget& InRequest) const -> bool;

        auto DoHandleRequest(
            HandleType InHandle,
            const FFragment_CameraShake_Params& InParams,
            const FCk_Request_CameraShake_PlayAtLocation& InRequest) const -> bool;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed camera shake's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKCAMERA_API FProcessor_CameraShake_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_CameraShake_CancelPendingRequests,
        FCk_Handle_CameraShake,
        ck::TReadOnly<FFragment_CameraShake_Requests>,
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
            const FFragment_CameraShake_Requests& InRequestsComp)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
