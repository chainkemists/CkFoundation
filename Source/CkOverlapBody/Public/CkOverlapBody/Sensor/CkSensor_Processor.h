#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkOverlapBody/Sensor/CkSensor_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKOVERLAPBODY_API FProcessor_Sensor_Setup : public ck_exp::TProcessor<
            FProcessor_Sensor_Setup,
            FCk_Handle_Sensor,
            FFragment_Sensor_Current,
            FFragment_Sensor_Params,
            FTag_Sensor_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Sensor_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(
            FCk_Time InDeltaT) -> void;

        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InSensorEntity,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOVERLAPBODY_API FProcessor_Sensor_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Sensor_HandleRequests,
            FCk_Handle_Sensor,
            FFragment_Sensor_Current,
            FFragment_Sensor_Params,
            FFragment_Sensor_Requests,
            CK_IGNORE_PENDING_KILL>
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
            const FFragment_Sensor_Requests& InRequestsComp) const -> void;

    private:
        static auto DoHandleRequest(
            HandleType InSensorEntity,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_EnableDisable& InRequest) -> void;

        static auto DoHandleRequest(
            HandleType InSensorEntity,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_OnBeginOverlap& InRequest) -> void;

        static auto DoHandleRequest(
            HandleType InSensorEntity,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_OnEndOverlap& InRequest) -> void;

        static auto DoHandleRequest(
            HandleType InSensorEntity,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_OnBeginOverlap_NonMarker& InRequest) -> void;

        static auto DoHandleRequest(
            HandleType InSensorEntity,
            FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params& InParamsComp,
            const FCk_Request_Sensor_OnEndOverlap_NonMarker& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOVERLAPBODY_API FProcessor_Sensor_UpdateTransform : public ck_exp::TProcessor<
            FProcessor_Sensor_UpdateTransform,
            FCk_Handle_Sensor,
            FFragment_Sensor_Current,
            FFragment_Sensor_Params,
            FTag_Sensor_UpdateTransform,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        static auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InSensorEntity,
            const FFragment_Sensor_Current& InCurrentComp,
            const FFragment_Sensor_Params&  InParamsComp) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOVERLAPBODY_API FProcessor_Sensor_Teardown : public ck_exp::TProcessor<
            FProcessor_Sensor_Teardown,
            FCk_Handle_Sensor,
            FFragment_Sensor_Current,
            FFragment_Sensor_Params,
            CK_IF_INITIATE_KILL>
    {
    public:
        using MarkedDirtyBy = ck::FTag_DestroyEntity_Initiate;

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
            const FCk_Registry& InRegistry);

    public:
        auto Tick(FCk_Time) -> void;

    private:
        FCk_Registry _Registry;
    };
}

// --------------------------------------------------------------------------------------------------------------------
