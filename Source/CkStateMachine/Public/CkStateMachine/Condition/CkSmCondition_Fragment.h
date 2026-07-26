#pragma once

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_SmCondition_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_SmCondition_Polled);
    CK_DEFINE_ECS_TAG(FTag_SmCondition_EventDriven);

    // Driven by the parent transition (Request_StartOrResumeEvaluating / Request_PauseEvaluation);
    // absence of the tag means evaluation is paused.
    CK_DEFINE_ECS_TAG(FTag_SmCondition_Evaluating);

    // Dedup guard for ExitCondition's two callers (FProcessor_SmCondition_Exit, and the script's
    // EndPlay fallback for cascade-destroyed conditions). Without it, event-driven conditions in
    // nested sub-SMs can leave delegates bound and fire MarkSatisfied on a destroyed entity.
    CK_DEFINE_ECS_TAG(FTag_SmCondition_Active);

    // Set by FProcessor_SmTransition_Exit when cascading exit; consumed by FProcessor_SmCondition_Exit
    // (EndPlay group, RunAfter SmTransition_Exit), which calls ExitCondition.
    CK_DEFINE_ECS_TAG(FTag_SmCondition_PendingExit);

    // --------------------------------------------------------------------------------------------------------------------

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

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfSmConditions, FCk_Handle_SmCondition);
}

// --------------------------------------------------------------------------------------------------------------------
