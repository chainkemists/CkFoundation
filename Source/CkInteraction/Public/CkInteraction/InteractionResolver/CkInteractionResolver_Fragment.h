#pragma once

#include "CkInteractionResolver_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkInteractionResolver_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_InteractionResolver_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_InteractionResolver_Updated);
    CK_DEFINE_ECS_TAG(FTag_InteractionResolver_IntentUpdated);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_InteractionResolver_Params = FCk_InteractionResolver_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKINTERACTION_API FFragment_InteractionResolver_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_InteractionResolver_Current);

    public:
        friend class FProcessor_InteractionResolver_Persistent;
        friend class FProcessor_InteractionResolver_HandleRequests;
        friend class FProcessor_InteractionResolver_EndPlay;
        friend class UCk_Utils_InteractionResolver_UE;

    private:
        // Cache of resolved targets per intent
        TMap<FGameplayTag, TArray<FCk_Handle_InteractTarget>> _CachedBestTargets;

        // Current active intents
        TSet<FGameplayTag> _ActiveIntents;

        // Available targets provided by callers
        TSet<FCk_Handle_InteractTarget> _AvailableTargets;

    public:
        CK_PROPERTY_GET(_CachedBestTargets);
        CK_PROPERTY_GET(_ActiveIntents);
        CK_PROPERTY_GET(_AvailableTargets);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKINTERACTION_API FFragment_InteractionResolver_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_InteractionResolver_Requests);

    public:
        friend class FProcessor_InteractionResolver_Persistent;
        friend class FProcessor_InteractionResolver_HandleRequests;
        friend class UCk_Utils_InteractionResolver_UE;

    public:
        using RequestType = std::variant
        <
            FCk_Request_InteractionResolver_StartIntent,
            FCk_Request_InteractionResolver_StopIntent,
            FCk_Request_InteractionResolver_AddInteractTarget,
            FCk_Request_InteractionResolver_RemoveInteractTarget,
            FCk_Request_InteractionResolver_RemoveAllTargetsByChannel
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
        InteractionResolver_OnBestTargetsChanged,
        FCk_Delegate_InteractionResolver_OnBestTargetsChanged_MC,
        FCk_Handle_InteractionResolver,
        FGameplayTag,
        TArray<FCk_Handle_InteractTarget>,
        TArray<FCk_Handle_InteractTarget>,
        TArray<FCk_Handle_InteractTarget>);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_InteractionResolver_Persistent; }

UCLASS(Blueprintable)
class CKINTERACTION_API UCk_Fragment_InteractionResolver_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_InteractionResolver_Rep);

public:
    friend class ck::FProcessor_InteractionResolver_Persistent;
};

// --------------------------------------------------------------------------------------------------------------------