#pragma once

#include "CkNavigation/Nav/CkNav_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Nav_UE;

namespace ck
{
    struct CKNAVIGATION_API FFragment_Nav_Requests
    {
        CK_GENERATED_BODY(FFragment_Nav_Requests);

        friend class FProcessor_Nav_HandleRequests;
        friend class ::UCk_Utils_Nav_UE;

        using FindPathRequestType = FCk_Request_Nav_FindPath;
        using RequestType = std::variant<FindPathRequestType>;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_Nav_PathResult = FCk_Nav_PathResult;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKNAVIGATION_API,
        Nav_OnPathReady,
        FCk_Delegate_Nav_OnPathReady,
        FCk_Handle,
        FCk_Nav_PathResult);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKNAVIGATION_API,
        Nav_OnPathFailed,
        FCk_Delegate_Nav_OnPathFailed,
        FCk_Handle);
}

// --------------------------------------------------------------------------------------------------------------------
