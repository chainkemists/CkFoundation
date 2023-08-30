#pragma once

#include "CkEcs/Processor/CkProcessor.h"
#include "CkOverlapBody/Marker/CkMarker_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKOVERLAPBODY_API FCk_Processor_Marker_Setup
        : public TProcessor<FCk_Processor_Marker_Setup, FCk_Fragment_Marker_Current, FCk_Fragment_Marker_Params, FCk_Tag_Marker_Setup>
    {
    public:
        using MarkedDirtyBy = FCk_Tag_Marker_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FCk_Fragment_Marker_Current& InCurrentComp,
            const FCk_Fragment_Marker_Params& InParamsComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOVERLAPBODY_API FCk_Processor_Marker_HandleRequests
        : public TProcessor<FCk_Processor_Marker_HandleRequests, FCk_Fragment_Marker_Current, FCk_Fragment_Marker_Params, FCk_Fragment_Marker_Requests>
    {
    public:
        using MarkedDirtyBy = FCk_Fragment_Marker_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InMarkerEntity,
            FCk_Fragment_Marker_Current& InCurrentComp,
            const FCk_Fragment_Marker_Params& InParamsComp,
            FCk_Fragment_Marker_Requests& InRequestsComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOVERLAPBODY_API FCk_Processor_Marker_UpdateTransform
        : public TProcessor<FCk_Processor_Marker_UpdateTransform, FCk_Fragment_Marker_Current, FCk_Fragment_Marker_Params, FCk_Tag_Marker_UpdateTransform>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InMarkerEntity,
            FCk_Fragment_Marker_Current& InCurrentComp,
            const FCk_Fragment_Marker_Params& InParamsComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOVERLAPBODY_API FCk_Processor_Marker_DebugPreviewAll
    {
    public:
        explicit FCk_Processor_Marker_DebugPreviewAll(
            FCk_Registry& InRegistry);

    public:
        auto Tick(FCk_Time) -> void;

    private:
        FCk_Registry _Registry;
    };
}

// --------------------------------------------------------------------------------------------------------------------
