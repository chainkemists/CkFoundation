#pragma once

#include "CkItemQuery_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ItemQuery_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKINVENTORY_API FFragment_ItemQuery_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_ItemQuery_Requests);

    public:
        friend class FProcessor_ItemQuery_HandleRequests;
        friend class UCk_Utils_ItemQuery_UE;

    public:
        using RequestType = FCk_Request_ItemQuery_QueryDefinitions;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKINVENTORY_API,
        OnItemDefinitionsQueried,
        FCk_Delegate_ItemQuery_OnQueried,
        FCk_ItemQuery_Result);
}

// --------------------------------------------------------------------------------------------------------------------
