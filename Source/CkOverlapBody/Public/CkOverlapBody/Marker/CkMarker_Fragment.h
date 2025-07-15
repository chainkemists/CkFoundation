#pragma once

#include "CkCore/Enums/CkEnums.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkOverlapBody/Marker/CkMarker_Fragment_Data.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Marker_UE;
class UCk_Utils_MarkerAndSensor_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Marker_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_Marker_SetupComplete);
    CK_DEFINE_ECS_TAG(FTag_Marker_UpdateTransform);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKOVERLAPBODY_API FFragment_Marker_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Marker_Params);

    public:
        using ParamsType = FCk_Fragment_Marker_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Marker_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKOVERLAPBODY_API FFragment_Marker_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Marker_Current);

    public:
        friend class FProcessor_Marker_Setup;
        friend class FProcessor_Marker_HandleRequests;
        friend class FProcessor_Marker_Teardown;
        friend class UCk_Utils_Marker_UE;
        friend class UCk_Utils_MarkerAndSensor_UE;

    private:
        TWeakObjectPtr<UShapeComponent> _Marker;
        FCk_EntityOwningActor_BasicDetails _AttachedEntityAndActor;
        ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Enable;

    public:
        CK_PROPERTY_GET(_EnableDisable);
        CK_PROPERTY_GET(_AttachedEntityAndActor);
        CK_PROPERTY_GET(_Marker);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Marker_Current, _EnableDisable);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKOVERLAPBODY_API FFragment_Marker_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Marker_Requests);

    public:
        friend class FProcessor_Marker_HandleRequests;
        friend class UCk_Utils_Marker_UE;

    public:
        using EnableDisableRequestType = FCk_Request_Marker_EnableDisable;
        using EnableDisableRequestList = TOptional<EnableDisableRequestType>;

        using ResizeRequestType = FCk_Request_Marker_Resize;
        using ResizeRequestList = TOptional<ResizeRequestType>;

    private:
        EnableDisableRequestList _EnableDisableRequest;
        ResizeRequestList _ResizeRequest;

    public:
        CK_PROPERTY_GET(_EnableDisableRequest);
        CK_PROPERTY_GET(_ResizeRequest);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfMarkers, FCk_Handle_Marker);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKOVERLAPBODY_API,
        OnMarkerEnableDisable,
        FCk_Delegate_Marker_OnEnableDisable_MC,
        FCk_Handle,
        FGameplayTag,
        ECk_EnableDisable);
}

// --------------------------------------------------------------------------------------------------------------------