#pragma once

#include "CkNavigation/NavSurface/CkNavSurface_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKNAVIGATION_API FProcessor_NavSurfaceMarkup_HandleRequests : public ck_exp::TProcessor<
        FProcessor_NavSurfaceMarkup_HandleRequests,
        FCk_Handle_NavSurfaceMarkup,
        ck::TReadWrite<FFragment_NavSurfaceMarkup_Requests>,
        ck::TReadWrite<FFragment_NavSurfaceMarkup_Current>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_NavSurfaceMarkup_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_NavSurfaceMarkup_Requests& InRequests,
            FFragment_NavSurfaceMarkup_Current& InCurrent) const -> void;

    private:
        static auto DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_NavSurface_AreaMarkup& InRequest) -> ECk_Request_OperationResult;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKNAVIGATION_API FProcessor_NavSurfaceMarkup_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_NavSurfaceMarkup_CancelPendingRequests,
        FCk_Handle_NavSurfaceMarkup,
        ck::TReadOnly<FFragment_NavSurfaceMarkup_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_NavSurfaceMarkup_Requests& InRequests) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKNAVIGATION_API FProcessor_NavSurfaceMarkup_EndPlay : public ck_exp::TProcessor<
        FProcessor_NavSurfaceMarkup_EndPlay,
        FCk_Handle_NavSurfaceMarkup,
        ck::TReadWrite<FFragment_NavSurfaceMarkup_Current>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_NavSurfaceMarkup_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Turns the provider's revision counter into the neutral OnSurfaceRebuilt signal, and keeps the
    // world's provider fragment's health reading current.
    class CKNAVIGATION_API FProcessor_NavSurface_RevisionWatch : public ck_exp::TProcessor<
        FProcessor_NavSurface_RevisionWatch,
        FCk_Handle,
        ck::TReadWrite<FFragment_NavSurface_Provider>,
        ck::TReadWrite<FFragment_NavSurface_RevisionWatch>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(FCk_Time InDeltaT) -> void;

        static auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_NavSurface_Provider& InProvider,
            FFragment_NavSurface_RevisionWatch& InWatch) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
