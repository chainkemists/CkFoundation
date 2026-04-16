#pragma once

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_SmTask_EntityScript;
class UCk_SmTask_SubStateMachine;
class UCk_Utils_SmTask_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ================================================================================================================
    // TAGS
    // ================================================================================================================

    CK_DEFINE_ECS_TAG(FTag_SmTask_Tick);
    CK_DEFINE_ECS_TAG(FTag_SmTask_EnterExit);
    CK_DEFINE_ECS_TAG(FTag_SmTask_ResultDirty);

    // ================================================================================================================
    // FRAGMENTS
    // ================================================================================================================

    struct CKSTATEMACHINE_API FFragment_SmTask_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_SmTask_Params);

    private:
        TSubclassOf<UCk_SmTask_EntityScript> _ScriptClass;

    public:
        CK_PROPERTY_GET(_ScriptClass);

        CK_DEFINE_CONSTRUCTORS(FFragment_SmTask_Params, _ScriptClass);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FFragment_SmTask_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_SmTask_Current);

        friend class FProcessor_SmTask_Tick;
        friend class FProcessor_SmTask_FireFinishedSignal;
        friend class ::UCk_SmTask_EntityScript;
        friend class ::UCk_Utils_SmTask_UE;

    private:
        ECk_SmTaskResult _LastResult = ECk_SmTaskResult::Running;

    public:
        CK_PROPERTY_GET(_LastResult);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FFragment_SmTask_SubStateMachine
    {
    public:
        CK_GENERATED_BODY(FFragment_SmTask_SubStateMachine);

        friend class UCk_SmTask_SubStateMachine;

    private:
        FCk_Handle_StateMachine _SubStateMachineHandle;

    public:
        CK_PROPERTY_GET(_SubStateMachineHandle);
    };

    // ================================================================================================================
    // SIGNALS
    // ================================================================================================================

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSTATEMACHINE_API,
        OnSubSmConstructed,
        FCk_Delegate_SmTask_OnSubSmConstructed,
        FCk_Handle_SmTask,
        FCk_Sm_Payload_OnSubSmConstructed);

    // ================================================================================================================
    // RECORDS
    // ================================================================================================================

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfSmTasks, FCk_Handle_SmTask);
}

// --------------------------------------------------------------------------------------------------------------------
