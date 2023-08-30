#pragma once

#include "CkEcs/Processor/CkProcessor.h"
#include "CkOverlapBody/Sensor/CkSensor_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKOVERLAPBODY_API FCk_Processor_Sensor_Setup
        : public TProcessor<FCk_Processor_Sensor_Setup, FCk_Fragment_Sensor_Current, FCk_Fragment_Sensor_Params, FCk_Tag_Sensor_Setup>
    {
    public:
        using MarkedDirtyBy = FCk_Tag_Sensor_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InSensorEntity,
            FCk_Fragment_Sensor_Current& InCurrentComp,
            const FCk_Fragment_Sensor_Params& InParamsComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOVERLAPBODY_API FCk_Processor_Sensor_HandleRequests
        : public TProcessor<FCk_Processor_Sensor_HandleRequests, FCk_Fragment_Sensor_Current, FCk_Fragment_Sensor_Params, FCk_Fragment_Sensor_Requests>
    {
    public:
        using MarkedDirtyBy = FCk_Fragment_Sensor_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FCk_Fragment_Sensor_Current& InCurrentComp,
            const FCk_Fragment_Sensor_Params& InParamsComp,
            FCk_Fragment_Sensor_Requests& InRequestsComp) const -> void;

    private:
        auto DoHandleRequest(
            HandleType InSensorEntity,
            FCk_Fragment_Sensor_Current& InCurrentComp,
            const FCk_Fragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_EnableDisable& InRequest) const -> void;

        auto DoHandleRequest(
            HandleType InSensorEntity,
            FCk_Fragment_Sensor_Current& InCurrentComp,
            const FCk_Fragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_OnBeginOverlap& InRequest) const -> void;

        auto DoHandleRequest(
            HandleType InSensorEntity,
            FCk_Fragment_Sensor_Current& InCurrentComp,
            const FCk_Fragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_OnEndOverlap& InRequest) const -> void;

        auto DoHandleRequest(
            HandleType InSensorEntity,
            FCk_Fragment_Sensor_Current& InCurrentComp,
            const FCk_Fragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_OnBeginOverlap_NonMarker& InRequest) const -> void;

        auto DoHandleRequest(
            HandleType InSensorEntity,
            FCk_Fragment_Sensor_Current& InCurrentComp,
            const FCk_Fragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_OnEndOverlap_NonMarker& InRequest) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOVERLAPBODY_API FCk_Processor_Sensor_UpdateTransform
        : public TProcessor<FCk_Processor_Sensor_UpdateTransform, FCk_Fragment_Sensor_Current, FCk_Fragment_Sensor_Params, FCk_Tag_Sensor_UpdateTransform>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InSensorEntity,
            FCk_Fragment_Sensor_Current& InCurrentComp,
            const FCk_Fragment_Sensor_Params& InParamsComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOVERLAPBODY_API FCk_Processor_Sensor_DebugPreviewAll
    {
    public:
        explicit FCk_Processor_Sensor_DebugPreviewAll(
            FCk_Registry& InRegistry);

    public:
        auto Tick(FCk_Time) -> void;

    private:
        FCk_Registry _Registry;
    };
}

// --------------------------------------------------------------------------------------------------------------------
