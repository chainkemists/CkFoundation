#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkState_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Lifecycle tags
    CK_DEFINE_ECS_TAG(FTag_State_Setup);
    CK_DEFINE_ECS_TAG(FTag_State_Enter);
    CK_DEFINE_ECS_TAG(FTag_State_Exit);
    CK_DEFINE_ECS_TAG(FTag_State_Update);
    CK_DEFINE_ECS_TAG(FTag_State_ReadyToTransition);

    // Behavior tags
    CK_DEFINE_ECS_TAG(FTag_State_IsEventDriven);
    CK_DEFINE_ECS_TAG(FTag_State_IsNotEventDriven);

    // State machine evaluation tags
    CK_DEFINE_ECS_TAG(FTag_StateMachine_Evaluate_State);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_State_Params = FCk_Fragment_State_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKHFSM_API FFragment_State_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_State_Current);

    public:
        friend class FProcessor_State_Setup;
        friend class FProcessor_State_Enter;
        friend class FProcessor_State_Exit;
        friend class FProcessor_State_Evaluate;
        friend class FProcessor_State_Update;

    private:
        FCk_Handle _NextState;

    public:
        CK_PROPERTY(_NextState);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_State_Current);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Records for child entities
    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfServices, FCk_Handle_Service);
    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfTransitions, FCk_Handle_Transition);

    // --------------------------------------------------------------------------------------------------------------------

    // Signals
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKHFSM_API, OnStateEnter, FCk_Delegate_State_MC,
        FCk_Handle_State, FCk_Time);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKHFSM_API, OnStateExit, FCk_Delegate_State_MC,
        FCk_Handle_State, FCk_Time);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKHFSM_API, OnStateUpdate, FCk_Delegate_State_MC,
        FCk_Handle_State, FCk_Time);
}

// --------------------------------------------------------------------------------------------------------------------