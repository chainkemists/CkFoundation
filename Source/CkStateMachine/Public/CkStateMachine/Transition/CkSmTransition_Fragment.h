#pragma once

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Fragment.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_SmState_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ================================================================================================================
    // TAGS
    // ================================================================================================================

    CK_DEFINE_ECS_TAG(FTag_Sm_TransitionQueued);
    CK_DEFINE_ECS_TAG(FTag_SmTransition_Evaluating);

    // Set on every new transition. Removed when any attached condition is Polled,
    // which also cascades to remove FTag_SmState_FullyEventDriven on the parent state.
    CK_DEFINE_ECS_TAG(FTag_SmTransition_FullyEventDriven);

    // ================================================================================================================
    // FRAGMENTS
    // ================================================================================================================

    struct CKSTATEMACHINE_API FFragment_SmTransition_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_SmTransition_Params);

    private:
        TSubclassOf<UCk_SmState_EntityScript> _TargetStateClass;

    public:
        CK_PROPERTY_GET(_TargetStateClass);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_SmTransition_Params, _TargetStateClass);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FFragment_SmTransition_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_SmTransition_Current);

        friend class FProcessor_SmTransition_Evaluate;

    private:
        ECk_SmTransitionResult _Result = ECk_SmTransitionResult::Undetermined;

    public:
        CK_PROPERTY(_Result);
    };

    // ================================================================================================================
    // RECORDS
    // ================================================================================================================

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfSmTransitions, FCk_Handle_SmTransition);

    // ================================================================================================================
    // ENTITY-HOLDER BACK-REFERENCES
    // ================================================================================================================

    CK_DEFINE_ENTITY_HOLDER_AND_UTILS(TUtils_Sm_ParentTransition, FFragment_Sm_ParentTransition, FCk_Handle_SmTransition);
}

// --------------------------------------------------------------------------------------------------------------------
