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

    public:
        CK_PROPERTY_GET(_CachedStates);
        CK_PROPERTY_GET(_History);
        CK_PROPERTY_GET(_LastObservedStateClass);
    };
}

// --------------------------------------------------------------------------------------------------------------------
