#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

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

    // Marks a TTL ping created via UCk_Utils_Poi_UE::Create with a lifetime. Transient BY DESIGN: the persistence
    // handler's Produce returns UNSET for entities carrying this tag (and bare Create()'d hosts have no rebuild
    // recipe under v3 anyway — see CkPoi_Fragment.cpp).
    CK_DEFINE_ECS_TAG(FTag_Poi_TransientTtl);

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

    // ROUNDTRIP: Poi child entities that were composed during their owner's Construct are rebuilt on a v3 load
    // (recipe replay) and hydrated by the CkPoi persistence handler — the owner's record must round-trip with them
    // (a TRANSIENT record would orphan restored POIs from their owner, same failure CkTimer documents).
    CK_DEFINE_RECORD_OF_ENTITIES_ROUNDTRIP(FFragment_RecordOfPois, FCk_Handle_Poi);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKPOI_API, OnPoiStateChanged, FCk_Delegate_Poi_StateChanged, FCk_Handle_Poi, FGameplayTagContainer);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKPOI_API, OnPoiEnableDisable, FCk_Delegate_Poi_EnableDisable, FCk_Handle_Poi, ECk_EnableDisable);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Poi_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
