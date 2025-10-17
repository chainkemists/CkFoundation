#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkTransition_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Lifecycle tags
    CK_DEFINE_ECS_TAG(FTag_Transition_Setup);
    CK_DEFINE_ECS_TAG(FTag_Transition_Enter);
    CK_DEFINE_ECS_TAG(FTag_Transition_Exit);
    CK_DEFINE_ECS_TAG(FTag_Transition_EvaluationPassed);
    CK_DEFINE_ECS_TAG(FTag_Transition_EvaluationFailed);

    // Behavior tags
    CK_DEFINE_ECS_TAG(FTag_Transition_IsEventDriven);
    CK_DEFINE_ECS_TAG(FTag_Transition_IsNotEventDriven);

    // State machine evaluation tags
    CK_DEFINE_ECS_TAG(FTag_StateMachine_Evaluate_TransitionOrCondition);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_Transition_Params = FCk_Fragment_Transition_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKHFSM_API FFragment_Transition_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Transition_Current);

    public:
        friend class FProcessor_Transition_Setup;
        friend class FProcessor_Transition_Enter;
        friend class FProcessor_Transition_Exit;
        friend class FProcessor_Transition_Evaluate;

    private:
        int32 _ReservedForFutureUse = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Signals
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKHFSM_API, OnTransitionEnter, FCk_Delegate_Transition_MC,
        FCk_Handle_Transition, FCk_Time);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKHFSM_API, OnTransitionExit, FCk_Delegate_Transition_MC,
        FCk_Handle_Transition, FCk_Time);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKHFSM_API, OnTransitionPassed, FCk_Delegate_Transition_MC,
        FCk_Handle_Transition, FCk_Time);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKHFSM_API, OnTransitionFailed, FCk_Delegate_Transition_MC,
        FCk_Handle_Transition, FCk_Time);
}

// --------------------------------------------------------------------------------------------------------------------