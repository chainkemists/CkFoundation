#pragma once

#include "CkStateMachine_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcsExt/EntityHolder/CkEntityHolder_Fragment.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"

// Per-feature fragment headers — included here for backward compatibility
#include "CkStateMachine/State/CkSmState_Fragment.h"
#include "CkStateMachine/Condition/CkSmCondition_Fragment.h"
#include "CkStateMachine/Transition/CkSmTransition_Fragment.h"
#include "CkStateMachine/Task/CkSmTask_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_SmState_EntityScript;
class UCk_SmTask_EntityScript;
class UCk_SmTask_SubStateMachine;
class UCk_Utils_StateMachine_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_SmState_Evaluate;

    // ================================================================================================================
    // TAGS
    // ================================================================================================================

    CK_DEFINE_ECS_TAG(FTag_Sm_RequiresSetup);
    CK_DEFINE_ECS_TAG(FTag_Sm_Running);
    CK_DEFINE_ECS_TAG(FTag_Sm_Paused);

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

    // Hierarchy prefix applied to every state spawned within this StateMachine.
    // Empty for top-level SMs. Seeded by sub-StateMachine spawners with the hosting state's hierarchy.
    struct CKSTATEMACHINE_API FFragment_Sm_ParentHierarchy
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_ParentHierarchy);

    private:
        TArray<FGameplayTag> _ParentHierarchy;

    public:
        CK_PROPERTY_GET(_ParentHierarchy);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Sm_ParentHierarchy, _ParentHierarchy);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // C++-only. Holds the list of state-override entries installed on this StateMachine.
    // Consulted inside UCk_Utils_SmState_UE::Create to swap the spawned class.
    struct CKSTATEMACHINE_API FFragment_Sm_StateOverrides
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_StateOverrides);

        friend class FProcessor_Sm_HandleRequests;
        friend class ::UCk_Utils_StateMachine_UE;
        friend class ::UCk_Utils_SmState_UE;

        struct FEntry
        {
            // Num()==1 -> loose match by leaf tag only (any hierarchy).
            // Num()>1  -> exact root->leaf element-wise match against prospective state's hierarchy.
            TArray<FGameplayTag> _OverriddenStateHierarchy;
            TSubclassOf<UCk_SmState_EntityScript> _OverridingStateClass;
        };

    private:
        TArray<FEntry> _Overrides;

    public:
        CK_PROPERTY_GET(_Overrides);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FFragment_Sm_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_Requests);

        friend class FProcessor_Sm_HandleRequests;
        friend class FProcessor_Sm_Setup;
        friend class FProcessor_SmState_Evaluate;
        friend class ::UCk_Utils_StateMachine_UE;

        using RequestType = std::variant<
            FCk_Request_Sm_Start,
            FCk_Request_Sm_Stop,
            FCk_Request_Sm_Pause,
            FCk_Request_Sm_Resume,
            FCk_Request_Sm_Transition,
            FCk_Request_Sm_OverrideState
        >;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // ================================================================================================================
    // ENTITY-HOLDER BACK-REFERENCES
    // ================================================================================================================

    CK_DEFINE_ENTITY_HOLDER_AND_UTILS(TUtils_Sm_OwningStateMachine, FFragment_Sm_OwningStateMachine, FCk_Handle_StateMachine);

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
