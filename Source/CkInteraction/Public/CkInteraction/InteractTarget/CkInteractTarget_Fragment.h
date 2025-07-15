#pragma once

#include "CkInteractTarget_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Record/CkRecord_Utils.h"

#include "CkInteraction/Interaction/CkInteraction_Fragment.h"
#include "CkInteraction/Interaction/CkInteraction_Fragment_Data.h"

#include "CkInteractTarget_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_InteractTarget_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_InteractTarget_RequiresSetup);
    CK_DEFINE_ECS_TAG(FTag_InteractTarget_Updated);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfInteractTargets, FCk_Handle_InteractTarget);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKINTERACTION_API FFragment_InteractTarget_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_InteractTarget_Params);

    public:
        using ParamsType = FCk_Fragment_InteractTarget_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_InteractTarget_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKINTERACTION_API FFragment_InteractTarget_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_InteractTarget_Current);

    public:
        friend class FProcessor_InteractTarget_Setup;
        friend class FProcessor_InteractTarget_HandleRequests;
        friend class FProcessor_InteractTarget_Teardown;
        friend class UCk_Utils_InteractTarget_UE;

    private:
        TMap<FCk_Handle_Interaction, UUtils_Signal_Interaction_OnInteractionFinished::ConnectionType> _InteractionFinishedSignals;

        ECk_EnableDisable _Enabled = ECk_EnableDisable::Enable;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKINTERACTION_API FFragment_InteractTarget_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_InteractTarget_Requests);

    public:
        friend class FProcessor_InteractTarget_HandleRequests;
        friend class UCk_Utils_InteractTarget_UE;

    public:
        using RequestType = std::variant
        <
            FCk_Try_InteractTarget_StartInteraction,
            FCk_Request_InteractTarget_CancelInteraction
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
        InteractTarget_OnNewInteraction,
        FCk_Delegate_InteractTarget_OnNewInteraction_MC,
        FCk_Handle_InteractTarget,
        FCk_Handle_Interaction);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKINTERACTION_API,
        InteractTarget_OnInteractionFinished,
        FCk_Delegate_InteractTarget_OnInteractionFinished_MC,
        FCk_Handle_InteractTarget,
        FCk_Handle_Interaction,
        ECk_SucceededFailed);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_InteractTarget_Replicate; }

UCLASS(Blueprintable)
class CKINTERACTION_API UCk_Fragment_InteractTarget_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_InteractTarget_Rep);

public:
    friend class ck::FProcessor_InteractTarget_Replicate;
};

// --------------------------------------------------------------------------------------------------------------------