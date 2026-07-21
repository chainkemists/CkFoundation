#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"

#include "CkPoi/CkPoi_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Poi_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Poi_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_Poi_Params = FCk_Fragment_Poi_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPOI_API FFragment_Poi_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Poi_Current);

    public:
        friend class FProcessor_Poi_Setup;
        friend class FProcessor_Poi_HandleRequests;
        friend class ::UCk_Utils_Poi_UE;

    private:
        ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Enable;
        FGameplayTagContainer _StateTags;

    public:
        CK_PROPERTY_GET(_EnableDisable);
        CK_PROPERTY_GET(_StateTags);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPOI_API FFragment_Poi_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Poi_Requests);

    public:
        friend class FProcessor_Poi_HandleRequests;
        friend class ::UCk_Utils_Poi_UE;

    public:
        using RequestType = std::variant<FCk_Request_Poi_EnableDisable, FCk_Request_Poi_AddStateTag,
            FCk_Request_Poi_RemoveStateTag, FCk_Request_Poi_SetStateTags>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKPOI_API, OnPoiStateChanged, FCk_Delegate_Poi_StateChanged, FCk_Handle_Poi, FGameplayTagContainer);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKPOI_API, OnPoiEnableDisable, FCk_Delegate_Poi_EnableDisable, FCk_Handle_Poi, ECk_EnableDisable);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Poi_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
