#pragma once

#include "CkEcs/Processor/CkProcessor.h"
#include "CkOverlapBody/Marker/CkMarker_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKOVERLAPBODY_API FProcessor_Marker_Setup
        : public TProcessor<FProcessor_Marker_Setup, FFragment_Marker_Current, FFragment_Marker_Params, FTag_Marker_Setup>
    {
    public:
        using MarkedDirtyBy = FTag_Marker_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Marker_Current& InCurrentComp,
            const FFragment_Marker_Params& InParamsComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOVERLAPBODY_API FProcessor_Marker_HandleRequests
        : public TProcessor<FProcessor_Marker_HandleRequests, FFragment_Marker_Current, FFragment_Marker_Params, FFragment_Marker_Requests>
    {
    public:
        using MarkedDirtyBy = FFragment_Marker_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InMarkerEntity,
            FFragment_Marker_Current& InCurrentComp,
            const FFragment_Marker_Params& InParamsComp,
            FFragment_Marker_Requests& InRequestsComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOVERLAPBODY_API FProcessor_Marker_UpdateTransform
        : public TProcessor<FProcessor_Marker_UpdateTransform, FFragment_Marker_Current, FFragment_Marker_Params, FTag_Marker_UpdateTransform>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InMarkerEntity,
            FFragment_Marker_Current& InCurrentComp,
            const FFragment_Marker_Params& InParamsComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOVERLAPBODY_API FProcessor_Marker_DebugPreviewAll
    {
    public:
        explicit FProcessor_Marker_DebugPreviewAll(
            FCk_Registry& InRegistry);

    public:
        auto Tick(FCk_Time) -> void;

    private:
        FCk_Registry _Registry;
    };
}

// --------------------------------------------------------------------------------------------------------------------
