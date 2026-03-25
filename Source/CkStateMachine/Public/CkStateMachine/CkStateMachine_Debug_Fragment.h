#pragma once

#include "CkStateMachine_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_SmState_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Sm_Debug;

    // ================================================================================================================
    // CACHED CONDITION
    // ================================================================================================================

    struct CKSTATEMACHINE_API FCk_SmDebug_CachedCondition
    {
        FString ClassName;
        ECk_SmConditionMode Mode = ECk_SmConditionMode::Polled;
        ECk_SmConditionResetBehavior ResetBehavior = ECk_SmConditionResetBehavior::ResetEveryFrame;
    };

    // ================================================================================================================
    // CACHED TRANSITION
    // ================================================================================================================

    struct CKSTATEMACHINE_API FCk_SmDebug_CachedTransition
    {
        TSubclassOf<UCk_SmState_EntityScript> SourceStateClass;
        TSubclassOf<UCk_SmState_EntityScript> TargetStateClass;
        int32 Order = 0;
        TArray<FCk_SmDebug_CachedCondition> Conditions;
    };

    // ================================================================================================================
    // CACHED TASK
    // ================================================================================================================

    struct CKSTATEMACHINE_API FCk_SmDebug_CachedTask
    {
        FString ClassName;
        ECk_SmTaskMode Mode = ECk_SmTaskMode::EnterExitOnly;
    };

    // ================================================================================================================
    // CACHED STATE
    // ================================================================================================================

    struct CKSTATEMACHINE_API FCk_SmDebug_CachedState
    {
        TSubclassOf<UCk_SmState_EntityScript> StateClass;
        FString StateName;
        TArray<FCk_SmDebug_CachedTransition> Transitions;
        TArray<FCk_SmDebug_CachedTask> Tasks;
    };

    // ================================================================================================================
    // HISTORY ENTRY
    // ================================================================================================================

    struct CKSTATEMACHINE_API FCk_SmDebug_HistoryEntry
    {
        TSubclassOf<UCk_SmState_EntityScript> FromStateClass;
        TSubclassOf<UCk_SmState_EntityScript> ToStateClass;
        FString FromStateName;
        FString ToStateName;
        uint64 FrameNumber = 0;
        int32 TransitionOrder = -1;
        TArray<FString> TransitionConditionNames;
        double RealTimeSeconds = 0.0;
    };

#if !UE_BUILD_SHIPPING
    // ================================================================================================================
    // LAST FIRED TRANSITION (per-frame cache consumed by debug processor)
    // ================================================================================================================

    struct CKSTATEMACHINE_API FFragment_Sm_Debug_LastFiredTransition
    {
        int32 Order = -1;
        TArray<FString> ConditionNames;
        double RealTimeSeconds = 0.0;
    };

    // ================================================================================================================
    // BREAKPOINTS — Stored on the SM entity (state/transition entities are transient)
    // ================================================================================================================

    struct CKSTATEMACHINE_API FCk_SmBreakpoint_TransitionKey
    {
        TSubclassOf<UCk_SmState_EntityScript> SourceStateClass;
        TSubclassOf<UCk_SmState_EntityScript> TargetStateClass;

        auto
        operator==(const FCk_SmBreakpoint_TransitionKey& InOther) const -> bool
        {
            return SourceStateClass == InOther.SourceStateClass
                && TargetStateClass == InOther.TargetStateClass;
        }

        friend auto
        GetTypeHash(const FCk_SmBreakpoint_TransitionKey& InKey) -> uint32
        {
            return HashCombine(
                GetTypeHash(InKey.SourceStateClass.Get()),
                GetTypeHash(InKey.TargetStateClass.Get()));
        }
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

    // ================================================================================================================
    // BREAKPOINT HIT INFO — Transient fragment, added at breakpoint site, read by viewer
    // ================================================================================================================

    struct CKSTATEMACHINE_API FFragment_Sm_Debug_BreakpointHit
    {
        CK_GENERATED_BODY(FFragment_Sm_Debug_BreakpointHit);

        FString Description;
        double RealTimeSeconds = 0.0;
    };
#endif

    // ================================================================================================================
    // RUN INFO (completed SM run snapshot)
    // ================================================================================================================

    struct CKSTATEMACHINE_API FCk_SmDebug_RunInfo
    {
        int32 RunIndex = 0;
        double StartRealTimeSeconds = 0.0;
        double EndRealTimeSeconds = 0.0;
        TArray<FCk_SmDebug_HistoryEntry> History;
    };

    // ================================================================================================================
    // DEBUG FRAGMENT
    // ================================================================================================================

    struct CKSTATEMACHINE_API FFragment_Sm_Debug
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_Debug);

        friend class FProcessor_Sm_Debug;

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
