#pragma once

#include "CkStateMachine_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcsExt/EntityHolder/CkEntityHolder_Fragment.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"

#include <StructUtils/InstancedStruct.h>

// Per-feature fragment headers — included here for backward compatibility
#include "CkStateMachine/State/CkSmState_Fragment.h"
#include "CkStateMachine/Condition/CkSmCondition_Fragment.h"
#include "CkStateMachine/Transition/CkSmTransition_Fragment.h"
#include "CkStateMachine/Task/CkSmTask_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_SmState_EntityScript;
class UCk_SmTask_EntityScript;
class UCk_SmTask_SubStateMachine;
class UCk_EntityScript_UE;
class UCk_Utils_StateMachine_UE;
class UCk_Utils_SmTask_UE;
class UCk_Utils_SmCondition_UE;

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

    // Marks an SM child entity (Task/Condition) whose user-authored EntityScript has been
    // deferred. A commit processor materializes the script before EntityScript processors
    // see the entity. Strip this tag + FFragment_SmScript_PendingAttach to cancel the
    // attach (e.g. when the child is removed before commit runs).
    CK_DEFINE_ECS_TAG(FTag_SmScript_PendingAttach);

    // ================================================================================================================
    // FRAGMENTS
    // ================================================================================================================

    // Carries the EntityScript class (and optional spawn params) to attach to an SM child
    // entity (Task/Condition) when the commit processor runs. Deferring the attach avoids
    // a same-frame race with FProcessor_EntityScript_ContinueConstruction when the child
    // is removed before BeginPlay runs — see CkEntityLifetime_Fragment.cpp destruction
    // pipeline, where CK_IGNORE_PENDING_KILL does NOT exclude FTag_DestroyEntity_Initiate.
    struct CKSTATEMACHINE_API FFragment_SmScript_PendingAttach
    {
    public:
        CK_GENERATED_BODY(FFragment_SmScript_PendingAttach);

        friend class FProcessor_SmScript_CommitPendingAttach;
        friend class ::UCk_Utils_SmTask_UE;
        friend class ::UCk_Utils_SmCondition_UE;

    private:
        TSubclassOf<UCk_EntityScript_UE> _ScriptClass;
        FInstancedStruct                 _SpawnParams;

    public:
        CK_PROPERTY_GET(_ScriptClass);
        CK_PROPERTY_GET(_SpawnParams);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_SmScript_PendingAttach, _ScriptClass, _SpawnParams);
    };

    // --------------------------------------------------------------------------------------------------------------------


    using FFragment_Sm_Params = FCk_Fragment_StateMachine_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FFragment_Sm_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_Current);

        friend class FProcessor_Sm_HandleRequests;
        friend class FProcessor_Sm_CommitPendingTransition;
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

    // Presence on an SM means "transition mid-flight": the previous state's Request_Exit has
    // been issued but the new state has not been entered. FProcessor_Sm_CommitPendingTransition
    // filters on this fragment, lands the entry once the exit cascade has drained, then removes it.
    struct CKSTATEMACHINE_API FFragment_Sm_PendingTransition
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_PendingTransition);

        friend class FProcessor_Sm_HandleRequests;
        friend class FProcessor_Sm_CommitPendingTransition;

    private:
        FCk_Handle_SmState _PreviousStateHandle;
        TSubclassOf<UCk_SmState_EntityScript> _PreviousStateClass;
        TSubclassOf<UCk_SmState_EntityScript> _TargetStateClass;

    public:
        CK_PROPERTY_GET(_PreviousStateHandle);
        CK_PROPERTY_GET(_PreviousStateClass);
        CK_PROPERTY_GET(_TargetStateClass);
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
    // Each entry caches the tags from the override class's Get_StatesToOverride() CDO call.
    struct CKSTATEMACHINE_API FFragment_Sm_StateOverrides
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_StateOverrides);

        friend class FProcessor_Sm_HandleRequests;
        friend class ::UCk_Utils_StateMachine_UE;
        friend class ::UCk_Utils_SmState_UE;

        struct FEntry
        {
            TSubclassOf<UCk_SmState_EntityScript> _OverrideStateClass;
            TArray<FGameplayTag> _CachedStatesToOverride;
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
            FCk_Request_Sm_AddOverrideState
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
