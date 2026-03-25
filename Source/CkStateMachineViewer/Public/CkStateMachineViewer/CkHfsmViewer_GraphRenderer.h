#pragma once

#include "CkHfsmViewer_Types.h"

// --------------------------------------------------------------------------------------------------------------------

class FCkHfsmViewer_GraphRenderer
{
public:
    auto
    Render(
        FCkHfsmViewer_SmInfo& InOutSmInfo,
        float InDeltaTime) -> FCkHfsmViewer_Command;

    auto
    RenderToolbar() -> void;

    auto
    RenderDetailPanel(
        const FCkHfsmViewer_SmInfo& InSmInfo) -> FCkHfsmViewer_Command;

    auto
    RequestRelayout() -> void;

    auto
    Get_SelectedNodeIndex() const -> int32 { return _SelectedNodeIndex; }

    auto
    Get_NeedsRelayout() const -> bool { return _NeedsRelayout; }

    auto
    SetScrubTransitionHighlight(
        int32 InSourceIndex,
        int32 InTargetIndex) -> void;

    auto
    ClearScrubTransitionHighlight() -> void;

    auto
    ComputeNodeWidth(
        const FCkHfsmViewer_StateInfo& InState) const -> float;

    auto
    ComputeNodeHeight(
        const FCkHfsmViewer_StateInfo& InState) const -> float;

    auto
    Get_LayoutHorizontalGap() const -> float { return _LayoutHorizontalGap; }

    auto
    Get_LayoutVerticalGap() const -> float { return _LayoutVerticalGap; }

    auto
    Get_LayoutCrossingMinRuns() const -> int32 { return _LayoutCrossingMinRuns; }

    auto
    Get_LayoutCrossingMaxFails() const -> int32 { return _LayoutCrossingMaxFails; }

    auto
    Get_LayoutBarycenterIterations() const -> int32 { return _LayoutBarycenterIterations; }

    auto
    Get_LayoutDummyNodeHeight() const -> float { return _LayoutDummyNodeHeight; }

    auto
    Get_LayoutPortUsableFraction() const -> float { return _LayoutPortUsableFraction; }

private:
    auto
    RenderCanvas(
        const FCkHfsmViewer_SmInfo& InSmInfo) -> FCkHfsmViewer_Command;

    auto
    RenderStateNode(
        ImDrawList* InDrawList,
        const FCkHfsmViewer_StateInfo& InState,
        ImVec2 InCanvasOrigin,
        bool InIsTransitionQueued,
        bool InIsDimmed,
        float InBorderFade,
        float InDwellFlash) -> void;

    auto
    RenderTransitionLine(
        ImDrawList* InDrawList,
        const FCkHfsmViewer_StateInfo& InSource,
        const FCkHfsmViewer_StateInfo& InTarget,
        const FCkHfsmViewer_TransitionInfo& InTransition,
        ImVec2 InCanvasOrigin,
        float InPerpOffset,
        bool InIsDimmed,
        float InFlash,
        float InSourcePortOffset,
        float InTargetPortOffset) -> void;

    auto
    IsNodeConnectedToSelection(
        int32 InNodeIndex,
        const FCkHfsmViewer_SmInfo& InSmInfo) const -> bool;

    auto
    CalculateLayout(
        TArray<FCkHfsmViewer_StateInfo>& InOutStates,
        const TArray<FCkHfsmViewer_TransitionInfo>& InTransitions,
        int32 InInitialStateIndex) -> void;

    auto
    HandleCanvasInteraction(
        const FCkHfsmViewer_SmInfo& InSmInfo,
        ImVec2 InCanvasOrigin) -> void;

    auto
    HitTestNode(
        const FCkHfsmViewer_SmInfo& InSmInfo,
        ImVec2 InCanvasOrigin,
        ImVec2 InMousePos) const -> int32;

    auto
    ApplyCachedPositions(
        TArray<FCkHfsmViewer_StateInfo>& InOutStates) -> void;

    auto
    StoreCachedPositions(
        const TArray<FCkHfsmViewer_StateInfo>& InStates) -> void;

private:
    bool _ExpandAllNodes = false;
    float _LayoutHorizontalGap = 50.0f;
    float _LayoutVerticalGap = 30.0f;
    int32 _LayoutCrossingMinRuns = 5;
    int32 _LayoutCrossingMaxFails = 4;
    int32 _LayoutBarycenterIterations = 12;
    float _LayoutDummyNodeHeight = 10.0f;
    float _LayoutPortUsableFraction = 0.7f;

    ImVec2 _CanvasOffset = {0, 0};
    float _CanvasZoom = 1.0f;
    bool _NeedsRelayout = true;
    TMap<FString, ImVec2> _CachedPositions;

    int32 _DraggedNodeIndex = -1;
    ImVec2 _DragStartMousePos = {0, 0};
    ImVec2 _DragStartNodePos = {0, 0};

    int32 _ContextMenuNodeIndex = -1;

    int32 _SelectedNodeIndex = -1;
    ImVec2 _ClickStartMousePos = {0, 0};
    bool _IsTrackingClick = false;

    int32 _PreviousCurrentStateIndex = -1;
    float _BorderFadeTimer = 0.0f;

    // Transition flash: source/target indices of the last taken transition
    int32 _FlashTransitionSource = -1;
    int32 _FlashTransitionTarget = -1;
    float _TransitionFlashTimer = 0.0f;

    // Dwell badge flash for newly entered state
    int32 _FlashDwellNodeIndex = -1;
    float _DwellFlashTimer = 0.0f;

    // Scrub mode: persistent highlight for the taken transition
    int32 _ScrubHighlightSource = -1;
    int32 _ScrubHighlightTarget = -1;

    TMap<FString, float> _CachedNodeWidths;

    // Edge routing waypoints from layout solver (keyed by transition index)
    TMap<int32, TArray<ImVec2>> _CachedEdgeRoutes;
};

// --------------------------------------------------------------------------------------------------------------------
