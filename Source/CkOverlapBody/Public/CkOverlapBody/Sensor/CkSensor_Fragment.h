#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkOverlapBody/Sensor/CkSensor_Fragment_Data.h"

#include <variant>

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkSignal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Sensor_UE;
class UCk_Utils_MarkerAndSensor_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Sensor_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_Sensor_SetupComplete);
    CK_DEFINE_ECS_TAG(FTag_Sensor_UpdateTransform);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKOVERLAPBODY_API FFragment_Sensor_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Sensor_Params);

    public:
        using ParamsType = FCk_Fragment_Sensor_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Sensor_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKOVERLAPBODY_API FFragment_Sensor_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Sensor_Current);

    public:
        friend class FProcessor_Sensor_HandleRequests;
        friend class FProcessor_Sensor_Setup;
        friend class FProcessor_Sensor_Teardown;
        friend class UCk_Utils_Sensor_UE;
        friend class UCk_Utils_MarkerAndSensor_UE;

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

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Sensor_Current, _EnableDisable);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKOVERLAPBODY_API FFragment_Sensor_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Sensor_Requests);

    public:
        friend class FProcessor_Sensor_HandleRequests;
        friend class UCk_Utils_Sensor_UE;

    public:
        using EnableDisableRequestType = FCk_Request_Sensor_EnableDisable;
        using EnableDisableRequestList = TOptional<EnableDisableRequestType>;

        using ResizeRequestType = FCk_Request_Sensor_Resize;
        using ResizeRequestList = TOptional<ResizeRequestType>;

        using BeginOverlapRequestType = FCk_Request_Sensor_OnBeginOverlap;
        using BeginOverlapNoMarkerRequestType = FCk_Request_Sensor_OnBeginOverlap_NonMarker;

        using EndOverlapRequestType = FCk_Request_Sensor_OnEndOverlap;
        using EndOverlapNoMarkerRequestType = FCk_Request_Sensor_OnEndOverlap_NonMarker;

        using BeginOrEndOverlapRequestType = std::variant<BeginOverlapRequestType, EndOverlapRequestType, BeginOverlapNoMarkerRequestType, EndOverlapNoMarkerRequestType>;
        using BeginOrEndOverlapRequestList = TArray<BeginOrEndOverlapRequestType>;

    private:
        EnableDisableRequestList _EnableDisableRequest;
        ResizeRequestList _ResizeRequest;
        BeginOrEndOverlapRequestList _BeginOrEndOverlapRequests;

    public:
        CK_PROPERTY_GET(_EnableDisableRequest);
        CK_PROPERTY_GET(_ResizeRequest);
        CK_PROPERTY_GET(_BeginOrEndOverlapRequests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfSensors, FCk_Handle_Sensor);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKOVERLAPBODY_API,
        OnSensorEnableDisable,
        FCk_Delegate_Sensor_OnEnableDisable_MC,
        FCk_Handle,
        FGameplayTag,
        ECk_EnableDisable);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKOVERLAPBODY_API,
        OnSensorBeginOverlap,
        FCk_Delegate_Sensor_OnBeginOverlap_MC,
        FCk_Handle,
        FCk_Sensor_Payload_OnBeginOverlap);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKOVERLAPBODY_API,
        OnSensorEndOverlap,
        FCk_Delegate_Sensor_OnEndOverlap_MC,
        FCk_Handle,
        FCk_Sensor_Payload_OnEndOverlap);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKOVERLAPBODY_API,
        OnSensorBeginOverlap_NonMarker,
        FCk_Delegate_Sensor_OnBeginOverlap_NonMarker_MC,
        FCk_Handle,
        FCk_Sensor_Payload_OnBeginOverlap_NonMarker);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKOVERLAPBODY_API,
        OnSensorEndOverlap_NonMarker,
        FCk_Delegate_Sensor_OnEndOverlap_NonMarker_MC,
        FCk_Handle,
        FCk_Sensor_Payload_OnEndOverlap_NonMarker);
}


// --------------------------------------------------------------------------------------------------------------------