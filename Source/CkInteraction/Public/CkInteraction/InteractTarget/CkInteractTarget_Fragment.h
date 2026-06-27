#pragma once

#include "CkInteractTarget_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkInteraction/Interaction/CkInteraction_Fragment.h"
#include "CkInteraction/Interaction/CkInteraction_Fragment_Data.h"

#include "CkEcs/Snapshot/CkSnapshot_Context.h" // ck::FSnapshotContext (referenced by the SerializeSnapshot decls)

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_InteractTarget_UE;
class FArchive;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_InteractTarget_RequiresSetup);
    CK_DEFINE_ECS_TAG(FTag_InteractTarget_Updated);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfInteractTargets, FCk_Handle_InteractTarget);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKINTERACTION_API FFragment_InteractTarget_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_InteractTarget_Params);
        using IsSnapshotable = void;

    public:
        using ParamsType = FCk_Fragment_InteractTarget_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_InteractTarget_Params, _Params);

        // Tier-C: the interaction config (channel/completion-policy/duration/concurrency) is authoritative state.
        // The SM condition UBb_SmCondition_InteractedWith reads Get_InteractionChannel on restore, so this fragment
        // MUST round-trip or As_InteractTarget() ensures. Body (in the .cpp) serializes the inner USTRUCT via
        // SerializeItem — the native delegate is not a UPROPERTY so it's skipped (re-bound at compose time).
        auto SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& InCtx) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKINTERACTION_API FFragment_InteractTarget_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_InteractTarget_Current);
        using IsSnapshotable = void;

    public:
        friend class FProcessor_InteractTarget_Setup;
        friend class FProcessor_InteractTarget_HandleRequests;
        friend class FProcessor_InteractTarget_EndPlay;
        friend class UCk_Utils_InteractTarget_UE;

    private:
        TMap<FCk_Handle_Interaction, UUtils_Signal_Interaction_OnInteractionFinished::ConnectionType> _InteractionFinishedSignals;

        ECk_EnableDisable _Enabled = ECk_EnableDisable::Enable;

    public:
        // Tier-C: persist ONLY _Enabled (whether a saved interactable is disabled). The signal-connection map is
        // pure runtime (entt connections aren't serializable; its keys point at transient interaction entities) and
        // is deliberately excluded — so a restored interactable is a valid InteractTarget but re-binds its signals.
        auto SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& InCtx) -> void;
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
        FCk_Delegate_InteractTarget_OnNewInteraction,
        FCk_Handle_InteractTarget,
        FCk_Handle_Interaction);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKINTERACTION_API,
        InteractTarget_OnInteractionFinished,
        FCk_Delegate_InteractTarget_OnInteractionFinished,
        FCk_Handle_InteractTarget,
        FCk_Handle_Interaction,
        ECk_SucceededFailed);
}

// --------------------------------------------------------------------------------------------------------------------