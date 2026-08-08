#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkInput/CkInputSource_Fragment_Data.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_InputSource_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_InputSource_Params = FCk_Fragment_InputSource_ParamsData;

    // ----------------------------------------------------------------------------------------------------------------

    // The inbox. Producers append rows through the request queue; a later router drains them. Events are plain
    // rows in one array rather than entities of their own — they arrive at device frequency, live for a frame,
    // and carry no identity anything outside this fragment could hold on to.
    struct CKINPUT_API FFragment_InputSource_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_InputSource_Current);

    public:
        friend class FProcessor_InputSource_HandleRequests;
        friend class FProcessor_InputLayer_Route;

    private:
        TArray<FCk_InputSource_RawEvent> _PendingRawEvents;

        TArray<FCk_InputSource_DeviceId> _OwnedDevices;

    public:
        CK_PROPERTY_GET(_PendingRawEvents);
        CK_PROPERTY_GET(_OwnedDevices);
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKINPUT_API FFragment_InputSource_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_InputSource_Requests);

    public:
        friend class FProcessor_InputSource_HandleRequests;
        friend class ::UCk_Utils_InputSource_UE;

    public:
        using RequestType = std::variant<FCk_Request_InputSource_InjectRawEvent, FCk_Request_InputSource_AssignDevice>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };
}

// --------------------------------------------------------------------------------------------------------------------
