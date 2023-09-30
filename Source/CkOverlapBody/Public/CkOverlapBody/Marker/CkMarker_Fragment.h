#pragma once

#include "CkCore/Enums/CkEnums.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkOverlapBody/Marker/CkMarker_Fragment_Params.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkSignal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Marker_UE;
class UCk_Utils_MarkerAndSensor_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FTag_Marker_Setup {};
    struct FTag_Marker_UpdateTransform {};

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
        using RequestType = FCk_Request_Marker_EnableDisable;
        using RequestList = TOptional<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_RecordOfMarkers : public FFragment_RecordOfEntities
    {
        using FFragment_RecordOfEntities::FFragment_RecordOfEntities;
    };;

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