#pragma once

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Fragment.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

#include "GameplayTagContainer.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_SmState_UE;

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
    // Added by UCk_Utils_SmState_UE::Create, removed when the state entity is destroyed on exit.
    CK_DEFINE_ECS_TAG(FTag_SmState_Active);

    // Signals that this state's transitions should be walked this pump cycle.
    // Added by FProcessor_SmState_Update (every frame, Ticking states) and by
    // FProcessor_SmTransition_Evaluate (on Pass/Fail, for EventDriven states).
    CK_DEFINE_ECS_TAG(FTag_SmState_NeedsEvaluation);

    // ================================================================================================================
    // FRAGMENTS
    // ================================================================================================================

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
    // RECORDS
    // ================================================================================================================

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfSmStates, FCk_Handle_SmState);

    // ================================================================================================================
    // ENTITY-HOLDER BACK-REFERENCES
    // ================================================================================================================

    CK_DEFINE_ENTITY_HOLDER_AND_UTILS(TUtils_Sm_ParentState, FFragment_Sm_ParentState, FCk_Handle_SmState);
}

// --------------------------------------------------------------------------------------------------------------------
