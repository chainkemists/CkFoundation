#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include "CkInput/CkInputLayer_Fragment_Data.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_InputLayer_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::input_layer
{
    // The reserved bottom of every stack. Global actions live on a layer at this priority so they are arbitrated by
    // the same top-down walk as everything else — a layer above them consumes and they simply never match. An
    // ordinary Add may not claim it, or a caller could accidentally sit underneath the debug keys.
    inline constexpr auto GlobalActionPriority = TNumericLimits<int32>::Lowest();
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_InputLayer_Params = FCk_Fragment_InputLayer_ParamsData;

    // ----------------------------------------------------------------------------------------------------------------

    // ONE stable fragment per layer holding every capture as a row. Never a fragment or tag per capture: these pools
    // are tombstone-mode, so adding and removing fragment TYPES per frame is the expensive shape, and the whole
    // point of the declarative model is that the active set is one inspectable array.
    struct CKINPUT_API FFragment_InputLayer_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_InputLayer_Current);

    public:
        friend class FProcessor_InputLayer_HandleRequests;

    private:
        TArray<FCk_InputLayer_Capture> _Captures;

    public:
        CK_PROPERTY_GET(_Captures);
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKINPUT_API FFragment_InputLayer_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_InputLayer_Requests);

    public:
        friend class FProcessor_InputLayer_HandleRequests;
        friend class ::UCk_Utils_InputLayer_UE;

    public:
        using RequestType = std::variant<FCk_Request_InputLayer_AddCapture, FCk_Request_InputLayer_RemoveCapture>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Which layer consumed a key's press. The router holds this, not the layers: a layer that consumed a press must
    // receive the matching release even if its captures changed or it was popped in between, and a layer below it
    // must never receive that release as a key-up for a press it never saw.
    struct CKINPUT_API FInputLayer_PressOwnership
    {
    public:
        CK_GENERATED_BODY(FInputLayer_PressOwnership);

    private:
        FKey _Key;

        FCk_Handle_InputLayer _Owner;

        FCk_InputLayer_Capture _Capture;

    public:
        CK_PROPERTY_GET(_Key);
        CK_PROPERTY_GET(_Owner);
        CK_PROPERTY_GET(_Capture);

    public:
        CK_DEFINE_CONSTRUCTORS(FInputLayer_PressOwnership, _Key, _Owner, _Capture);
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKINPUT_API FInputLayer_RankedLayer
    {
    public:
        CK_GENERATED_BODY(FInputLayer_RankedLayer);

    private:
        FCk_Handle_InputLayer _Layer;

        int32 _Priority = 0;

    public:
        CK_PROPERTY_GET(_Layer);
        CK_PROPERTY_GET(_Priority);

    public:
        CK_DEFINE_CONSTRUCTORS(FInputLayer_RankedLayer, _Layer, _Priority);
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Router state, on the input SOURCE rather than on any layer, because it has to outlive the layer it points at.
    struct CKINPUT_API FFragment_InputLayer_RouterState
    {
    public:
        CK_GENERATED_BODY(FFragment_InputLayer_RouterState);

    public:
        friend class FProcessor_InputLayer_Route;

    private:
        TArray<FInputLayer_PressOwnership> _PressOwners;

        TArray<FInputLayer_RankedLayer> _RankedLayers;

    public:
        CK_PROPERTY_GET(_PressOwners);
    };

    // ----------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKINPUT_API, OnInputCaptureTriggered, FCk_Delegate_InputLayer_CaptureTriggered, FCk_Handle_InputLayer, FCk_InputSource_RawEvent, FCk_InputLayer_Capture);
}

// --------------------------------------------------------------------------------------------------------------------
