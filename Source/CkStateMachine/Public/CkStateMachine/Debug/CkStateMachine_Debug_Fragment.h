#pragma once

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"
#include "CkStateMachine/Debug/CkStateMachine_Debug_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_StateMachineDebug_UE;
class UCk_SmState_EntityScript;
class UCk_SmTask_EntityScript;
class UCk_SmCondition_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Sm_Debug;
    class FProcessor_SmDebug_HandleRequests;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FCk_SmDebug_CachedCondition
    {
        FString ClassName;
        TSubclassOf<UCk_SmCondition_EntityScript> ScriptClass;
        ECk_SmConditionMode Mode = ECk_SmConditionMode::Polled;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FCk_SmDebug_CachedTransition
    {
        TSubclassOf<UCk_SmState_EntityScript> SourceStateClass;
        TSubclassOf<UCk_SmState_EntityScript> TargetStateClass;
        TArray<FCk_SmDebug_CachedCondition> Conditions;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FCk_SmDebug_CachedTask
    {
        FString ClassName;
        TSubclassOf<UCk_SmTask_EntityScript> ScriptClass;
        ECk_SmTaskMode Mode = ECk_SmTaskMode::EnterExitOnly;
        ECk_SmTaskResult LastResult = ECk_SmTaskResult::Running;
        bool HasSubStateMachine = false;
        TSubclassOf<UCk_SmState_EntityScript> SubSmInitialStateClass;
        FCk_Handle_StateMachine SubSmHandle;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FCk_SmDebug_CachedState
    {
        TSubclassOf<UCk_SmState_EntityScript> StateClass;
        TSubclassOf<UCk_SmState_EntityScript> ScriptClass;
        TSubclassOf<UCk_SmState_EntityScript> RequestedScriptClass;
        FString StateName;
        TArray<FCk_SmDebug_CachedTransition> Transitions;
        TArray<FCk_SmDebug_CachedTask> Tasks;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FCk_SmDebug_HistoryTaskSnapshot
    {
        FString TaskName;
        ECk_SmTaskResult Result = ECk_SmTaskResult::Running;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FCk_SmDebug_HistoryEntry
    {
        TSubclassOf<UCk_SmState_EntityScript> FromStateClass;
        TSubclassOf<UCk_SmState_EntityScript> ToStateClass;
        FString FromStateName;
        FString ToStateName;
        FString SubSmParentStateName;
        uint64 FrameNumber = 0;
        TArray<FString> TransitionConditionNames;
        TArray<FCk_SmDebug_HistoryTaskSnapshot> TaskSnapshots;
        double RealTimeSeconds = 0.0;
    };

    // --------------------------------------------------------------------------------------------------------------------
    // Mirrors FFragment_Sm_Requests so same-frame pumps each get their own entry, without coupling
    // the core state machine to the debug processor.

    struct CKSTATEMACHINE_API FFragment_SmDebug_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_SmDebug_Requests);

        friend class FProcessor_SmDebug_HandleRequests;
        friend class ::UCk_Utils_StateMachineDebug_UE;

        using RequestType = std::variant<
            FCk_Request_SmDebug_RecordTransition
        >;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

#if !UE_BUILD_SHIPPING
    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FFragment_Sm_Debug_LastFiredTransition
    {
        TArray<FString> ConditionNames;
        double RealTimeSeconds = 0.0;
    };

    // --------------------------------------------------------------------------------------------------------------------
    // Breakpoints live on the SM entity because state/transition entities are transient.

    struct CKSTATEMACHINE_API FCk_SmBreakpoint_TransitionKey
    {
        TSubclassOf<UCk_SmState_EntityScript> SourceStateClass;
        TSubclassOf<UCk_SmState_EntityScript> TargetStateClass;

        auto operator==(const FCk_SmBreakpoint_TransitionKey& InOther) const -> bool;

        friend CKSTATEMACHINE_API auto
        GetTypeHash(const FCk_SmBreakpoint_TransitionKey& InKey) -> uint32;
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FFragment_Sm_Breakpoints
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_Breakpoints);

        using FTransitionKey = FCk_SmBreakpoint_TransitionKey;

    private:
        TSet<TSubclassOf<UCk_SmState_EntityScript>> _EntryBreakpoints;
        TSet<TSubclassOf<UCk_SmState_EntityScript>> _ExitBreakpoints;
        TSet<FTransitionKey> _TransitionBreakpoints;

    public:
        CK_PROPERTY(_EntryBreakpoints);
        CK_PROPERTY(_ExitBreakpoints);
        CK_PROPERTY(_TransitionBreakpoints);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FFragment_Sm_Debug_BreakpointHit
    {
        CK_GENERATED_BODY(FFragment_Sm_Debug_BreakpointHit);

        FString Description;
        double RealTimeSeconds = 0.0;
    };
#endif

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FCk_SmDebug_RunInfo
    {
        int32 RunIndex = 0;
        double StartRealTimeSeconds = 0.0;
        double EndRealTimeSeconds = 0.0;
        TArray<FCk_SmDebug_HistoryEntry> History;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FFragment_Sm_Debug
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_Debug);

        friend class FProcessor_Sm_Debug;
        friend class FProcessor_SmDebug_HandleRequests;

    private:
        TMap<TSubclassOf<UCk_SmState_EntityScript>, FCk_SmDebug_CachedState> _CachedStates;
        TArray<FCk_SmDebug_HistoryEntry> _History;
        TSubclassOf<UCk_SmState_EntityScript> _LastObservedStateClass;
        double _CurrentStateEnteredAtRealTime = 0.0;

        TArray<FCk_SmDebug_RunInfo> _CompletedRuns;
        int32 _RunCounter = 0;
        ECk_SmRunStatus _LastObservedRunStatus = ECk_SmRunStatus::Stopped;
        double _CurrentRunStartRealTime = 0.0;

    public:
        CK_PROPERTY_GET(_CachedStates);
        CK_PROPERTY_GET(_History);
        CK_PROPERTY_GET(_LastObservedStateClass);
        CK_PROPERTY_GET(_CurrentStateEnteredAtRealTime);
        CK_PROPERTY_GET(_CompletedRuns);
        CK_PROPERTY_GET(_RunCounter);
    };
}

// --------------------------------------------------------------------------------------------------------------------
