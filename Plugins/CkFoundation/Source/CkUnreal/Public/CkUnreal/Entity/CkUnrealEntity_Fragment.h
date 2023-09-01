#pragma once

#include "CkUnrealEntity_Fragment_Params.h"

#include <variant>

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    struct CKUNREAL_API FCk_Fragment_UnrealEntity_Requests
    {
        CK_GENERATED_BODY(FCk_Fragment_UnrealEntity_Requests);

    public:
        friend class UCk_Utils_UnrealEntity_UE;
        friend class FProcessor_UnrealEntity_HandleRequests;

    public:
        using FRequestType = FCk_Request_UnrealEntity_Spawn;
        using FRequestList = TArray<FRequestType>;

    private:
        FRequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // TODO: add signal for entity created when Signals are added
}

