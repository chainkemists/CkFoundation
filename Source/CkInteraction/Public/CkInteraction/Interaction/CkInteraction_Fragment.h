#pragma once

#include "CkInteraction_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkRecord/Record/CkRecord_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Interaction_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfInteractions, FCk_Handle_Interaction);

    // Set once OnInteractionFinished has been broadcast, so the EndPlay processor's destroy-mid-flight
    // Failed broadcast never doubles up on an interaction that already finished normally.
    CK_DEFINE_ECS_TAG(FTag_Interaction_FinishedBroadcastSent);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKINTERACTION_API FFragment_Interaction_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Interaction_Params);

    public:
        using ParamsType = FCk_Interaction_Spec;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Interaction_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKINTERACTION_API FFragment_Interaction_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Interaction_Current);

    public:
        friend class FProcessor_Interaction_Setup;
        friend class FProcessor_Interaction_HandleRequests;
        friend class FProcessor_Interaction_EndPlay;
        friend class UCk_Utils_Interaction_UE;

    private:
        // Currently not used by any processors, but we need at least one variable so it isn't a considered a Tag
        int32 _DummyProperty = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKINTERACTION_API FFragment_Interaction_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Interaction_Requests);

    public:
        friend class FProcessor_Interaction_HandleRequests;
        friend class UCk_Utils_Interaction_UE;

    public:
        using RequestType = std::variant
        <
            FCk_Request_Interaction_EndInteraction
        >;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKINTERACTION_API,
        Interaction_OnInteractionFinished,
        FCk_Delegate_Interaction_OnInteractionFinished,
        FCk_Handle_Interaction,
        ECk_SucceededFailed);
}

// --------------------------------------------------------------------------------------------------------------------