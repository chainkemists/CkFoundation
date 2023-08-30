#pragma once

#include "CkCore/Enums/CkEnums.h"

#include "CkMacros/CkMacros.h"

#include "CkOverlapBody/Marker/CkMarker_Fragment_Params.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Marker_UE;
class UCk_Utils_MarkerAndSensor_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FCk_Tag_Marker_Setup {};
    struct FCk_Tag_Marker_UpdateTransform {};

    // --------------------------------------------------------------------------------------------------------------------

    struct CKOVERLAPBODY_API FCk_Fragment_Marker_Params
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_Marker_Params);

    public:
        using ParamsType = FCk_Fragment_Marker_ParamsData;

    public:
        FCk_Fragment_Marker_Params() = default;
        explicit FCk_Fragment_Marker_Params(
            ParamsType InParams);

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKOVERLAPBODY_API FCk_Fragment_Marker_Current
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_Marker_Current);

    public:
        friend class FCk_Processor_Marker_Setup;
        friend class FCk_Processor_Marker_HandleRequests;
        friend class UCk_Utils_Marker_UE;
        friend class UCk_Utils_MarkerAndSensor_UE;

    public:
        FCk_Fragment_Marker_Current() = default;
        explicit FCk_Fragment_Marker_Current(
            ECk_EnableDisable InEnableDisable);

    private:
        TWeakObjectPtr<UShapeComponent> _Marker;
        FCk_EntityOwningActor_BasicDetails _AttachedEntityAndActor;
        ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Enable;

    public:
        CK_PROPERTY_GET(_EnableDisable);
        CK_PROPERTY_GET(_AttachedEntityAndActor);
        CK_PROPERTY_GET(_Marker);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKOVERLAPBODY_API FCk_Fragment_Marker_Requests
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_Marker_Requests);

    public:
        friend class FCk_Processor_Marker_HandleRequests;
        friend class UCk_Utils_Marker_UE;

    public:
        using RequestType = FCk_Request_Marker_EnableDisable;
        using RequestList = TOptional<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FCk_Fragment_RecordOfMarkers : public FCk_Fragment_Record {};
}


// --------------------------------------------------------------------------------------------------------------------