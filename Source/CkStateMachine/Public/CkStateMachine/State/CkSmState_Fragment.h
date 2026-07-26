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
    // --------------------------------------------------------------------------------------------------------------------

    // Every state starts fully event-driven; removed once any attached transition gains a Polled
    // condition (cascades via Request_MarkTransition_AsNotFullyEventDriven).
    CK_DEFINE_ECS_TAG(FTag_SmState_FullyEventDriven);

    // The running SM's active state. Added by EnterState (BeginPlay), removed by ExitState — the
    // Enter/Exit dedup and the graph-walk isolation chain both hinge on exactly this timing.
    CK_DEFINE_ECS_TAG(FTag_SmState_Active);

    // This state's transitions should be walked this pump cycle. Added by FProcessor_SmState_Update
    // (Ticking states) and FProcessor_SmTransition_Evaluate (EventDriven states, on Pass/Fail).
    CK_DEFINE_ECS_TAG(FTag_SmState_NeedsEvaluation);

    // Set by UCk_Utils_SmState_UE::Request_Exit. Picked up by FProcessor_SmState_Exit (EndPlay
    // group) which cascades exit tags to tasks/transitions then calls ExitState on the script.
    CK_DEFINE_ECS_TAG(FTag_SmState_PendingExit);

    // --------------------------------------------------------------------------------------------------------------------

    // Requested vs resolved (post-FFragment_Sm_StateOverrides) script class — equal when no override
    // applies, so comparing them tells you whether this state was overridden and to what.
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

    // Root -> leaf path from the root StateMachine down to this state; the leaf equals
    // UCk_SmState_EntityScript::Get_StateTagForClass for this state's class. Consumed by override resolution.
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

    // --------------------------------------------------------------------------------------------------------------------

    class FProcessor_Sm_CommitPendingTransition;

    // Structural fingerprint of the state's DefineState output, computed locally on every machine and
    // compared to the replicated value at construction time on non-authority machines. Only the
    // transition-time check exists — the payload's _InitialStateFingerprint is informational.
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

    // Transient DefineState scratch: ComposeFromState appends each composed-from class in call order,
    // then Construct reads the list for the fingerprint inputs and removes the fragment.
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

    // Transient carrier attached at commit time when the commit was driven by a replicated event with
    // a non-zero fingerprint; Construct compares it to the local hash and faults the SM on mismatch.
    // Authority commits leave the fingerprint at 0, so this is only ever seen on non-authority machines.
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

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfSmStates, FCk_Handle_SmState);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ENTITY_HOLDER_AND_UTILS_TRANSIENT(TUtils_Sm_ParentState, FFragment_Sm_ParentState, FCk_Handle_SmState);
}

// --------------------------------------------------------------------------------------------------------------------
