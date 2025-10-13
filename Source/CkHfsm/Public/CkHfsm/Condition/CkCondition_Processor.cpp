#include "CkHfsm/Condition/CkCondition_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkHfsm/CkHfsm_Log.h"
#include "CkHfsm/Transition/CkTransition_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Condition_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Condition_Params& InParams,
            FFragment_Condition_Current& InCurrent) const
        -> void
    {
        // Apply configuration from params
        if (InParams.Get_NegateResult())
        {
            InHandle.AddOrGet<FTag_Condition_NegateResult>();
        }

        if (InParams.Get_IsEventDriven())
        {
            InHandle.AddOrGet<FTag_Condition_IsEventDriven>();
        }
        else
        {
            InHandle.AddOrGet<FTag_Condition_IsNotEventDriven>();
        }

        InHandle.Remove<FTag_Condition_Setup>();

        hfsm::VeryVerbose(TEXT("[SETUP][CONDITION] Entity [{}]"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Condition_Enter::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Condition_Current& InCurrent) const
        -> void
    {
        InHandle.Remove<FTag_Condition_Enter>();

        {
#if STATS
            auto StatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif
            UUtils_Signal_OnConditionEnter::Broadcast(InHandle, MakePayload(InHandle, InDeltaT));
        }

        hfsm::VeryVerbose(TEXT("[ENTER][CONDITION] Entity [{}]"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Condition_Exit::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Condition_Current& InCurrent) const
        -> void
    {
        InHandle.Remove<FTag_Condition_Exit>();
        InHandle.Remove<FTag_Condition_EvaluationPaused>();
        InHandle.Remove<FTag_Condition_EvaluationPassed>();
        InHandle.Remove<FTag_Condition_EvaluationFailed>();

        {
#if STATS
            auto StatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif
            UUtils_Signal_OnConditionExit::Broadcast(InHandle, MakePayload(InHandle, InDeltaT));
        }

        hfsm::VeryVerbose(TEXT("[EXIT][CONDITION] Entity [{}]"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Condition_Evaluate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Condition_Current& InCurrent) const
        -> void
    {
        InHandle.Remove<FTag_StateMachine_Evaluate_TransitionOrCondition>();

        // Derived condition types will implement their own evaluation logic
        // Base condition just checks if tags are already set
        // (e.g., via Request_MarkResult)

        hfsm::VeryVerbose(TEXT("[EVALUATE][CONDITION] Entity [{}]"), InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------