#include "CkHfsmViewer_GraphRenderer.h"
#include "CkHfsmViewer_GraphRenderer_Constants.h"
#include "CkHfsmViewer_LayoutSolver.h"

#include "CkStateMachine/EntityScripts/CkSmState_EntityScript.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_GraphRenderer::
    RequestRelayout()
    -> void
{
    _NeedsRelayout = true;
    _CanvasOffset = {0, 0};
    _CanvasZoom = 1.0f;
    _SelectedNodeIndex = -1;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_GraphRenderer::
    RenderToolbar()
    -> void
{
    ImGui::Checkbox("Expand Nodes", &_ExpandAllNodes);

    ImGui::SameLine();
    if (ImGui::SmallButton("Layout Settings"))
    {
        ImGui::OpenPopup("LayoutSettingsPopup");
    }

    if (ImGui::BeginPopup("LayoutSettingsPopup"))
    {
        auto PrevHGap = _LayoutHorizontalGap;
        auto PrevVGap = _LayoutVerticalGap;
        auto PrevCrossingRuns = _LayoutCrossingMinRuns;
        auto PrevCrossingFails = _LayoutCrossingMaxFails;
        auto PrevBarycenter = _LayoutBarycenterIterations;
        auto PrevDummyHeight = _LayoutDummyNodeHeight;
        auto PrevPortFraction = _LayoutPortUsableFraction;

        constexpr auto FloatSpeed = 1.0f;
        constexpr auto IntSpeed = 0.1f;
        constexpr auto PortSpeed = 0.05f;

        ImGui::Text("Spacing");
        ImGui::Separator();
        ImGui::DragFloat("Horizontal Gap", &_LayoutHorizontalGap, FloatSpeed, 10.0f, 200.0f, "%.0f");
        ImGui::DragFloat("Vertical Gap", &_LayoutVerticalGap, FloatSpeed, 10.0f, 100.0f, "%.0f");

        ImGui::Spacing();
        ImGui::Text("Algorithm");
        ImGui::Separator();
        ImGui::DragInt("Crossing Runs", &_LayoutCrossingMinRuns, IntSpeed, 1, 20);
        ImGui::DragInt("Max Fails", &_LayoutCrossingMaxFails, IntSpeed, 1, 10);
        ImGui::DragInt("Refinement Iters", &_LayoutBarycenterIterations, IntSpeed, 1, 50);

        ImGui::Spacing();
        ImGui::Text("Routing");
        ImGui::Separator();
        ImGui::DragFloat("Dummy Node Height", &_LayoutDummyNodeHeight, FloatSpeed, 0.0f, 50.0f, "%.0f");
        ImGui::DragFloat("Port Spread", &_LayoutPortUsableFraction, PortSpeed, 0.1f, 1.0f, "%.2f");

        auto LayoutChanged =
            FMath::Abs(_LayoutHorizontalGap - PrevHGap) > 0.01f
            || FMath::Abs(_LayoutVerticalGap - PrevVGap) > 0.01f
            || _LayoutCrossingMinRuns != PrevCrossingRuns
            || _LayoutCrossingMaxFails != PrevCrossingFails
            || _LayoutBarycenterIterations != PrevBarycenter
            || FMath::Abs(_LayoutDummyNodeHeight - PrevDummyHeight) > 0.01f
            || FMath::Abs(_LayoutPortUsableFraction - PrevPortFraction) > 0.001f;

        if (LayoutChanged)
        {
            _NeedsRelayout = true;
        }

        ImGui::EndPopup();
    }
}

auto
    FCkHfsmViewer_GraphRenderer::
    Render(
        FCkHfsmViewer_SmInfo& InOutSmInfo,
        float InDeltaTime)
    -> FCkHfsmViewer_Command
{
    if (_NeedsRelayout)
    {
        // Find the structural initial state index (not the current active state)
        auto InitialStateIndex = 0;
        for (auto Index = 0; Index < InOutSmInfo.States.Num(); ++Index)
        {
            if (InOutSmInfo.States[Index].StateClass == InOutSmInfo.InitialStateClass)
            {
                InitialStateIndex = Index;
                break;
            }
        }

        CalculateLayout(InOutSmInfo.States, InOutSmInfo.Transitions, InitialStateIndex);
        StoreCachedPositions(InOutSmInfo.States);
        _NeedsRelayout = false;
    }
    else
    {
        ApplyCachedPositions(InOutSmInfo.States);
    }

    // Apply cached edge routes to transitions
    for (auto& Trans : InOutSmInfo.Transitions)
    {
        Trans.RouteWaypoints.Reset();
    }
    for (const auto& [TransIdx, Waypoints] : _CachedEdgeRoutes)
    {
        if (TransIdx >= 0 && TransIdx < InOutSmInfo.Transitions.Num())
        {
            InOutSmInfo.Transitions[TransIdx].RouteWaypoints = Waypoints;
        }
    }

    // Detect state change — drive border fade, transition flash, dwell flash
    if (InOutSmInfo.CurrentStateIndex != _PreviousCurrentStateIndex)
    {
        if (_PreviousCurrentStateIndex >= 0)
        {
            constexpr auto BorderFadeDuration = 0.8f;
            _BorderFadeTimer = BorderFadeDuration;

            // Find which transition was taken (from previous → current)
            _FlashTransitionSource = _PreviousCurrentStateIndex;
            _FlashTransitionTarget = InOutSmInfo.CurrentStateIndex;

            constexpr auto TransitionFlashDuration = 0.6f;
            _TransitionFlashTimer = TransitionFlashDuration;
        }

        // Flash the dwell badge on the newly entered state
        _FlashDwellNodeIndex = InOutSmInfo.CurrentStateIndex;
        constexpr auto DwellFlashDuration = 0.8f;
        _DwellFlashTimer = DwellFlashDuration;

        _PreviousCurrentStateIndex = InOutSmInfo.CurrentStateIndex;
    }

    if (_BorderFadeTimer > 0.0f)
    {
        _BorderFadeTimer = FMath::Max(0.0f, _BorderFadeTimer - InDeltaTime);
    }

    if (_TransitionFlashTimer > 0.0f)
    {
        _TransitionFlashTimer = FMath::Max(0.0f, _TransitionFlashTimer - InDeltaTime);
    }

    if (_DwellFlashTimer > 0.0f)
    {
        _DwellFlashTimer = FMath::Max(0.0f, _DwellFlashTimer - InDeltaTime);
    }

    return RenderCanvas(InOutSmInfo);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_GraphRenderer::
    RenderCanvas(
        const FCkHfsmViewer_SmInfo& InSmInfo)
    -> FCkHfsmViewer_Command
{
    auto Command = FCkHfsmViewer_Command{};

    auto* DrawList = ImGui::GetWindowDrawList();
    auto CanvasPos = ImGui::GetCursorScreenPos();
    auto CanvasSize = ImGui::GetContentRegionAvail();

    if (CanvasSize.x <= 0.0f || CanvasSize.y <= 0.0f)
    { return Command; }

    DrawList->AddRectFilled(CanvasPos, {CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y}, Colors::CanvasBackground);

    // Draw grid
    for (auto X = fmodf(_CanvasOffset.x, Layout::GridSpacing); X < CanvasSize.x; X += Layout::GridSpacing)
    {
        DrawList->AddLine(
            {CanvasPos.x + X, CanvasPos.y},
            {CanvasPos.x + X, CanvasPos.y + CanvasSize.y},
            Colors::CanvasGridLines);
    }

    for (auto Y = fmodf(_CanvasOffset.y, Layout::GridSpacing); Y < CanvasSize.y; Y += Layout::GridSpacing)
    {
        DrawList->AddLine(
            {CanvasPos.x, CanvasPos.y + Y},
            {CanvasPos.x + CanvasSize.x, CanvasPos.y + Y},
            Colors::CanvasGridLines);
    }

    auto CanvasOrigin = ImVec2{
        CanvasPos.x + CanvasSize.x * 0.5f + _CanvasOffset.x,
        CanvasPos.y + CanvasSize.y * 0.5f + _CanvasOffset.y
    };

    DrawList->PushClipRect(CanvasPos, {CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y}, true);

    auto HasSelection = (_SelectedNodeIndex >= 0 && _SelectedNodeIndex < InSmInfo.States.Num());

    // Use draw channels: 0 = lines/labels, 1 = nodes/selection, 2 = arrowheads (on top)
    constexpr auto ChannelCount = 3;
    constexpr auto ChannelLines = 0;
    constexpr auto ChannelNodes = 1;
    constexpr auto ChannelArrows = 2;
    DrawList->ChannelsSplit(ChannelCount);

    // Render transition lines + arrowheads (arrowheads go to a separate channel)
    DrawList->ChannelsSetCurrent(ChannelLines);

    // Pre-compute edge port assignments per node per exit side
    // Edges sharing the same exit/entry side of a node are spread along that side
    auto ComputeExitSide = [&](int32 InSourceIdx, int32 InTargetIdx) -> int32
    {
        if (InSourceIdx < 0 || InTargetIdx < 0
            || InSourceIdx >= InSmInfo.States.Num() || InTargetIdx >= InSmInfo.States.Num())
        { return 0; }

        const auto& Src = InSmInfo.States[InSourceIdx];
        const auto& Tgt = InSmInfo.States[InTargetIdx];

        auto SrcCx = Src.NodePosition.x + ComputeNodeWidth(Src) * 0.5f;
        auto SrcCy = Src.NodePosition.y + ComputeNodeHeight(Src) * 0.5f;
        auto TgtCx = Tgt.NodePosition.x + ComputeNodeWidth(Tgt) * 0.5f;
        auto TgtCy = Tgt.NodePosition.y + ComputeNodeHeight(Tgt) * 0.5f;

        auto EdgeDx = TgtCx - SrcCx;
        auto EdgeDy = TgtCy - SrcCy;

        constexpr auto SideRight = 0;
        constexpr auto SideLeft = 1;
        constexpr auto SideBottom = 2;
        constexpr auto SideTop = 3;

        if (FMath::Abs(EdgeDx) >= FMath::Abs(EdgeDy))
        {
            return EdgeDx >= 0.0f ? SideRight : SideLeft;
        }
        return EdgeDy >= 0.0f ? SideBottom : SideTop;
    };

    // Compute entry side (opposite perspective)
    auto ComputeEntrySide = [&](int32 InSourceIdx, int32 InTargetIdx) -> int32
    {
        if (InSourceIdx < 0 || InTargetIdx < 0
            || InSourceIdx >= InSmInfo.States.Num() || InTargetIdx >= InSmInfo.States.Num())
        { return 1; }

        const auto& Src = InSmInfo.States[InSourceIdx];
        const auto& Tgt = InSmInfo.States[InTargetIdx];

        auto SrcCx = Src.NodePosition.x + ComputeNodeWidth(Src) * 0.5f;
        auto SrcCy = Src.NodePosition.y + ComputeNodeHeight(Src) * 0.5f;
        auto TgtCx = Tgt.NodePosition.x + ComputeNodeWidth(Tgt) * 0.5f;
        auto TgtCy = Tgt.NodePosition.y + ComputeNodeHeight(Tgt) * 0.5f;

        auto EdgeDx = SrcCx - TgtCx;
        auto EdgeDy = SrcCy - TgtCy;

        constexpr auto SideRight = 0;
        constexpr auto SideLeft = 1;
        constexpr auto SideBottom = 2;
        constexpr auto SideTop = 3;

        if (FMath::Abs(EdgeDx) >= FMath::Abs(EdgeDy))
        {
            return EdgeDx >= 0.0f ? SideRight : SideLeft;
        }
        return EdgeDy >= 0.0f ? SideBottom : SideTop;
    };

    // Group edges by source node+side and target node+side
    // For each group, sort by the connected endpoint's perpendicular position
    // Source exit groups: sorted by target node's Y (for left/right sides) or X (for top/bottom)
    // Target entry groups: sorted by source node's Y or X

    struct FPortEntry
    {
        int32 TransitionIndex = -1;
        float SortKey = 0.0f;
    };

    // Key: packed as NodeIndex * 8 + Side * 2 + (IsSource ? 1 : 0)
    auto PackPortKey = [](int32 InNodeIndex, int32 InSide, bool InIsSource) -> int32
    {
        return InNodeIndex * 8 + InSide * 2 + (InIsSource ? 1 : 0);
    };

    auto PortGroups = TMap<int32, TArray<FPortEntry>>{};

    for (auto TransIdx = 0; TransIdx < InSmInfo.Transitions.Num(); ++TransIdx)
    {
        const auto& Trans = InSmInfo.Transitions[TransIdx];
        if (Trans.SourceStateIndex < 0 || Trans.SourceStateIndex >= InSmInfo.States.Num())
        { continue; }
        if (Trans.TargetStateIndex < 0 || Trans.TargetStateIndex >= InSmInfo.States.Num())
        { continue; }
        if (Trans.SourceStateIndex == Trans.TargetStateIndex)
        { continue; }

        auto SourceSide = ComputeExitSide(Trans.SourceStateIndex, Trans.TargetStateIndex);
        auto TargetSide = ComputeEntrySide(Trans.SourceStateIndex, Trans.TargetStateIndex);

        const auto& Tgt = InSmInfo.States[Trans.TargetStateIndex];
        const auto& Src = InSmInfo.States[Trans.SourceStateIndex];

        auto TgtCy = Tgt.NodePosition.y + ComputeNodeHeight(Tgt) * 0.5f;
        auto TgtCx = Tgt.NodePosition.x + ComputeNodeWidth(Tgt) * 0.5f;
        auto SrcCy = Src.NodePosition.y + ComputeNodeHeight(Src) * 0.5f;
        auto SrcCx = Src.NodePosition.x + ComputeNodeWidth(Src) * 0.5f;

        constexpr auto SideRight = 0;
        constexpr auto SideLeft = 1;

        auto SourceSortKey = (SourceSide == SideRight || SourceSide == SideLeft) ? TgtCy : TgtCx;
        auto TargetSortKey = (TargetSide == SideRight || TargetSide == SideLeft) ? SrcCy : SrcCx;

        constexpr auto IsSourceFlag = true;
        constexpr auto IsTargetFlag = false;

        auto SourceGroupKey = PackPortKey(Trans.SourceStateIndex, SourceSide, IsSourceFlag);
        auto TargetGroupKey = PackPortKey(Trans.TargetStateIndex, TargetSide, IsTargetFlag);

        PortGroups.FindOrAdd(SourceGroupKey).Add({TransIdx, SourceSortKey});
        PortGroups.FindOrAdd(TargetGroupKey).Add({TransIdx, TargetSortKey});
    }

    // Sort each group and compute offsets
    // Offset range: evenly spaced from -0.5 to +0.5
    auto SourcePortOffsets = TMap<int32, float>{};
    auto TargetPortOffsets = TMap<int32, float>{};

    for (auto& Pair : PortGroups)
    {
        auto& Entries = Pair.Value;
        Entries.Sort([](const FPortEntry& A, const FPortEntry& B) { return A.SortKey < B.SortKey; });

        auto Count = Entries.Num();
        for (auto Idx = 0; Idx < Count; ++Idx)
        {
            auto Offset = (Count == 1)
                ? 0.0f
                : (static_cast<float>(Idx) / static_cast<float>(Count - 1) - 0.5f);

            auto PackedKey = Pair.Key;
            auto IsSource = (PackedKey % 2) == 1;

            if (IsSource)
            {
                SourcePortOffsets.Add(Entries[Idx].TransitionIndex, Offset);
            }
            else
            {
                TargetPortOffsets.Add(Entries[Idx].TransitionIndex, Offset);
            }
        }
    }

    for (auto TransIdx = 0; TransIdx < InSmInfo.Transitions.Num(); ++TransIdx)
    {
        const auto& Transition = InSmInfo.Transitions[TransIdx];

        if (Transition.SourceStateIndex < 0 || Transition.SourceStateIndex >= InSmInfo.States.Num())
        { continue; }
        if (Transition.TargetStateIndex < 0 || Transition.TargetStateIndex >= InSmInfo.States.Num())
        { continue; }

        // Detect if a reverse transition exists (same pair, opposite direction)
        auto HasReverse = false;
        for (const auto& Other : InSmInfo.Transitions)
        {
            if (Other.SourceStateIndex == Transition.TargetStateIndex &&
                Other.TargetStateIndex == Transition.SourceStateIndex)
            {
                HasReverse = true;
                break;
            }
        }

        auto PerpOffset = HasReverse ? Layout::BiDirectionalOffset * _CanvasZoom : 0.0f;

        auto IsDimmed = HasSelection
            && Transition.SourceStateIndex != _SelectedNodeIndex
            && Transition.TargetStateIndex != _SelectedNodeIndex;

        // Flash the transition that was just taken
        auto TransitionFlash = 0.0f;
        if (_TransitionFlashTimer > 0.0f
            && Transition.SourceStateIndex == _FlashTransitionSource
            && Transition.TargetStateIndex == _FlashTransitionTarget)
        {
            constexpr auto TransitionFlashDuration = 0.6f;
            auto LinearProgress = _TransitionFlashTimer / TransitionFlashDuration;
            TransitionFlash = LinearProgress * LinearProgress;
        }

        // Scrub mode: persistent highlight for the taken transition
        if (_ScrubHighlightSource >= 0
            && Transition.SourceStateIndex == _ScrubHighlightSource
            && Transition.TargetStateIndex == _ScrubHighlightTarget)
        {
            TransitionFlash = FMath::Max(TransitionFlash, 0.8f);
        }

        const auto& Source = InSmInfo.States[Transition.SourceStateIndex];
        const auto& Target = InSmInfo.States[Transition.TargetStateIndex];

        auto SrcPortOff = SourcePortOffsets.Contains(TransIdx) ? SourcePortOffsets[TransIdx] : 0.0f;
        auto TgtPortOff = TargetPortOffsets.Contains(TransIdx) ? TargetPortOffsets[TransIdx] : 0.0f;

        RenderTransitionLine(DrawList, Source, Target, Transition, CanvasOrigin, PerpOffset, IsDimmed, TransitionFlash, SrcPortOff, TgtPortOff);
    }

    // Render state nodes on the node channel (above lines, below arrows)
    DrawList->ChannelsSetCurrent(ChannelNodes);

    for (auto Index = 0; Index < InSmInfo.States.Num(); ++Index)
    {
        const auto& State = InSmInfo.States[Index];
        auto IsQueuedAndCurrent = InSmInfo.IsTransitionQueued && State.IsCurrentState;
        auto IsDimmed = HasSelection && Index != _SelectedNodeIndex && NOT IsNodeConnectedToSelection(Index, InSmInfo);

        auto BorderFade = 0.0f;

        if (_BorderFadeTimer > 0.0f && NOT State.IsCurrentState && NOT IsQueuedAndCurrent)
        {
            constexpr auto BorderFadeDuration = 0.8f;
            auto LinearProgress = _BorderFadeTimer / BorderFadeDuration;

            if (NOT InSmInfo.History.IsEmpty())
            {
                const auto& LastEntry = InSmInfo.History.Last();
                if (LastEntry.FromStateName == State.StateName)
                {
                    // Ease-out: fast start, slow end
                    BorderFade = LinearProgress * LinearProgress;
                }
            }
        }

        auto DwellFlash = 0.0f;
        if (_DwellFlashTimer > 0.0f && Index == _FlashDwellNodeIndex)
        {
            constexpr auto DwellFlashDuration = 0.8f;
            auto LinearProgress = _DwellFlashTimer / DwellFlashDuration;
            DwellFlash = LinearProgress * LinearProgress;
        }

        RenderStateNode(DrawList, State, CanvasOrigin, IsQueuedAndCurrent, IsDimmed, BorderFade, DwellFlash);
    }

    // Selected node highlight glow
    if (HasSelection)
    {
        const auto& SelectedState = InSmInfo.States[_SelectedNodeIndex];
        auto NodeMin = ImVec2{
            CanvasOrigin.x + SelectedState.NodePosition.x * _CanvasZoom,
            CanvasOrigin.y + SelectedState.NodePosition.y * _CanvasZoom
        };

        auto NodeWidth = ComputeNodeWidth(SelectedState) * _CanvasZoom;
        auto NodeHeight = ComputeNodeHeight(SelectedState) * _CanvasZoom;
        auto NodeMax = ImVec2{NodeMin.x + NodeWidth, NodeMin.y + NodeHeight};
        auto Rounding = Layout::CornerRadius * _CanvasZoom;

        constexpr auto GlowColor = IM_COL32(0x90, 0xCA, 0xF9, 0x50);
        constexpr auto GlowThickness = 4.0f;
        DrawList->AddRect(
            {NodeMin.x - 2.0f, NodeMin.y - 2.0f},
            {NodeMax.x + 2.0f, NodeMax.y + 2.0f},
            GlowColor, Rounding, ImDrawFlags_None, GlowThickness * _CanvasZoom);
    }

    // Merge all channels: lines (bottom) → nodes → arrows (top)
    DrawList->ChannelsMerge();

    DrawList->PopClipRect();

    // Invisible button for canvas interaction (left for drag, middle for pan)
    ImGui::SetCursorScreenPos(CanvasPos);
    ImGui::InvisibleButton("##canvas_interaction", CanvasSize,
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle | ImGuiButtonFlags_MouseButtonRight);

    HandleCanvasInteraction(InSmInfo, CanvasOrigin);

    // Right-click context menu
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        auto MousePos = ImGui::GetIO().MousePos;
        auto HitIndex = HitTestNode(InSmInfo, CanvasOrigin, MousePos);

        if (HitIndex >= 0)
        {
            _ContextMenuNodeIndex = HitIndex;
            ImGui::OpenPopup("##NodeContextMenu");
        }
    }

    if (ImGui::BeginPopup("##NodeContextMenu"))
    {
        if (_ContextMenuNodeIndex >= 0 && _ContextMenuNodeIndex < InSmInfo.States.Num())
        {
            const auto& TargetState = InSmInfo.States[_ContextMenuNodeIndex];

            if (NOT TargetState.IsCurrentState)
            {
                auto ForceLabelAnsi = StringCast<ANSICHAR>(*FString::Printf(TEXT("Force Transition To: %s"), *TargetState.StateName));

                if (ImGui::MenuItem(ForceLabelAnsi.Get()))
                {
                    Command.Type = FCkHfsmViewer_Command::EType::ForceTransition;
                    Command.TargetStateClass = TargetState.StateClass;
                    _ContextMenuNodeIndex = -1;
                }

                ImGui::Separator();
            }

            auto EntryLabel = TargetState.HasEntryBreakpoint
                ? "Remove Entry Breakpoint"
                : "Add Entry Breakpoint";

            if (ImGui::MenuItem(EntryLabel))
            {
                Command.Type = FCkHfsmViewer_Command::EType::ToggleStateEntryBreakpoint;
                Command.StateIndex = _ContextMenuNodeIndex;
                _ContextMenuNodeIndex = -1;
            }

            auto ExitLabel = TargetState.HasExitBreakpoint
                ? "Remove Exit Breakpoint"
                : "Add Exit Breakpoint";

            if (ImGui::MenuItem(ExitLabel))
            {
                Command.Type = FCkHfsmViewer_Command::EType::ToggleStateExitBreakpoint;
                Command.StateIndex = _ContextMenuNodeIndex;
                _ContextMenuNodeIndex = -1;
            }
        }

        ImGui::EndPopup();
    }

    return Command;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_GraphRenderer::
    ComputeNodeWidth(
        const FCkHfsmViewer_StateInfo& InState) const
    -> float
{
    auto* CachedWidth = _CachedNodeWidths.Find(InState.StateName);

    if (CachedWidth)
    {
        return *CachedWidth;
    }

    auto NameAnsi = StringCast<ANSICHAR>(*InState.StateName);
    auto TextSize = ImGui::CalcTextSize(NameAnsi.Get());
    constexpr auto MinNodeWidth = 180.0f;
    constexpr auto TextPaddingX = 24.0f;
    auto NeededWidth = TextSize.x + TextPaddingX * 2.0f;

    auto Width = FMath::Max(MinNodeWidth, NeededWidth);
    const_cast<FCkHfsmViewer_GraphRenderer*>(this)->_CachedNodeWidths.Add(InState.StateName, Width);
    return Width;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_GraphRenderer::
    ComputeNodeHeight(
        const FCkHfsmViewer_StateInfo& InState) const
    -> float
{
    auto Height = Layout::HeaderHeight + Layout::NodePadding * 2.0f;

    if (_ExpandAllNodes)
    {
        if (InState.HasBeenVisited)
        {
            constexpr auto DwellBadgeRowHeight = 18.0f;
            Height += DwellBadgeRowHeight;
        }

        if (InState.Tasks.Num() > 0)
        {
            Height += Layout::TaskRowHeight * InState.Tasks.Num() + Layout::NodePadding;
        }
    }

    return Height;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_GraphRenderer::
    ApplyCachedPositions(
        TArray<FCkHfsmViewer_StateInfo>& InOutStates)
    -> void
{
    for (auto& State : InOutStates)
    {
        if (auto* CachedPos = _CachedPositions.Find(State.StateName))
        {
            State.NodePosition = *CachedPos;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_GraphRenderer::
    StoreCachedPositions(
        const TArray<FCkHfsmViewer_StateInfo>& InStates)
    -> void
{
    _CachedPositions.Reset();

    for (const auto& State : InStates)
    {
        _CachedPositions.Add(State.StateName, State.NodePosition);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_GraphRenderer::
    CalculateLayout(
        TArray<FCkHfsmViewer_StateInfo>& InOutStates,
        const TArray<FCkHfsmViewer_TransitionInfo>& InTransitions,
        int32 InInitialStateIndex)
    -> void
{
    // We need a mutable copy of transitions for route waypoints
    // But CalculateLayout takes const ref — we populate waypoints via Render()
    // For now, use the solver with a local mutable copy and copy waypoints back

    auto MutableTransitions = InTransitions;

    auto Solver = FCkHfsmViewer_LayoutSolver{};
    Solver.Solve(InOutStates, MutableTransitions, InInitialStateIndex, *this);

    // Copy waypoints back to the actual transitions
    // NOTE: InTransitions is const, so we store waypoints on the states' transitions
    // This will be handled in the Render call where we have mutable access
    _CachedEdgeRoutes.Reset();

    for (auto TransIdx = 0; TransIdx < MutableTransitions.Num(); ++TransIdx)
    {
        if (NOT MutableTransitions[TransIdx].RouteWaypoints.IsEmpty())
        {
            _CachedEdgeRoutes.Add(TransIdx, MoveTemp(MutableTransitions[TransIdx].RouteWaypoints));
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_GraphRenderer::
    HitTestNode(
        const FCkHfsmViewer_SmInfo& InSmInfo,
        ImVec2 InCanvasOrigin,
        ImVec2 InMousePos) const
    -> int32
{
    for (auto Index = 0; Index < InSmInfo.States.Num(); ++Index)
    {
        const auto& State = InSmInfo.States[Index];

        auto NodeMin = ImVec2{
            InCanvasOrigin.x + State.NodePosition.x * _CanvasZoom,
            InCanvasOrigin.y + State.NodePosition.y * _CanvasZoom
        };

        auto NodeWidth = ComputeNodeWidth(State) * _CanvasZoom;
        auto NodeHeight = ComputeNodeHeight(State) * _CanvasZoom;

        auto NodeMax = ImVec2{NodeMin.x + NodeWidth, NodeMin.y + NodeHeight};

        if (InMousePos.x >= NodeMin.x && InMousePos.x <= NodeMax.x &&
            InMousePos.y >= NodeMin.y && InMousePos.y <= NodeMax.y)
        {
            return Index;
        }
    }

    return -1;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_GraphRenderer::
    HandleCanvasInteraction(
        const FCkHfsmViewer_SmInfo& InSmInfo,
        ImVec2 InCanvasOrigin)
    -> void
{
    // Node dragging + selection (left mouse)
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        auto MousePos = ImGui::GetIO().MousePos;
        auto HitIndex = HitTestNode(InSmInfo, InCanvasOrigin, MousePos);

        _ClickStartMousePos = MousePos;
        _IsTrackingClick = true;

        if (HitIndex >= 0)
        {
            _DraggedNodeIndex = HitIndex;
            _DragStartMousePos = MousePos;
            _DragStartNodePos = InSmInfo.States[HitIndex].NodePosition;
        }
    }

    if (_DraggedNodeIndex >= 0 && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        auto MousePos = ImGui::GetIO().MousePos;
        auto DeltaX = (MousePos.x - _DragStartMousePos.x) / _CanvasZoom;
        auto DeltaY = (MousePos.y - _DragStartMousePos.y) / _CanvasZoom;

        auto NewPos = ImVec2{
            _DragStartNodePos.x + DeltaX,
            _DragStartNodePos.y + DeltaY
        };

        if (_DraggedNodeIndex < InSmInfo.States.Num())
        {
            const auto& StateName = InSmInfo.States[_DraggedNodeIndex].StateName;
            _CachedPositions.FindOrAdd(StateName) = NewPos;
        }
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        // Distinguish click vs drag: only set selection if mouse didn't move more than 3px
        if (_IsTrackingClick)
        {
            auto MousePos = ImGui::GetIO().MousePos;
            auto MoveDx = MousePos.x - _ClickStartMousePos.x;
            auto MoveDy = MousePos.y - _ClickStartMousePos.y;
            constexpr auto ClickThresholdSq = 3.0f * 3.0f;
            auto MoveDistSq = MoveDx * MoveDx + MoveDy * MoveDy;

            if (MoveDistSq <= ClickThresholdSq)
            {
                auto HitIndex = HitTestNode(InSmInfo, InCanvasOrigin, MousePos);

                if (HitIndex >= 0)
                {
                    _SelectedNodeIndex = (_SelectedNodeIndex == HitIndex) ? -1 : HitIndex;
                }
                else
                {
                    _SelectedNodeIndex = -1;
                }
            }

            _IsTrackingClick = false;
        }

        _DraggedNodeIndex = -1;
    }

    // Canvas panning (middle mouse)
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
    {
        auto Delta = ImGui::GetIO().MouseDelta;
        _CanvasOffset.x += Delta.x;
        _CanvasOffset.y += Delta.y;
    }

    // Zoom (scroll wheel)
    if (ImGui::IsItemHovered())
    {
        auto Scroll = ImGui::GetIO().MouseWheel;

        if (FMath::Abs(Scroll) > 0.0f)
        {
            constexpr auto ZoomSpeed = 0.1f;
            constexpr auto MinZoom = 0.3f;
            constexpr auto MaxZoom = 3.0f;

            _CanvasZoom = FMath::Clamp(_CanvasZoom + Scroll * ZoomSpeed, MinZoom, MaxZoom);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_GraphRenderer::
    IsNodeConnectedToSelection(
        int32 InNodeIndex,
        const FCkHfsmViewer_SmInfo& InSmInfo) const
    -> bool
{
    if (_SelectedNodeIndex < 0)
    { return false; }

    for (const auto& Transition : InSmInfo.Transitions)
    {
        if ((Transition.SourceStateIndex == _SelectedNodeIndex && Transition.TargetStateIndex == InNodeIndex)
            || (Transition.SourceStateIndex == InNodeIndex && Transition.TargetStateIndex == _SelectedNodeIndex))
        {
            return true;
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_GraphRenderer::
    SetScrubTransitionHighlight(
        int32 InSourceIndex,
        int32 InTargetIndex)
    -> void
{
    _ScrubHighlightSource = InSourceIndex;
    _ScrubHighlightTarget = InTargetIndex;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_GraphRenderer::
    ClearScrubTransitionHighlight()
    -> void
{
    _ScrubHighlightSource = -1;
    _ScrubHighlightTarget = -1;
}

// --------------------------------------------------------------------------------------------------------------------
