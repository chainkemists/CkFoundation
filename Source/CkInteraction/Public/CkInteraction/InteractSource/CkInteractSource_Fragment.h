#pragma once

#include "CkInteractSource_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkInteraction/Interaction/CkInteraction_Fragment.h"
#include "CkInteraction/Interaction/CkInteraction_Fragment_Data.h"

#include "CkInteractSource_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_InteractSource_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_InteractSource_RequiresSetup);
    CK_DEFINE_ECS_TAG(FTag_InteractSource_Updated);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfInteractSources, FCk_Handle_InteractSource);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKINTERACTION_API FFragment_InteractSource_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_InteractSource_Params);

    public:
        using ParamsType = FCk_Fragment_InteractSource_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_InteractSource_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKINTERACTION_API FFragment_InteractSource_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_InteractSource_Current);

    public:
        friend class FProcessor_InteractSource_Setup;
        friend class FProcessor_InteractSource_HandleRequests;
        friend class FProcessor_InteractSource_Teardown;
        friend class UCk_Utils_InteractSource_UE;

    private:
        // This is also used as a set of interaction handles
        TMap<FCk_Handle_Interaction, UUtils_Signal_Interaction_OnInteractionFinished::ConnectionType> _InteractionFinishedSignals;

        // Needed so we know if there will be an interaction that prevents us from starting a new one until we process request to start interaction
        TArray<FCk_Handle_Interaction> _InteractionsPendingAdd;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKINTERACTION_API FFragment_InteractSource_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_InteractSource_Requests);

    public:
        friend class FProcessor_InteractSource_HandleRequests;
        friend class UCk_Utils_InteractSource_UE;

    public:
        using RequestType = std::variant
        <
            FCk_Request_InteractSource_StartInteraction,
            FCk_Request_InteractSource_CancelInteraction
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
        InteractSource_OnNewInteraction,
        FCk_Delegate_InteractSource_OnNewInteraction_MC,
        FCk_Handle_InteractSource,
        FCk_Handle_Interaction);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKINTERACTION_API,
        InteractSource_OnInteractionFinished,
        FCk_Delegate_InteractSource_OnInteractionFinished_MC,
        FCk_Handle_InteractSource,
        FCk_Handle_Interaction,
        ECk_SucceededFailed);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_InteractSource_Replicate; }

UCLASS(Blueprintable)
class CKINTERACTION_API UCk_Fragment_InteractSource_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_InteractSource_Rep);

public:
    friend class ck::FProcessor_InteractSource_Replicate;
};

// --------------------------------------------------------------------------------------------------------------------