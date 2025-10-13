#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkStateMachine_Fragment_Data.h"
#include "CkHFSM/State/CkState_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Lifecycle tags
    CK_DEFINE_ECS_TAG(FTag_StateMachine_Setup);
    CK_DEFINE_ECS_TAG(FTag_StateMachine_Enter);
    CK_DEFINE_ECS_TAG(FTag_StateMachine_Exit);
    CK_DEFINE_ECS_TAG(FTag_StateMachine_Update);
    CK_DEFINE_ECS_TAG(FTag_StateMachine_Transition);

    // State machine evaluation tag
    CK_DEFINE_ECS_TAG(FTag_StateMachine_Evaluate_StateMachine);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_StateMachine_Params = FCk_Fragment_StateMachine_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKHFSM_API FFragment_StateMachine_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_StateMachine_Current);

    public:
        friend class FProcessor_StateMachine_Setup;
        friend class FProcessor_StateMachine_Enter;
        friend class FProcessor_StateMachine_Exit;
        friend class FProcessor_StateMachine_Transition;
        friend class FProcessor_StateMachine_Evaluate;
        friend class UCk_Utils_StateMachine_UE;

    private:
        FCk_Handle _CurrentState;
        FCk_Handle _PreviousState;

    public:
        CK_PROPERTY_GET(_CurrentState);
        CK_PROPERTY_GET(_PreviousState);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_StateMachine_Current);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Record for child state entities
    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfStates, FCk_Handle_State);

    // --------------------------------------------------------------------------------------------------------------------

    // Signals
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKHFSM_API, OnStateMachineStart, FCk_Delegate_StateMachine_MC,
        FCk_Handle_StateMachine, FCk_Time);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKHFSM_API, OnStateMachineStop, FCk_Delegate_StateMachine_MC,
        FCk_Handle_StateMachine, FCk_Time);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE_CUSTOM(CKHFSM_API, OnStateMachineTransition, FCk_Delegate_StateMachine_Transition_MC,
        FCk_Handle_StateMachine, FCk_Handle_State, FCk_Handle_State);
}

// --------------------------------------------------------------------------------------------------------------------