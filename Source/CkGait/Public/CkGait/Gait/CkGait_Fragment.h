#pragma once

#include "CkGait/Gait/CkGait_Fragment_Data.h"
#include "CkGait/Gait/CkGait_Kernel.h"
#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Gait_UE;

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Gait_Disabled);

    // --------------------------------------------------------------------------------------------------------------------

    // Wholesale alias (CkSway precedent). Tunables, not Params: Request_UpdateSpec replaces it at runtime.
    using FFragment_Gait_Tunables = FCk_Gait_Spec;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKGAIT_API FFragment_Gait
    {
    public:
        CK_GENERATED_BODY(FFragment_Gait);

    public:
        friend class FProcessor_Gait_HandleRequests;
        friend class FProcessor_Gait_Update;
        friend class UCk_Utils_Gait_UE;

    private:
        gait::FClockState _Clock;
        FCk_Gait_Motion _LastMotion;
        int32 _LandingCount = 0;
        float _LastLandImpactSpeed = 0.0f;

    public:
        CK_PROPERTY_GET(_Clock);
        CK_PROPERTY_GET(_LastMotion);
        CK_PROPERTY_GET(_LandingCount);
        CK_PROPERTY_GET(_LastLandImpactSpeed);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Gait, _LandingCount);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKGAIT_API FFragment_Gait_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Gait_Requests);

        friend class FProcessor_Gait_HandleRequests;
        friend class FProcessor_Gait_CancelPendingRequests;
        friend class UCk_Utils_Gait_UE;

        using RequestType = std::variant<FCk_Request_Gait_UpdateSpec, FCk_Request_Gait_EnableDisable, FCk_Request_Gait_Reset>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Gait_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
