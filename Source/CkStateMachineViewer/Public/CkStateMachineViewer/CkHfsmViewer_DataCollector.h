#pragma once

#include "CkHfsmViewer_Types.h"

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UWorld;

namespace ck { struct FFragment_Sm_Debug; }

// --------------------------------------------------------------------------------------------------------------------

class FCkHfsmViewer_DataCollector
{
public:
    auto
    Collect(
        UWorld* InWorld) -> void;

    auto
    Get_AllStateMachines() const -> const TArray<FCkHfsmViewer_SmInfo>&;

    auto
    CollectStateMachineByHandle(
        FCk_Handle_StateMachine InSmHandle) -> FCkHfsmViewer_SmInfo;

#if CK_BUILD_SM_GRAPH_WALK
    auto
    BuildStructuralSubSmData(
        FCk_Handle_StateMachine InParentSmHandle,
        TSubclassOf<UCk_SmState_EntityScript> InParentStateClass,
        TSubclassOf<UCk_SmState_EntityScript> InSubSmInitialStateClass) -> FCkHfsmViewer_SmInfo;
#endif

private:
    auto
    CollectStateMachine(
        const FCk_Handle& InSmHandle) -> FCkHfsmViewer_SmInfo;

    auto
    OverlayLiveData(
        const FCk_Handle& InStateHandle,
        int32 InCurrentStateIndex,
        const TMap<TSubclassOf<UCk_SmState_EntityScript>, int32>& InStateClassToIndex,
        FCkHfsmViewer_SmInfo& InOutSmInfo) -> void;

    static auto
    BuildTimelineSegments(
        const TArray<FCkHfsmViewer_HistoryEntry>& InHistory,
        double InRunStartTime,
        double InNow,
        const FString& InInitialStateName) -> TArray<FCkHfsmViewer_TimelineSegment>;

    static auto
    DetectBusyFrames(
        const TArray<FCkHfsmViewer_HistoryEntry>& InHistory,
        double InRunStartTime) -> TArray<FCkHfsmViewer_TimelineBusyFrame>;

    static auto
    BuildFrameSegments(
        const TArray<FCkHfsmViewer_HistoryEntry>& InHistory,
        double InRunStartTime,
        double InNow) -> TArray<FCkHfsmViewer_FrameSegment>;

    auto
    ComputeLogicalTime(
        double InWallClockTime) const -> double;

private:
    TArray<FCkHfsmViewer_SmInfo> _StateMachines;

    bool _IsPieDebugPaused = false;
    bool _WasPausedLastTick = false;
    double _PauseStartTime = 0.0;
    TArray<TPair<double, double>> _CompletedPauseIntervals;
    TArray<double> _BreakpointHitWallTimes;
    int32 _LastObservedRunCounter = -1;
};

// --------------------------------------------------------------------------------------------------------------------
