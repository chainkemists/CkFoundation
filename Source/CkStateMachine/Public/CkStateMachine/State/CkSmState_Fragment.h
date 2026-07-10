#pragma once

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Fragment.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

#include "GameplayTagContainer.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_SmState_UE;
class UCk_SmState_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ================================================================================================================
    // TAGS
    // ================================================================================================================

    // State evaluation model — every state starts as fully event-driven.
    // Automatically removed when any attached transition has a Polled condition (cascades up via
    // Request_MarkTransition_AsNotFullyEventDriven → Request_MarkState_AsNotFullyEventDriven).
    CK_DEFINE_ECS_TAG(FTag_SmState_FullyEventDriven);

    // Marks the currently active state of a running SM.
    // Added by UCk_SmState_EntityScript::EnterState (BeginPlay), removed by ExitState — the
    // Enter/Exit dedup and the graph-walk isolation chain both hinge on exactly this timing.
    CK_DEFINE_ECS_TAG(FTag_SmState_Active);

    // Signals that this state's transitions should be walked this pump cycle.
    // Added by FProcessor_SmState_Update (every frame, Ticking states) and by
    // FProcessor_SmTransition_Evaluate (on Pass/Fail, for EventDriven states).
    CK_DEFINE_ECS_TAG(FTag_SmState_NeedsEvaluation);

    // Set by UCk_Utils_SmState_UE::Request_Exit. Picked up by FProcessor_SmState_Exit (EndPlay
    // group) which cascades exit tags to tasks/transitions then calls ExitState on the script.
    CK_DEFINE_ECS_TAG(FTag_SmState_PendingExit);

    // ================================================================================================================
    // FRAGMENTS
    // ================================================================================================================

    // Stores both the originally-requested entity script class and the resolved class (the class
    // actually instantiated, post-override resolution via FFragment_Sm_StateOverrides). When no
    // override applies, both are equal. Comparing them tells you whether this state was overridden,
    // and to what. Populated by UCk_Utils_SmState_UE::Create.
    struct CKSTATEMACHINE_API FFragment_SmState_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_SmState_Params);

    private:
        TSubclassOf<UCk_SmState_EntityScript> _RequestedScriptClass;
        TSubclassOf<UCk_SmState_EntityScript> _ResolvedScriptClass;

    public:
        CK_PROPERTY_GET(_RequestedScriptClass);
        CK_PROPERTY_GET(_ResolvedScriptClass);

        CK_DEFINE_CONSTRUCTORS(FFragment_SmState_Params, _RequestedScriptClass, _ResolvedScriptClass);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Hierarchical path of this state from the root StateMachine down to this state, ordered root -> leaf.
    // Leaf element equals UCk_SmState_EntityScript::Get_StateTagForClass for this state's class.
    // Populated by UCk_Utils_SmState_UE::Create and consumed by override resolution.
    struct CKSTATEMACHINE_API FFragment_SmState_Hierarchy
    {
    public:
        CK_GENERATED_BODY(FFragment_SmState_Hierarchy);

        friend class ::UCk_Utils_SmState_UE;

    private:
        TArray<FGameplayTag> _Hierarchy;

    public:
        CK_PROPERTY_GET(_Hierarchy);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_SmState_Hierarchy, _Hierarchy);
    };

    // ================================================================================================================
    // FINGERPRINT
    // ================================================================================================================

    class FProcessor_Sm_CommitPendingTransition;

    // Structural fingerprint of the state's DefineState output (declaration-order for tasks,
    // transitions, and conditions; see spec §9). Computed locally on every machine after
    // DefineState completes. Compared to the replicated fingerprint at construction time on
    // non-authority machines (transition-time path). NOTE: the spec §9.5 initial-state
    // (Setup-time) check was never implemented — only transition-time verification exists;
    // the rep payload's _InitialStateFingerprint is currently informational.
    struct CKSTATEMACHINE_API FFragment_SmState_Fingerprint
    {
    public:
        CK_GENERATED_BODY(FFragment_SmState_Fingerprint);

        friend class FProcessor_Sm_CommitPendingTransition;
        friend class ::UCk_Utils_SmState_UE;
        friend class ::UCk_SmState_EntityScript;

    private:
        int32 _Hash = 0;

    public:
        CK_PROPERTY(_Hash);
    };

    // Transient scratch fragment used only during the state's DefineState. UCk_SmState_EntityScript::
    // ComposeFromState appends each composed-from class here in call order; after DefineState
    // returns, the entity-script's Construct path reads the list to build the fingerprint inputs,
    // then removes this fragment. Not part of the persistent state representation.
    struct CKSTATEMACHINE_API FFragment_SmState_ComposedFromInProgress
    {
    public:
        CK_GENERATED_BODY(FFragment_SmState_ComposedFromInProgress);

        friend class ::UCk_SmState_EntityScript;

    private:
        TArray<TSubclassOf<UCk_SmState_EntityScript>> _ComposedFromClasses;

    public:
        CK_PROPERTY_GET(_ComposedFromClasses);
    };

    // Transient fingerprint carrier — placed on a state entity at commit time by
    // FProcessor_Sm_CommitPendingTransition when the commit was driven by a replicated event
    // with a non-zero fingerprint. UCk_SmState_EntityScript::Construct reads the value after
    // DoComputeFingerprint runs, compares to the locally-computed hash, and either removes the
    // fragment (match) or escalates to FTag_Sm_DeterminismFault on the owning SM (mismatch).
    //
    // Authority-driven commits leave PendingTransition._NewStateFingerprint at 0, so this
    // fragment is only ever attached on non-authority machines that received a replicated event.
    struct CKSTATEMACHINE_API FFragment_SmState_ExpectedFingerprint
    {
    public:
        CK_GENERATED_BODY(FFragment_SmState_ExpectedFingerprint);

        friend class FProcessor_Sm_CommitPendingTransition;
        friend class ::UCk_SmState_EntityScript;

    private:
        int32 _Hash = 0;

    public:
        CK_PROPERTY(_Hash);
    };

    // ================================================================================================================
    // RECORDS
    // ================================================================================================================

    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfSmStates, FCk_Handle_SmState);

    // ================================================================================================================
    // ENTITY-HOLDER BACK-REFERENCES
    // ================================================================================================================

    CK_DEFINE_ENTITY_HOLDER_AND_UTILS_TRANSIENT(TUtils_Sm_ParentState, FFragment_Sm_ParentState, FCk_Handle_SmState);
}

// --------------------------------------------------------------------------------------------------------------------
