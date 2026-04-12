#pragma once

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ================================================================================================================
    // TAGS
    // ================================================================================================================

    CK_DEFINE_ECS_TAG(FTag_SmCondition_Polled);
    CK_DEFINE_ECS_TAG(FTag_SmCondition_EventDriven);
    CK_DEFINE_ECS_TAG(FTag_SmCondition_ResetsEveryFrame);
    CK_DEFINE_ECS_TAG(FTag_SmCondition_EvaluationPaused);

    // ================================================================================================================
    // FRAGMENTS
    // ================================================================================================================

    struct CKSTATEMACHINE_API FFragment_SmCondition_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_SmCondition_Current);

        friend class FProcessor_SmCondition_ResetEveryFrame;
        friend class FProcessor_SmCondition_Polled;
        friend class FProcessor_SmTransition_Evaluate;

    private:
        ECk_SmConditionResult _Result = ECk_SmConditionResult::Undetermined;
        ECk_SmConditionResetBehavior _ResetBehavior = ECk_SmConditionResetBehavior::ResetEveryFrame;

    public:
        CK_PROPERTY(_Result);
        CK_PROPERTY(_ResetBehavior);
    };

    // ================================================================================================================
    // RECORDS
    // ================================================================================================================

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfSmConditions, FCk_Handle_SmCondition);
}

// --------------------------------------------------------------------------------------------------------------------
