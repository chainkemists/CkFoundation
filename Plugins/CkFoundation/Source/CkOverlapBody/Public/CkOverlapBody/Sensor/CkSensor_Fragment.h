#pragma once

#include "CkMacros/CkMacros.h"

#include "CkOverlapBody/Sensor/CkSensor_Fragment_Params.h"

#include <variant>

#include "CkRecord/Record/CkRecord_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Sensor_UE;
class UCk_Utils_MarkerAndSensor_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FCk_Tag_Sensor_Setup {};
    struct FCk_Tag_Sensor_UpdateTransform {};

    // --------------------------------------------------------------------------------------------------------------------

    struct CKOVERLAPBODY_API FCk_Fragment_Sensor_Params
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_Sensor_Params);

    public:
        using ParamsType = FCk_Fragment_Sensor_ParamsData;

    public:
        FCk_Fragment_Sensor_Params() = default;
        explicit FCk_Fragment_Sensor_Params(
            ParamsType InParams);

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKOVERLAPBODY_API FCk_Fragment_Sensor_Current
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_Sensor_Current);

    public:
        friend class FCk_Processor_Sensor_HandleRequests;
        friend class FCk_Processor_Sensor_Setup;
        friend class UCk_Utils_Sensor_UE;
        friend class UCk_Utils_MarkerAndSensor_UE;

    public:
        FCk_Fragment_Sensor_Current() = default;
        explicit FCk_Fragment_Sensor_Current(
            ECk_EnableDisable InEnableDisable);

    private:
        TWeakObjectPtr<UShapeComponent> _Sensor;
        FCk_EntityOwningActor_BasicDetails _AttachedEntityAndActor;
        ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Enable;

        FCk_Sensor_MarkerOverlaps _CurrentMarkerOverlaps;
        FCk_Sensor_NonMarkerOverlaps _CurrentNonMarkerOverlaps;

    public:
        CK_PROPERTY_GET(_Sensor);
        CK_PROPERTY_GET(_AttachedEntityAndActor);
        CK_PROPERTY_GET(_EnableDisable);
        CK_PROPERTY_GET(_CurrentMarkerOverlaps);
        CK_PROPERTY_GET(_CurrentNonMarkerOverlaps);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKOVERLAPBODY_API FCk_Fragment_Sensor_Requests
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_Sensor_Requests);

    public:
        friend class FCk_Processor_Sensor_HandleRequests;
        friend class UCk_Utils_Sensor_UE;

    public:
        using EnableDisableRequestType = FCk_Request_Sensor_EnableDisable;
        using EnableDisableRequestList = TOptional<EnableDisableRequestType>;

        using BeginOverlapRequestType = FCk_Request_Sensor_OnBeginOverlap;
        using BeginOverlapNoMarkerRequestType = FCk_Request_Sensor_OnBeginOverlap_NonMarker;

        using EndOverlapRequestType = FCk_Request_Sensor_OnEndOverlap;
        using EndOverlapNoMarkerRequestType = FCk_Request_Sensor_OnEndOverlap_NonMarker;

        using BeginOrEndOverlapRequestType = std::variant<BeginOverlapRequestType, EndOverlapRequestType, BeginOverlapNoMarkerRequestType, EndOverlapNoMarkerRequestType>;
        using BeginOrEndOverlapRequestList = TArray<BeginOrEndOverlapRequestType>;

    private:
        EnableDisableRequestList _EnableDisableRequest;
        BeginOrEndOverlapRequestList _BeginOrEndOverlapRequests;

    public:
        CK_PROPERTY_GET(_EnableDisableRequest);
        CK_PROPERTY_GET(_BeginOrEndOverlapRequests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FCk_Fragment_RecordOfSensors : public FCk_Fragment_Record {};
}


// --------------------------------------------------------------------------------------------------------------------