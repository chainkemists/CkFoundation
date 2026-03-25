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

    double DwellTimeSeconds = 0.0;
    bool HasBeenVisited = false;
    bool IsCurrentDwellLive = false;

    bool HasEntryBreakpoint = false;
    bool HasExitBreakpoint = false;
    bool IsBreakpointHit = false;

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

    bool HasBreakpoint = false;

    TArray<ImVec2> RouteWaypoints;
};

// --------------------------------------------------------------------------------------------------------------------

struct FCkHfsmViewer_HistoryEntry
{
    FString FromStateName;
    FString ToStateName;
    uint64 FrameNumber = 0;
    int32 TransitionOrder = -1;
    TArray<FString> ConditionNames;
    double RealTimeSeconds = 0.0;
};

// --------------------------------------------------------------------------------------------------------------------

struct FCkHfsmViewer_TimelineSegment
{
    FString StateName;
    int32 StateIndex = -1;
    double StartTime = 0.0;
    double EndTime = 0.0;
    ImU32 Color = 0;
    int32 HierarchyDepth = 0;
};

// --------------------------------------------------------------------------------------------------------------------

struct FCkHfsmViewer_TimelineBusyFrame
{
    double Time = 0.0;
    uint64 FrameNumber = 0;
    int32 TransitionCount = 0;
};

// --------------------------------------------------------------------------------------------------------------------

struct FCkHfsmViewer_TimelinePauseMarker
{
    double Time = 0.0;
    bool IsBreakpoint = false;
};

// --------------------------------------------------------------------------------------------------------------------

struct FCkHfsmViewer_FrameSegment
{
    double StartTime = 0.0;
    double EndTime = 0.0;
    uint64 StartFrame = 0;
    uint64 EndFrame = 0;
};

// --------------------------------------------------------------------------------------------------------------------

struct FCkHfsmViewer_RunInfo
{
    int32 RunIndex = 0;
    double StartTime = 0.0;
    double EndTime = 0.0;
    double Duration = 0.0;
    TArray<FCkHfsmViewer_HistoryEntry> History;
    TArray<FCkHfsmViewer_TimelineSegment> Segments;
    TArray<FCkHfsmViewer_TimelineBusyFrame> BusyFrames;
    TArray<FCkHfsmViewer_TimelinePauseMarker> PauseMarkers;
    TArray<FCkHfsmViewer_FrameSegment> FrameSegments;
};

// --------------------------------------------------------------------------------------------------------------------

struct FCkHfsmViewer_SmInfo
{
    FCk_Handle_StateMachine Handle;
    FCk_Handle GameEntity;
    FString DebugName;
    ECk_SmRunStatus RunStatus = ECk_SmRunStatus::Stopped;
    TSubclassOf<UCk_SmState_EntityScript> CurrentStateClass;
    TSubclassOf<UCk_SmState_EntityScript> InitialStateClass;
    bool IsTransitionQueued = false;

    int32 CurrentStateIndex = -1;
    TArray<FCkHfsmViewer_StateInfo> States;
    TArray<FCkHfsmViewer_TransitionInfo> Transitions;
    TArray<FCkHfsmViewer_HistoryEntry> History;

    FCkHfsmViewer_RunInfo CurrentRun;
    TArray<FCkHfsmViewer_RunInfo> CompletedRuns;

    bool IsPieDebugPaused = false;
    bool HasBreakpointHit = false;
    FString BreakpointHitDescription;
};

// --------------------------------------------------------------------------------------------------------------------

enum class ECkHfsmViewer_ViewMode : uint8
{
    Live,
    Scrub
};

// --------------------------------------------------------------------------------------------------------------------

struct FCkHfsmViewer_ScrubSnapshot
{
    int32 ActiveStateIndex = -1;
    FString ActiveStateName;
    double TimeInState = 0.0;
    int32 TakenTransitionSourceIdx = -1;
    int32 TakenTransitionTargetIdx = -1;
    int32 HistoryIndex = -1;
};

// --------------------------------------------------------------------------------------------------------------------

struct FCkHfsmViewer_ScrubState
{
    ECkHfsmViewer_ViewMode ViewMode = ECkHfsmViewer_ViewMode::Live;
    int32 SelectedRunIndex = -1;
    double ScrubTime = 0.0;
    int32 SelectedHistoryIndex = -1;
    double TimelineViewDuration = 10.0;
    float TimelineScrollX = 0.0f;
};

// --------------------------------------------------------------------------------------------------------------------

struct FCkHfsmViewer_Command
{
    enum class EType
    {
        None,
        ForceTransition,
        ResumeFromBreakpoint,
        ToggleStateEntryBreakpoint,
        ToggleStateExitBreakpoint,
        ToggleTransitionBreakpoint
    };

    EType Type = EType::None;
    TSubclassOf<UCk_SmState_EntityScript> TargetStateClass;
    int32 StateIndex = -1;
    int32 TransitionIndex = -1;
};

// --------------------------------------------------------------------------------------------------------------------

static auto
ComputeStateColor(
    const FString& InStateName) -> ImU32
{
    auto Hash = GetTypeHash(InStateName);
    auto Hue = static_cast<float>(Hash % 360) / 360.0f;
    constexpr auto Saturation = 0.70f;
    constexpr auto Value = 0.85f;

    auto C = Value * Saturation;
    auto X = C * (1.0f - FMath::Abs(FMath::Fmod(Hue * 6.0f, 2.0f) - 1.0f));
    auto M = Value - C;

    auto R = 0.0f;
    auto G = 0.0f;
    auto B = 0.0f;
    auto H6 = Hue * 6.0f;

    if (H6 < 1.0f)      { R = C; G = X; B = 0; }
    else if (H6 < 2.0f) { R = X; G = C; B = 0; }
    else if (H6 < 3.0f) { R = 0; G = C; B = X; }
    else if (H6 < 4.0f) { R = 0; G = X; B = C; }
    else if (H6 < 5.0f) { R = X; G = 0; B = C; }
    else                 { R = C; G = 0; B = X; }

    auto R8 = static_cast<uint8>((R + M) * 255.0f);
    auto G8 = static_cast<uint8>((G + M) * 255.0f);
    auto B8 = static_cast<uint8>((B + M) * 255.0f);

    return IM_COL32(R8, G8, B8, 255);
}

// --------------------------------------------------------------------------------------------------------------------
