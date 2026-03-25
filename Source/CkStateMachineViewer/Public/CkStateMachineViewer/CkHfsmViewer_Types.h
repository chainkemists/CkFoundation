#pragma once

#include "CkStateMachine/CkStateMachine_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START
#include <imgui.h>
THIRD_PARTY_INCLUDES_END

// --------------------------------------------------------------------------------------------------------------------

class UCk_SmState_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

struct FCkHfsmViewer_ConditionInfo
{
    FCk_Handle Handle;
    FString ClassName;
    bool IsSatisfied = false;
    ECk_SmConditionMode Mode = ECk_SmConditionMode::Polled;
    ECk_SmConditionResetBehavior ResetBehavior = ECk_SmConditionResetBehavior::ResetEveryFrame;
};

// --------------------------------------------------------------------------------------------------------------------

struct FCkHfsmViewer_TaskInfo
{
    FCk_Handle Handle;
    FString ClassName;
    ECk_SmTaskMode Mode = ECk_SmTaskMode::EnterExitOnly;
    ECk_SmTaskResult LastResult = ECk_SmTaskResult::Running;
};

// --------------------------------------------------------------------------------------------------------------------

struct FCkHfsmViewer_StateInfo
{
    FCk_Handle Handle;
    TSubclassOf<UCk_SmState_EntityScript> StateClass;
    FString StateName;
    bool IsCurrentState = false;

    TArray<FCkHfsmViewer_TaskInfo> Tasks;

    ImVec2 NodePosition = {0, 0};
    ImVec2 NodeSize = {0, 0};
};

// --------------------------------------------------------------------------------------------------------------------

struct FCkHfsmViewer_TransitionInfo
{
    FCk_Handle Handle;
    int32 SourceStateIndex = -1;
    int32 TargetStateIndex = -1;
    int32 Order = 0;
    TSubclassOf<UCk_SmState_EntityScript> TargetStateClass;
    FString TargetStateName;

    TArray<FCkHfsmViewer_ConditionInfo> Conditions;
    bool AreAllConditionsSatisfied = false;
    int32 SatisfiedCount = 0;
    int32 TotalCount = 0;
};

// --------------------------------------------------------------------------------------------------------------------

struct FCkHfsmViewer_HistoryEntry
{
    FString FromStateName;
    FString ToStateName;
    uint64 FrameNumber = 0;
};

// --------------------------------------------------------------------------------------------------------------------

struct FCkHfsmViewer_SmInfo
{
    FCk_Handle_StateMachine Handle;
    FCk_Handle GameEntity;
    FString DebugName;
    ECk_SmRunStatus RunStatus = ECk_SmRunStatus::Stopped;
    TSubclassOf<UCk_SmState_EntityScript> CurrentStateClass;
    bool IsTransitionQueued = false;

    int32 CurrentStateIndex = -1;
    TArray<FCkHfsmViewer_StateInfo> States;
    TArray<FCkHfsmViewer_TransitionInfo> Transitions;
    TArray<FCkHfsmViewer_HistoryEntry> History;
};

// --------------------------------------------------------------------------------------------------------------------

struct FCkHfsmViewer_Command
{
    enum class EType { None, ForceTransition };

    EType Type = EType::None;
    TSubclassOf<UCk_SmState_EntityScript> TargetStateClass;
};

// --------------------------------------------------------------------------------------------------------------------
