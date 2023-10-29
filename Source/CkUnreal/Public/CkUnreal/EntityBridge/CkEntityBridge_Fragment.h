#pragma once

#include "CkEntityBridge_Fragment_Params.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKUNREAL_API FFragment_EntityBridge_Requests
    {
        CK_GENERATED_BODY(FFragment_EntityBridge_Requests);

    public:
        friend class UCk_Utils_EntityBridge_UE;
        friend class FProcessor_EntityBridge_HandleRequests;

    public:
        using RequestType = FCk_Request_EntityBridge_SpawnEntity;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // TODO: add signal for entity created when Signals are added
}

// --------------------------------------------------------------------------------------------------------------------

