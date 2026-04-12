#pragma once

#include "CkStateMachine_Request_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcsExt/EntityHolder/CkEntityHolder_Fragment.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_SmState_EntityScript;
class UCk_SmTask_EntityScript;
class UCk_SmTask_SubStateMachine;
class UCk_Utils_StateMachine_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ================================================================================================================
    // TAGS
    // ================================================================================================================

    CK_DEFINE_ECS_TAG(FTag_Sm_RequiresSetup);
    CK_DEFINE_ECS_TAG(FTag_Sm_Running);
    CK_DEFINE_ECS_TAG(FTag_Sm_Paused);

    CK_DEFINE_ECS_TAG(FTag_SmTask_Tick);
    CK_DEFINE_ECS_TAG(FTag_SmTask_EnterExit);
    CK_DEFINE_ECS_TAG(FTag_SmTask_ResultDirty);

    CK_DEFINE_ECS_TAG(FTag_SmCondition_Polled);
    CK_DEFINE_ECS_TAG(FTag_SmCondition_EventDriven);

    CK_DEFINE_ECS_TAG(FTag_Sm_TransitionQueued);

    // ================================================================================================================
    // FRAGMENTS
    // ================================================================================================================

    using FFragment_Sm_Params = FCk_Fragment_StateMachine_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FFragment_Sm_Context
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_Context);

        friend class FProcessor_Sm_Setup;

    private:
        FCk_Handle _GameEntityHandle;

    public:
        CK_PROPERTY_GET(_GameEntityHandle);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Sm_Context, _GameEntityHandle);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FFragment_Sm_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_Current);

        friend class FProcessor_Sm_HandleRequests;
        friend class FProcessor_Sm_Setup;
        friend class FProcessor_Sm_EndPlay;
        friend class ::UCk_Utils_StateMachine_UE;

    private:
        ECk_SmRunStatus _RunStatus = ECk_SmRunStatus::Stopped;
        FCk_Handle_SmState _CurrentStateHandle;
        TSubclassOf<UCk_SmState_EntityScript> _CurrentStateClass;

    public:
        CK_PROPERTY_GET(_RunStatus);
        CK_PROPERTY_GET(_CurrentStateHandle);
        CK_PROPERTY_GET(_CurrentStateClass);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FFragment_Sm_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_Requests);

        friend class FProcessor_Sm_HandleRequests;
        friend class FProcessor_Sm_Setup;
        friend class FProcessor_Sm_EvalTransitions;
        friend class ::UCk_Utils_StateMachine_UE;

        using RequestType = std::variant<
            FCk_Request_Sm_Start,
            FCk_Request_Sm_Stop,
            FCk_Request_Sm_Pause,
            FCk_Request_Sm_Resume,
            FCk_Request_Sm_Transition
        >;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FFragment_SmTransition_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_SmTransition_Params);

        friend class FProcessor_Sm_EvalTransitions;

    private:
        TSubclassOf<UCk_SmState_EntityScript> _TargetStateClass;
        int32 _Order = 0;

    public:
        CK_PROPERTY_GET(_TargetStateClass);
        CK_PROPERTY_GET(_Order);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_SmTransition_Params, _TargetStateClass, _Order);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FFragment_SmCondition_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_SmCondition_Current);

        friend class FProcessor_SmCondition_ResetEveryFrame;
        friend class FProcessor_SmCondition_Polled;
        friend class FProcessor_Sm_EvalTransitions;

    private:
        bool _IsSatisfied = false;
        ECk_SmConditionResetBehavior _ResetBehavior = ECk_SmConditionResetBehavior::ResetEveryFrame;

    public:
        CK_PROPERTY(_IsSatisfied);
        CK_PROPERTY(_ResetBehavior);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FFragment_SmTask_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_SmTask_Current);

        friend class FProcessor_SmTask_Tick;
        friend class FProcessor_SmTask_FireFinishedSignal;
        friend class ::UCk_SmTask_EntityScript;

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
    // RECORDS
    // ================================================================================================================

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfSmStates,      FCk_Handle_SmState);
    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfSmTransitions, FCk_Handle_SmTransition);
    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfSmTasks,       FCk_Handle_SmTask);
    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfSmConditions,  FCk_Handle_SmCondition);

    // ================================================================================================================
    // ENTITY-HOLDER BACK-REFERENCES
    // ================================================================================================================

    CK_DEFINE_ENTITY_HOLDER_AND_UTILS(TUtils_Sm_ParentState,           FFragment_Sm_ParentState,           FCk_Handle_SmState);
    CK_DEFINE_ENTITY_HOLDER_AND_UTILS(TUtils_Sm_ParentTransition,      FFragment_Sm_ParentTransition,      FCk_Handle_SmTransition);
    CK_DEFINE_ENTITY_HOLDER_AND_UTILS(TUtils_Sm_OwningStateMachine,    FFragment_Sm_OwningStateMachine,    FCk_Handle_StateMachine);

    // ================================================================================================================
    // SIGNALS
    // ================================================================================================================

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSTATEMACHINE_API,
        OnSmStateChanged,
        FCk_Delegate_Sm_OnStateChanged,
        FCk_Handle_StateMachine,
        FCk_Sm_Payload_OnStateChanged);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSTATEMACHINE_API,
        OnSmStarted,
        FCk_Delegate_Sm_OnStarted,
        FCk_Handle_StateMachine,
        FCk_Sm_Payload_OnStarted);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSTATEMACHINE_API,
        OnSmStopped,
        FCk_Delegate_Sm_OnStopped,
        FCk_Handle_StateMachine,
        FCk_Sm_Payload_OnStopped);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSTATEMACHINE_API,
        OnSmTaskFinished,
        FCk_Delegate_SmTask_OnFinished,
        FCk_Handle_SmTask,
        ECk_SmTaskResult);

    // ================================================================================================================

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Sm_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
