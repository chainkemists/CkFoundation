#pragma once

#include "CkHfsmViewer_Types.h"

// --------------------------------------------------------------------------------------------------------------------

class FCkHfsmViewer_GraphRenderer;

// --------------------------------------------------------------------------------------------------------------------

struct FCkHfsmViewer_LayoutEdge
{
    int32 Source = -1;
    int32 Target = -1;
    int32 OriginalTransitionIndex = -1;
    bool IsReversed = false;
};

struct FCkHfsmViewer_LayoutNode
{
    int32 OriginalStateIndex = -1;
    bool IsDummy = false;
    float Width = 0.0f;
    float Height = 0.0f;
    int32 Rank = -1;
    int32 PositionInRank = -1;
    float X = 0.0f;
    float Y = 0.0f;
};

// --------------------------------------------------------------------------------------------------------------------

struct FCkHfsmViewer_LayoutSolver
{
    auto
    Solve(
        TArray<FCkHfsmViewer_StateInfo>& InOutStates,
        TArray<FCkHfsmViewer_TransitionInfo>& InOutTransitions,
        int32 InRootIndex,
        const FCkHfsmViewer_GraphRenderer& InRenderer) -> void;

private:
    TArray<FCkHfsmViewer_LayoutNode> Nodes;
    TArray<FCkHfsmViewer_LayoutEdge> Edges;
    TArray<TArray<int32>> RankBuckets;
    int32 MaxRank = 0;
    int32 RealNodeCount = 0;
    TSet<int32> ReversedTransitionIndices;

    static auto
    RemoveCycles(
        TArray<FCkHfsmViewer_LayoutEdge>& InOutEdges,
        int32 InNodeCount,
        int32 InRootIndex) -> void;

    static auto
    ComputeLongestPathRanking(
        const TArray<FCkHfsmViewer_LayoutEdge>& InEdges,
        int32 InNodeCount,
        int32 InRootIndex) -> TArray<int32>;

    auto InsertDummyNodes(const FCkHfsmViewer_GraphRenderer& InRenderer) -> void;
    auto CountCrossings() const -> int32;
    auto UpdatePositionsInRank() -> void;
    auto BarycenterSweepForward() -> void;
    auto BarycenterSweepBackward() -> void;
    auto TransposeStep() -> bool;
    auto MinimizeCrossings(const FCkHfsmViewer_GraphRenderer& InRenderer) -> void;

    auto
    AssignCoordinates(
        const FCkHfsmViewer_GraphRenderer& InRenderer) -> void;

    auto
    EnforceMinSpacing(
        const TArray<int32>& InBucket,
        float InVerticalGap) -> void;

    auto
    WriteBack(
        TArray<FCkHfsmViewer_StateInfo>& InOutStates,
        TArray<FCkHfsmViewer_TransitionInfo>& InOutTransitions) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
