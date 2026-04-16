#pragma once

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_SmCondition_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ================================================================================================================
    // TAGS
    // ================================================================================================================

    CK_DEFINE_ECS_TAG(FTag_SmCondition_Polled);
    CK_DEFINE_ECS_TAG(FTag_SmCondition_EventDriven);
    CK_DEFINE_ECS_TAG(FTag_SmCondition_EvaluationPaused);

    // ================================================================================================================
    // FRAGMENTS
    // ================================================================================================================

    struct CKSTATEMACHINE_API FFragment_SmCondition_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_SmCondition_Params);

    private:
        TSubclassOf<UCk_SmCondition_EntityScript> _ScriptClass;

    public:
        CK_PROPERTY_GET(_ScriptClass);

        CK_DEFINE_CONSTRUCTORS(FFragment_SmCondition_Params, _ScriptClass);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FFragment_SmCondition_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_SmCondition_Current);

        friend class FProcessor_SmCondition_ResetEveryFrame;
        friend class FProcessor_SmCondition_Polled;
        friend class FProcessor_SmTransition_Evaluate;

    private:
        ECk_SmConditionResult _Result = ECk_SmConditionResult::Undetermined;

    public:
        CK_PROPERTY(_Result);
    };

    // ================================================================================================================
    // RECORDS
    // ================================================================================================================

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfSmConditions, FCk_Handle_SmCondition);
}

// --------------------------------------------------------------------------------------------------------------------
