#pragma once

#include "CkEcs/Processor/CkProcessor.h"
#include "CkOverlapBody/Sensor/CkSensor_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKOVERLAPBODY_API FProcessor_Sensor_Setup
        : public TProcessor<FProcessor_Sensor_Setup, FFragment_Sensor_Current, FFragment_Sensor_Params, FTag_Sensor_Setup>
    {
    public:
        using MarkedDirtyBy = FTag_Sensor_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InSensorEntity,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOVERLAPBODY_API FProcessor_Sensor_HandleRequests
        : public TProcessor<FProcessor_Sensor_HandleRequests, FFragment_Sensor_Current, FFragment_Sensor_Params, FFragment_Sensor_Requests>
    {
    public:
        using MarkedDirtyBy = FFragment_Sensor_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp,
            FFragment_Sensor_Requests& InRequestsComp) const -> void;

    private:
        auto DoHandleRequest(
            HandleType InSensorEntity,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_EnableDisable& InRequest) const -> void;

        auto DoHandleRequest(
            HandleType InSensorEntity,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_OnBeginOverlap& InRequest) const -> void;

        auto DoHandleRequest(
            HandleType InSensorEntity,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_OnEndOverlap& InRequest) const -> void;

        auto DoHandleRequest(
            HandleType InSensorEntity,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_OnBeginOverlap_NonMarker& InRequest) const -> void;

        auto DoHandleRequest(
            HandleType InSensorEntity,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_OnEndOverlap_NonMarker& InRequest) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOVERLAPBODY_API FProcessor_Sensor_UpdateTransform
        : public TProcessor<FProcessor_Sensor_UpdateTransform, FFragment_Sensor_Current, FFragment_Sensor_Params, FTag_Sensor_UpdateTransform>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InSensorEntity,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOVERLAPBODY_API FProcessor_Sensor_DebugPreviewAll
    {
    public:
        explicit FProcessor_Sensor_DebugPreviewAll(
            FCk_Registry& InRegistry);

    public:
        auto Tick(FCk_Time) -> void;

    private:
        FCk_Registry _Registry;
    };
}

// --------------------------------------------------------------------------------------------------------------------
