#include "CkHfsmViewer_GraphRenderer.h"
#include "CkHfsmViewer_GraphRenderer_Constants.h"
#include "CkHfsmViewer_LayoutSolver.h"

#include "CkStateMachine/EntityScripts/CkSmState_EntityScript.h"

#if CK_BUILD_SM_GRAPH_WALK
#include "CkStateMachine/CkStateMachine_Debug_GraphWalk_Fragment.h"
#endif

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
    _GraphWalkWasComplete = false;
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

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_GraphRenderer::
    PrepareCompoundNodes(
        FCkHfsmViewer_SmInfo& InOutSmInfo,
        const TMap<FString, FCkHfsmViewer_SmInfo>& InSubSmData)
    -> void
{
    _SubSmRanges.Reset();
    _ParentStateCount = InOutSmInfo.States.Num();

    for (const auto& [ParentStateName, SubSmInfo] : InSubSmData)
    {
        if (SubSmInfo.States.IsEmpty())
        { continue; }

        // Find the parent state index
        auto ParentStateIndex = -1;
        for (auto Idx = 0; Idx < _ParentStateCount; ++Idx)
        {
            if (InOutSmInfo.States[Idx].StateName == ParentStateName)
            {
                ParentStateIndex = Idx;
                break;
            }
        }

        if (ParentStateIndex < 0)
        { continue; }

        auto Range = FSubSmRange{};
        Range.ParentStateIndex = ParentStateIndex;

        // Phase 1: Run internal layout on sub-SM states
        auto InternalStates = SubSmInfo.States;
        auto InternalTransitions = SubSmInfo.Transitions;

        // Prefix state names for cache uniqueness
        for (auto& State : InternalStates)
        {
            State.StateName = FString::Printf(TEXT("%s/%s"), *ParentStateName, *State.StateName);
            State.IsSubSmNode = true;
            State.SubSmParentStateName = ParentStateName;
            State.SubSmParentStateIndex = ParentStateIndex;
        }

        if (_NeedsRelayout)
        {
            // Find initial state for internal layout
            auto InternalInitialIndex = 0;
            for (auto Idx = 0; Idx < SubSmInfo.States.Num(); ++Idx)
            {
                if (SubSmInfo.States[Idx].StateClass == SubSmInfo.InitialStateClass)
                {
                    InternalInitialIndex = Idx;
                    break;
                }
            }

            CalculateLayout(InternalStates, InternalTransitions, InternalInitialIndex);

            // Store internal positions in cache (prefixed names)
            for (const auto& State : InternalStates)
            {
                _CachedPositions.Add(State.StateName, State.NodePosition);
            }
        }
        else
        {
            // Apply cached positions to internal states
            for (auto& State : InternalStates)
            {
                if (auto* CachedPos = _CachedPositions.Find(State.StateName))
                {
                    State.NodePosition = *CachedPos;
                }
            }
        }

        // Phase 2: Normalize internal positions to (0,0) origin and compute bounding box
        auto MinX = FLT_MAX;
        auto MinY = FLT_MAX;
        auto MaxX = -FLT_MAX;
        auto MaxY = -FLT_MAX;

        for (const auto& State : InternalStates)
        {
            auto W = ComputeNodeWidth(State);
            auto H = ComputeNodeHeight(State);
            MinX = FMath::Min(MinX, State.NodePosition.x);
            MinY = FMath::Min(MinY, State.NodePosition.y);
            MaxX = FMath::Max(MaxX, State.NodePosition.x + W);
            MaxY = FMath::Max(MaxY, State.NodePosition.y + H);
        }

        // Normalize positions so top-left of bounding box is at (0,0)
        for (auto& State : InternalStates)
        {
            State.NodePosition.x -= MinX;
            State.NodePosition.y -= MinY;
        }

        auto InternalWidth = MaxX - MinX;
        auto InternalHeight = MaxY - MinY;

        constexpr auto LabelHeight = 16.0f;
        auto CompoundWidth = InternalWidth + Layout::SubSmClusterPadding * 2.0f;
        auto CompoundHeight = InternalHeight + Layout::SubSmClusterPadding * 2.0f + LabelHeight;

        // Phase 3: Create compound node in parent graph
        auto CompoundNode = FCkHfsmViewer_StateInfo{};
        CompoundNode.StateName = FString::Printf(TEXT("__box_%s"), *ParentStateName);
        CompoundNode.IsCompoundNode = true;
        CompoundNode.CompoundNodeWidth = CompoundWidth;
        CompoundNode.CompoundNodeHeight = CompoundHeight;
        CompoundNode.CompoundNodeParentStateIndex = ParentStateIndex;

        Range.CompoundNodeIndex = InOutSmInfo.States.Num();
        InOutSmInfo.States.Add(MoveTemp(CompoundNode));

        // Create synthetic connector transition: parent state -> compound node
        auto ConnectorTrans = FCkHfsmViewer_TransitionInfo{};
        ConnectorTrans.SourceStateIndex = ParentStateIndex;
        ConnectorTrans.TargetStateIndex = Range.CompoundNodeIndex;
        ConnectorTrans.IsSubSmConnector = true;
        Range.ConnectorTransitionIndex = InOutSmInfo.Transitions.Num();
        InOutSmInfo.Transitions.Add(MoveTemp(ConnectorTrans));

        // Store internal data for FinalizeInternalPositions
        Range.InternalStates = MoveTemp(InternalStates);
        Range.InternalTransitions = MoveTemp(InternalTransitions);

        _SubSmRanges.Add(ParentStateName, MoveTemp(Range));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_GraphRenderer::
    FinalizeInternalPositions(
        FCkHfsmViewer_SmInfo& InOutSmInfo)
    -> void
{
    constexpr auto LabelHeight = 16.0f;

    for (auto& [ParentName, Range] : _SubSmRanges)
    {
        if (Range.CompoundNodeIndex < 0 || Range.CompoundNodeIndex >= InOutSmInfo.States.Num())
        { continue; }

        auto BoxPos = InOutSmInfo.States[Range.CompoundNodeIndex].NodePosition;
        auto BoxOriginX = BoxPos.x + Layout::SubSmClusterPadding;
        auto BoxOriginY = BoxPos.y + Layout::SubSmClusterPadding + LabelHeight;

        // Compute final absolute positions for internal states
        Range.FirstInternalStateIndex = InOutSmInfo.States.Num();

        for (auto& State : Range.InternalStates)
        {
            State.NodePosition.x += BoxOriginX;
            State.NodePosition.y += BoxOriginY;

            // If not a relayout frame, check if user has manually dragged this state
            if (NOT _NeedsRelayout)
            {
                if (auto* CachedPos = _CachedPositions.Find(State.StateName))
                {
                    State.NodePosition = *CachedPos;
                }
            }
            else
            {
                // Store the finalized absolute position in cache
                _CachedPositions.Add(State.StateName, State.NodePosition);
            }

            InOutSmInfo.States.Add(State);
        }

        Range.InternalStateCount = Range.InternalStates.Num();

        // Append internal transitions with remapped indices
        Range.FirstInternalTransitionIndex = InOutSmInfo.Transitions.Num();

        auto BaseIndex = Range.FirstInternalStateIndex;

        for (auto Trans : Range.InternalTransitions)
        {
            Trans.SourceStateIndex += BaseIndex;
            Trans.TargetStateIndex += BaseIndex;
            Trans.IsSubSmTransition = true;
            InOutSmInfo.Transitions.Add(MoveTemp(Trans));
        }

        Range.InternalTransitionCount = Range.InternalTransitions.Num();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_GraphRenderer::
    Render(
        FCkHfsmViewer_SmInfo& InOutSmInfo,
        const TMap<FString, FCkHfsmViewer_SmInfo>& InSubSmData,
        float InDeltaTime)
    -> FCkHfsmViewer_Command
{
    // Trigger one-time relayout when graph walk completes (all states discovered)
#if CK_BUILD_SM_GRAPH_WALK
    auto SmHandle = static_cast<FCk_Handle>(InOutSmInfo.Handle);

    if (ck::IsValid(SmHandle) && SmHandle.Has<ck::FFragment_Sm_Debug_GraphDefinition>())
    {
        auto IsComplete = SmHandle.Get<ck::FFragment_Sm_Debug_GraphDefinition>().Get_IsComplete();

        if (IsComplete && NOT _GraphWalkWasComplete)
        {
            _NeedsRelayout = true;
        }

        _GraphWalkWasComplete = IsComplete;
    }
#endif

    // Phase 1-3: Run internal sub-SM layouts, create compound nodes + connectors
    PrepareCompoundNodes(InOutSmInfo, InSubSmData);

    // At this point: States = [parent states] + [compound nodes]
    // Transitions = [parent transitions] + [connector transitions]

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
    }
    else
    {
        ApplyCachedPositions(InOutSmInfo.States);
    }

    // Phase 4: Finalize internal sub-SM state positions based on compound node positions
    // Must run before clearing _NeedsRelayout so FinalizeInternalPositions knows if this
    // is a relayout frame (needs to store positions) or a normal frame (uses cached positions)
    FinalizeInternalPositions(InOutSmInfo);

    _NeedsRelayout = false;

    // Clamp selection index after compound node expansion
    if (_SelectedNodeIndex >= InOutSmInfo.States.Num())
    {
        _SelectedNodeIndex = -1;
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

            _FlashTransitionSource = _PreviousCurrentStateIndex;
            _FlashTransitionTarget = InOutSmInfo.CurrentStateIndex;

            constexpr auto TransitionFlashDuration = 0.6f;
            _TransitionFlashTimer = TransitionFlashDuration;
        }

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

    // Sub-SM compound node boxes (draw on lines channel, below nodes)
    DrawList->ChannelsSetCurrent(ChannelLines);

    for (const auto& [ParentName, Range] : _SubSmRanges)
    {
        if (Range.CompoundNodeIndex < 0 || Range.CompoundNodeIndex >= InSmInfo.States.Num())
        { continue; }

        const auto& CompoundNode = InSmInfo.States[Range.CompoundNodeIndex];

        auto BgMin = ImVec2{
            CanvasOrigin.x + CompoundNode.NodePosition.x * _CanvasZoom,
            CanvasOrigin.y + CompoundNode.NodePosition.y * _CanvasZoom
        };
        auto BgMax = ImVec2{
            BgMin.x + CompoundNode.CompoundNodeWidth * _CanvasZoom,
            BgMin.y + CompoundNode.CompoundNodeHeight * _CanvasZoom
        };

        auto ParentStateIndex = Range.ParentStateIndex;
        auto IsParentActive = (ParentStateIndex >= 0
            && ParentStateIndex < InSmInfo.States.Num()
            && InSmInfo.States[ParentStateIndex].IsCurrentState);

        constexpr auto ActiveBgAlpha = 0xA0;
        constexpr auto InactiveBgAlpha = 0x80;
        auto BgAlpha = static_cast<uint8_t>(IsParentActive ? ActiveBgAlpha : InactiveBgAlpha);

        auto Rounding = 10.0f * _CanvasZoom;
        DrawList->AddRectFilled(BgMin, BgMax, IM_COL32(0x12, 0x15, 0x22, BgAlpha), Rounding);

        auto BorderAlpha = IsParentActive ? 0x50 : 0x38;
        DrawList->AddRect(BgMin, BgMax, IM_COL32(0x42, 0xA5, 0xF5, BorderAlpha), Rounding, ImDrawFlags_None, 1.0f * _CanvasZoom);

        // Centered label with pill background straddling the top edge
        auto LabelAnsi = StringCast<ANSICHAR>(*FString::Printf(TEXT("Sub-SM [%s]"), *ParentName));
        auto LabelColor = IsParentActive ? Colors::SubSmLabel : ApplyDimAlpha(Colors::SubSmLabel, 0.5f);
        constexpr auto LabelFontSize = 12.0f;
        auto LabelTextSize = ImGui::CalcTextSize(LabelAnsi.Get());
        auto LabelCenterX = (BgMin.x + BgMax.x) * 0.5f;

        constexpr auto PillPadX = 6.0f;
        constexpr auto PillPadY = 2.0f;
        auto PillMin = ImVec2{
            LabelCenterX - LabelTextSize.x * 0.5f - PillPadX * _CanvasZoom,
            BgMin.y - LabelTextSize.y * 0.5f - PillPadY * _CanvasZoom
        };
        auto PillMax = ImVec2{
            LabelCenterX + LabelTextSize.x * 0.5f + PillPadX * _CanvasZoom,
            BgMin.y + LabelTextSize.y * 0.5f + PillPadY * _CanvasZoom
        };

        constexpr auto PillRounding = 4.0f;
        DrawList->AddRectFilled(PillMin, PillMax, IM_COL32(0x12, 0x15, 0x22, 0xFF), PillRounding * _CanvasZoom);
        DrawList->AddRect(PillMin, PillMax, IM_COL32(0x42, 0xA5, 0xF5, BorderAlpha), PillRounding * _CanvasZoom, ImDrawFlags_None, 1.0f * _CanvasZoom);

        auto LabelPos = ImVec2{
            LabelCenterX - LabelTextSize.x * 0.5f,
            BgMin.y - LabelTextSize.y * 0.5f
        };
        DrawList->AddText(nullptr, LabelFontSize * _CanvasZoom, LabelPos, LabelColor, LabelAnsi.Get());
    }

    // Entry indicator — small "Entry" label with arrow pointing to initial state
    DrawList->ChannelsSetCurrent(ChannelLines);
    {
        auto InitialIdx = -1;
        for (auto Idx = 0; Idx < InSmInfo.States.Num(); ++Idx)
        {
            if (InSmInfo.States[Idx].StateClass == InSmInfo.InitialStateClass
                && NOT InSmInfo.States[Idx].IsCompoundNode)
            {
                InitialIdx = Idx;
                break;
            }
        }

        if (InitialIdx >= 0)
        {
            const auto& InitialState = InSmInfo.States[InitialIdx];
            auto NodeLeft = CanvasOrigin.x + InitialState.NodePosition.x * _CanvasZoom;
            auto NodeCenterY = CanvasOrigin.y + InitialState.NodePosition.y * _CanvasZoom
                + ComputeNodeHeight(InitialState) * 0.5f * _CanvasZoom;

            constexpr auto EntryOffset = 50.0f;
            constexpr auto TriangleSize = 5.0f;
            constexpr auto EntryFontSize = 11.0f;
            auto EntryColor = ApplyDimAlpha(Colors::TextSecondary, 0.7f);

            auto TriangleTipX = NodeLeft - 8.0f * _CanvasZoom;
            auto EntryLabelAnsi = StringCast<ANSICHAR>(TEXT("Entry"));
            auto EntryTextSize = ImGui::CalcTextSize(EntryLabelAnsi.Get());

            auto LabelRight = TriangleTipX - TriangleSize * _CanvasZoom - 4.0f * _CanvasZoom;
            auto LabelLeft = LabelRight - EntryTextSize.x;

            // Connecting line
            DrawList->AddLine(
                {LabelRight + 2.0f * _CanvasZoom, NodeCenterY},
                {NodeLeft, NodeCenterY},
                EntryColor, 1.0f * _CanvasZoom);

            // Triangle arrow
            auto TriTip = ImVec2{TriangleTipX, NodeCenterY};
            auto TriLeft = ImVec2{TriangleTipX - TriangleSize * 1.5f * _CanvasZoom, NodeCenterY - TriangleSize * _CanvasZoom};
            auto TriRight = ImVec2{TriangleTipX - TriangleSize * 1.5f * _CanvasZoom, NodeCenterY + TriangleSize * _CanvasZoom};
            DrawList->AddTriangleFilled(TriTip, TriLeft, TriRight, EntryColor);

            // "Entry" text
            auto EntryTextPos = ImVec2{
                LabelLeft,
                NodeCenterY - EntryTextSize.y * 0.5f
            };
            DrawList->AddText(nullptr, EntryFontSize * _CanvasZoom, EntryTextPos, EntryColor, EntryLabelAnsi.Get());
        }
    }

    // Render transition lines + arrowheads
    DrawList->ChannelsSetCurrent(ChannelLines);

    // Pre-compute edge port assignments per node per exit side
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

    struct FPortEntry
    {
        int32 TransitionIndex = -1;
        float SortKey = 0.0f;
    };

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
        if (Trans.IsSubSmConnector)
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

        // Connector dashed line for sub-SM connectors
        if (Transition.IsSubSmConnector)
        {
            const auto& Src = InSmInfo.States[Transition.SourceStateIndex];
            const auto& Tgt = InSmInfo.States[Transition.TargetStateIndex];

            auto SrcW = ComputeNodeWidth(Src) * _CanvasZoom;
            auto SrcH = ComputeNodeHeight(Src) * _CanvasZoom;
            auto TgtW = ComputeNodeWidth(Tgt) * _CanvasZoom;
            auto TgtH = ComputeNodeHeight(Tgt) * _CanvasZoom;

            auto SrcCenter = ImVec2{
                CanvasOrigin.x + Src.NodePosition.x * _CanvasZoom + SrcW * 0.5f,
                CanvasOrigin.y + Src.NodePosition.y * _CanvasZoom + SrcH * 0.5f
            };
            auto TgtCenter = ImVec2{
                CanvasOrigin.x + Tgt.NodePosition.x * _CanvasZoom + TgtW * 0.5f,
                CanvasOrigin.y + Tgt.NodePosition.y * _CanvasZoom + TgtH * 0.5f
            };

            auto Dx = TgtCenter.x - SrcCenter.x;
            auto Dy = TgtCenter.y - SrcCenter.y;
            auto ConnectorLen = FMath::Sqrt(Dx * Dx + Dy * Dy);

            if (ConnectorLen > 0.001f)
            {
                auto DashLen = Layout::SubSmConnectorDash * _CanvasZoom;
                auto NumDashes = FMath::Max(1, static_cast<int32>(ConnectorLen / (DashLen * 2.0f)));

                for (auto Idx = 0; Idx < NumDashes; ++Idx)
                {
                    auto T0 = static_cast<float>(Idx * 2) * DashLen / ConnectorLen;
                    auto T1 = static_cast<float>(Idx * 2 + 1) * DashLen / ConnectorLen;
                    T0 = FMath::Clamp(T0, 0.0f, 1.0f);
                    T1 = FMath::Clamp(T1, 0.0f, 1.0f);

                    auto DashStart = ImVec2{
                        FMath::Lerp(SrcCenter.x, TgtCenter.x, T0),
                        FMath::Lerp(SrcCenter.y, TgtCenter.y, T0)
                    };
                    auto DashEnd = ImVec2{
                        FMath::Lerp(SrcCenter.x, TgtCenter.x, T1),
                        FMath::Lerp(SrcCenter.y, TgtCenter.y, T1)
                    };

                    DrawList->AddLine(DashStart, DashEnd, Colors::SubSmConnector, 1.5f * _CanvasZoom);
                }
            }

            continue;
        }

        // Determine if this sub-SM transition should be muted
        auto IsSubSmTransMuted = false;
        if (Transition.IsSubSmTransition)
        {
            auto ParentIdx = InSmInfo.States[Transition.SourceStateIndex].SubSmParentStateIndex;
            IsSubSmTransMuted = (ParentIdx >= 0
                && ParentIdx < InSmInfo.States.Num()
                && NOT InSmInfo.States[ParentIdx].IsCurrentState);
        }

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

        auto TransitionFlash = 0.0f;
        if (_TransitionFlashTimer > 0.0f
            && Transition.SourceStateIndex == _FlashTransitionSource
            && Transition.TargetStateIndex == _FlashTransitionTarget)
        {
            constexpr auto TransitionFlashDuration = 0.6f;
            auto LinearProgress = _TransitionFlashTimer / TransitionFlashDuration;
            TransitionFlash = LinearProgress * LinearProgress;
        }

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

        RenderTransitionLine(DrawList, Source, Target, Transition, CanvasOrigin, PerpOffset, IsDimmed, IsSubSmTransMuted, TransitionFlash, SrcPortOff, TgtPortOff);
    }

    // Render state nodes on the node channel (above lines, below arrows)
    DrawList->ChannelsSetCurrent(ChannelNodes);

    for (auto Index = 0; Index < InSmInfo.States.Num(); ++Index)
    {
        const auto& State = InSmInfo.States[Index];

        // Compound nodes are rendered as boxes above, not as state nodes
        if (State.IsCompoundNode)
        { continue; }

        // Determine if sub-SM node should be muted (parent state not active)
        auto IsSubSmMuted = false;
        if (State.IsSubSmNode)
        {
            auto ParentIdx = State.SubSmParentStateIndex;
            IsSubSmMuted = (ParentIdx >= 0
                && ParentIdx < InSmInfo.States.Num()
                && NOT InSmInfo.States[ParentIdx].IsCurrentState);
        }

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

        RenderStateNode(DrawList, State, CanvasOrigin, IsQueuedAndCurrent, IsDimmed, IsSubSmMuted, BorderFade, DwellFlash);
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

    // Merge all channels: lines (bottom) -> nodes -> arrows (top)
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

            if (NOT TargetState.IsCurrentState && NOT TargetState.IsSubSmNode && NOT TargetState.IsCompoundNode)
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

            if (NOT TargetState.IsCompoundNode)
            {
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
    if (InState.IsCompoundNode)
    { return InState.CompoundNodeWidth; }

    auto* CachedWidth = _CachedNodeWidths.Find(InState.StateName);

    if (CachedWidth)
    {
        return *CachedWidth;
    }

    auto NameAnsi = StringCast<ANSICHAR>(*InState.StateName);
    auto TextSize = ImGui::CalcTextSize(NameAnsi.Get());
    constexpr auto MinNodeWidth = 160.0f;
    auto LeftPad = Layout::AccentBarWidth + Layout::NodePadding + Layout::StateIconSize * 2.0f + Layout::StateIconGap;
    auto RightPad = Layout::NodePadding;
    auto NeededWidth = TextSize.x + LeftPad + RightPad;

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
    if (InState.IsCompoundNode)
    { return InState.CompoundNodeHeight; }

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
    auto MutableTransitions = InTransitions;

    auto Solver = FCkHfsmViewer_LayoutSolver{};
    Solver.Solve(InOutStates, MutableTransitions, InInitialStateIndex, *this);

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
    // Reverse iterate so internal sub-SM states (higher indices) are tested
    // before compound nodes, giving them visual priority
    for (auto Index = InSmInfo.States.Num() - 1; Index >= 0; --Index)
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
            const auto& DraggedState = InSmInfo.States[_DraggedNodeIndex];
            _CachedPositions.FindOrAdd(DraggedState.StateName) = NewPos;

            // When dragging a compound node (box), move all internal states by the same delta
            if (DraggedState.IsCompoundNode)
            {
                auto MoveDelta = ImVec2{
                    NewPos.x - DraggedState.NodePosition.x,
                    NewPos.y - DraggedState.NodePosition.y
                };

                for (const auto& [ParentName, Range] : _SubSmRanges)
                {
                    if (Range.CompoundNodeIndex != _DraggedNodeIndex)
                    { continue; }

                    for (auto Idx = Range.FirstInternalStateIndex;
                        Idx < Range.FirstInternalStateIndex + Range.InternalStateCount; ++Idx)
                    {
                        if (Idx >= 0 && Idx < InSmInfo.States.Num())
                        {
                            const auto& InternalState = InSmInfo.States[Idx];
                            auto InternalNewPos = ImVec2{
                                InternalState.NodePosition.x + MoveDelta.x,
                                InternalState.NodePosition.y + MoveDelta.y
                            };
                            _CachedPositions.FindOrAdd(InternalState.StateName) = InternalNewPos;
                        }
                    }

                    break;
                }
            }
        }
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
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
