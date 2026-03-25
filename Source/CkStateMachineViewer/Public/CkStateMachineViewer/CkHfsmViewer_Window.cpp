#include "CkHfsmViewer_Window.h"

#include "CkStateMachineViewer/CkHfsmViewer_Log.h"

#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkStateMachine/CkStateMachine_Debug_Fragment.h"

#include <Framework/Application/SlateApplication.h>
#include <Widgets/SWindow.h>

#include <ImGuiContext.h>
#include <ImGuiModule.h>

THIRD_PARTY_INCLUDES_START
#include <imgui.h>
THIRD_PARTY_INCLUDES_END

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_Window::
    Open()
    -> void
{
    if (_SlateWindow.IsValid())
    {
        _SlateWindow->BringToFront();
        return;
    }

    _SlateWindow = SNew(SWindow)
        .Title(FText::FromString(TEXT("HFSM Viewer")))
        .ClientSize(FVector2D{1280.0, 720.0})
        .SupportsMinimize(true)
        .SupportsMaximize(true);

    FSlateApplication::Get().AddWindow(_SlateWindow.ToSharedRef());

    _ImGuiContext = FImGuiModule::CreateWindowContext(_SlateWindow.ToSharedRef());

    CK_ENSURE_IF_NOT(_ImGuiContext.IsValid(),
        TEXT("Failed to create ImGui context for HFSM Viewer window"))
    {
        Close();
        return;
    }

    _SlateWindow->GetOnWindowClosedEvent().AddSP(
        this,
        &FCkHfsmViewer_Window::OnWindowClosed);

    _TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateSP(this, &FCkHfsmViewer_Window::Tick));

    _SelectedSmIndex = 0;

    ck::hfsmviewer::Log(TEXT("HFSM Viewer window opened"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_Window::
    Close()
    -> void
{
    if (_SlateWindow.IsValid())
    {
        _SlateWindow->RequestDestroyWindow();
    }

    Cleanup();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_Window::
    IsOpen() const
    -> bool
{
    return _SlateWindow.IsValid();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_Window::
    Tick(
        float InDeltaTime)
    -> bool
{
    if (NOT _ImGuiContext.IsValid())
    { return false; }

    _LastDeltaTime = InDeltaTime;
    _DataCollector.Collect(Get_World());

    RenderImGui();

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_Window::
    RenderImGui()
    -> void
{
    ImGui::FScopedContext ScopedContext{_ImGuiContext};

    if (NOT ScopedContext)
    { return; }

    const auto& AllSms = _DataCollector.Get_AllStateMachines();

    constexpr auto WindowFlags =
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::Begin("HFSM Viewer##Main", nullptr, WindowFlags);

    if (AllSms.IsEmpty())
    {
        ImGui::TextColored({0.6f, 0.6f, 0.6f, 1.0f}, "No state machines found in world.");
        ImGui::End();
        return;
    }

    _SelectedSmIndex = FMath::Clamp(_SelectedSmIndex, 0, AllSms.Num() - 1);

    // === Toolbar: SM selector + controls + Reset Layout + Expand Nodes ===
    RenderSmSelector();

    ImGui::Separator();

    // === Main area: Graph canvas + detail panel with draggable splitters ===
    auto SelectedSmData = AllSms[_SelectedSmIndex];

    // === Scrub mode: override graph state to show historical snapshot ===
    if (_ScrubState.ViewMode == ECkHfsmViewer_ViewMode::Scrub)
    {
        auto Snapshot = ComputeScrubSnapshot(SelectedSmData);

        if (Snapshot.ActiveStateIndex >= 0)
        {
            for (auto& State : SelectedSmData.States)
            {
                State.IsCurrentState = false;
                State.IsCurrentDwellLive = false;
            }

            SelectedSmData.States[Snapshot.ActiveStateIndex].IsCurrentState = true;
            SelectedSmData.States[Snapshot.ActiveStateIndex].DwellTimeSeconds = Snapshot.TimeInState;
            SelectedSmData.CurrentStateIndex = Snapshot.ActiveStateIndex;
        }

        _GraphRenderer.SetScrubTransitionHighlight(
            Snapshot.TakenTransitionSourceIdx,
            Snapshot.TakenTransitionTargetIdx);
    }
    else
    {
        _GraphRenderer.ClearScrubTransitionHighlight();
    }

    auto TotalAvailableWidth = ImGui::GetContentRegionAvail().x;
    auto TotalAvailableHeight = ImGui::GetContentRegionAvail().y;

    constexpr auto SplitterThickness = 4.0f;
    constexpr auto MinDetailWidth = 200.0f;
    constexpr auto MinGraphWidth = 300.0f;
    constexpr auto MinHistoryHeight = 60.0f;
    constexpr auto MinGraphHeight = 150.0f;

    _DetailPanelWidth = FMath::Clamp(_DetailPanelWidth, MinDetailWidth, TotalAvailableWidth - MinGraphWidth - SplitterThickness);
    _HistoryHeight = FMath::Clamp(_HistoryHeight, MinHistoryHeight, TotalAvailableHeight - MinGraphHeight - SplitterThickness);

    auto GraphHeight = TotalAvailableHeight - _HistoryHeight - SplitterThickness;
    auto GraphWidth = TotalAvailableWidth - _DetailPanelWidth - SplitterThickness;

    // --- Graph canvas ---
    ImGui::BeginChild("##GraphArea", {GraphWidth, GraphHeight}, false, WindowFlags);
    auto Command = _GraphRenderer.Render(SelectedSmData, _LastDeltaTime);
    ImGui::EndChild();

    auto HandleBreakpointCommand = [&](const FCkHfsmViewer_Command& InCommand)
    {
#if !UE_BUILD_SHIPPING
        auto SmHandle = static_cast<FCk_Handle>(AllSms[_SelectedSmIndex].Handle);
        auto& Breakpoints = SmHandle.AddOrGet<ck::FFragment_Sm_Breakpoints>();

        if (InCommand.Type == FCkHfsmViewer_Command::EType::ToggleStateEntryBreakpoint)
        {
            if (InCommand.StateIndex >= 0 && InCommand.StateIndex < SelectedSmData.States.Num())
            {
                auto StateClass = SelectedSmData.States[InCommand.StateIndex].StateClass;
                auto EntrySet = Breakpoints.Get_EntryBreakpoints();

                if (EntrySet.Contains(StateClass))
                { EntrySet.Remove(StateClass); }
                else
                { EntrySet.Add(StateClass); }

                Breakpoints.Set_EntryBreakpoints(MoveTemp(EntrySet));
            }
        }
        else if (InCommand.Type == FCkHfsmViewer_Command::EType::ToggleStateExitBreakpoint)
        {
            if (InCommand.StateIndex >= 0 && InCommand.StateIndex < SelectedSmData.States.Num())
            {
                auto StateClass = SelectedSmData.States[InCommand.StateIndex].StateClass;
                auto ExitSet = Breakpoints.Get_ExitBreakpoints();

                if (ExitSet.Contains(StateClass))
                { ExitSet.Remove(StateClass); }
                else
                { ExitSet.Add(StateClass); }

                Breakpoints.Set_ExitBreakpoints(MoveTemp(ExitSet));
            }
        }
        else if (InCommand.Type == FCkHfsmViewer_Command::EType::ToggleTransitionBreakpoint)
        {
            if (InCommand.TransitionIndex >= 0 && InCommand.TransitionIndex < SelectedSmData.Transitions.Num())
            {
                const auto& Trans = SelectedSmData.Transitions[InCommand.TransitionIndex];
                auto SourceClass = SelectedSmData.States[Trans.SourceStateIndex].StateClass;
                auto Key = ck::FFragment_Sm_Breakpoints::FTransitionKey{SourceClass, Trans.TargetStateClass};
                auto TransSet = Breakpoints.Get_TransitionBreakpoints();

                if (TransSet.Contains(Key))
                { TransSet.Remove(Key); }
                else
                { TransSet.Add(Key); }

                Breakpoints.Set_TransitionBreakpoints(MoveTemp(TransSet));
            }
        }
#endif
    };

    if (Command.Type == FCkHfsmViewer_Command::EType::ForceTransition)
    {
        auto SmHandle = AllSms[_SelectedSmIndex].Handle;
        UCk_Utils_StateMachine_UE::Request_Transition(SmHandle, Command.TargetStateClass);
    }
    else if (Command.Type != FCkHfsmViewer_Command::EType::None)
    {
        HandleBreakpointCommand(Command);
    }

    // --- Vertical splitter (between graph and detail panel) ---
    ImGui::SameLine();
    ImGui::Button("##VSplitter", {SplitterThickness, GraphHeight});
    if (ImGui::IsItemActive())
    {
        _DetailPanelWidth -= ImGui::GetIO().MouseDelta.x;
        _DetailPanelWidth = FMath::Clamp(_DetailPanelWidth, MinDetailWidth, TotalAvailableWidth - MinGraphWidth - SplitterThickness);
    }
    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }

    // --- Detail panel ---
    ImGui::SameLine();
    ImGui::BeginChild("##DetailArea", {_DetailPanelWidth, GraphHeight}, true);
    auto DetailCommand = _GraphRenderer.RenderDetailPanel(SelectedSmData);
    ImGui::EndChild();

    if (DetailCommand.Type != FCkHfsmViewer_Command::EType::None)
    {
        HandleBreakpointCommand(DetailCommand);
    }

    // --- Horizontal splitter (between graph area and timeline+history) ---
    ImGui::Button("##HSplitter", {-1, SplitterThickness});
    if (ImGui::IsItemActive())
    {
        _HistoryHeight -= ImGui::GetIO().MouseDelta.y;
        _HistoryHeight = FMath::Clamp(_HistoryHeight, MinHistoryHeight, TotalAvailableHeight - MinGraphHeight - SplitterThickness);
    }
    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    }

    // --- Timeline + History (bottom section) ---
    ImGui::BeginChild("##BottomArea", {0, _HistoryHeight}, false);

    RenderTimeline(AllSms[_SelectedSmIndex]);
    RenderHistory(AllSms[_SelectedSmIndex]);

    ImGui::EndChild();

    // --- F5 to resume from breakpoint ---
    if (AllSms[_SelectedSmIndex].IsPieDebugPaused && ImGui::IsKeyPressed(ImGuiKey_F5))
    {
        UCk_Utils_EditorOnly_UE::Request_DebugResumeExecution();
    }

    // --- Escape key returns to Live mode ---
    if (_ScrubState.ViewMode == ECkHfsmViewer_ViewMode::Scrub && ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        _ScrubState.ViewMode = ECkHfsmViewer_ViewMode::Live;
        _ScrubState.SelectedHistoryIndex = -1;
        _ScrubState.SelectedRunIndex = -1;
    }

    ImGui::End();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_Window::
    RenderSmSelector()
    -> void
{
    const auto& AllSms = _DataCollector.Get_AllStateMachines();

    if (AllSms.IsEmpty())
    { return; }

    if (AllSms.Num() > 1)
    {
        ImGui::Text("State Machine:");
        ImGui::SameLine();

        ImGui::SetNextItemWidth(200.0f);
        auto CurrentLabel = StringCast<ANSICHAR>(*AllSms[_SelectedSmIndex].DebugName);

        if (ImGui::BeginCombo("##SmCombo", CurrentLabel.Get()))
        {
            for (auto Index = 0; Index < AllSms.Num(); ++Index)
            {
                auto Label = StringCast<ANSICHAR>(*AllSms[Index].DebugName);
                auto IsSelected = (Index == _SelectedSmIndex);

                if (ImGui::Selectable(Label.Get(), IsSelected))
                {
                    if (_SelectedSmIndex != Index)
                    {
                        _SelectedSmIndex = Index;
                        _GraphRenderer.RequestRelayout();
                    }
                }

                if (IsSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }
    }
    else
    {
        auto NameLabel = StringCast<ANSICHAR>(*AllSms[_SelectedSmIndex].DebugName);
        ImGui::Text("%s", NameLabel.Get());
    }

    const auto& Sm = AllSms[_SelectedSmIndex];
    auto StatusText = static_cast<const ANSICHAR*>("Stopped");

    switch (Sm.RunStatus)
    {
    case ECk_SmRunStatus::Running: StatusText = "Running"; break;
    case ECk_SmRunStatus::Paused:  StatusText = "Paused";  break;
    default: break;
    }

    ImGui::SameLine();
    ImGui::TextColored(
        Sm.RunStatus == ECk_SmRunStatus::Running ? ImVec4{0.3f, 0.69f, 0.31f, 1.0f} : ImVec4{0.6f, 0.6f, 0.6f, 1.0f},
        "[%s]",
        StatusText);

    auto SmHandle = Sm.Handle;

    auto IsStopped = (Sm.RunStatus == ECk_SmRunStatus::Stopped);
    auto IsRunning = (Sm.RunStatus == ECk_SmRunStatus::Running);
    auto IsPaused  = (Sm.RunStatus == ECk_SmRunStatus::Paused);

    ImGui::SameLine();
    ImGui::BeginDisabled(NOT IsStopped);
    if (ImGui::SmallButton("Start")) { UCk_Utils_StateMachine_UE::Request_Start(SmHandle); }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(IsStopped);
    if (ImGui::SmallButton("Stop")) { UCk_Utils_StateMachine_UE::Request_Stop(SmHandle); }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(NOT IsRunning);
    if (ImGui::SmallButton("Pause")) { UCk_Utils_StateMachine_UE::Request_Pause(SmHandle); }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(NOT IsPaused);
    if (ImGui::SmallButton("Resume")) { UCk_Utils_StateMachine_UE::Request_Resume(SmHandle); }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::TextColored({0.3f, 0.3f, 0.3f, 1.0f}, "|");
    ImGui::SameLine();
    if (ImGui::SmallButton("Reset Layout")) { _GraphRenderer.RequestRelayout(); }

    ImGui::SameLine();
    ImGui::TextColored({0.3f, 0.3f, 0.3f, 1.0f}, "|");
    ImGui::SameLine();
    _GraphRenderer.RenderToolbar();

    // --- Breakpoint hit indicator + Resume button ---
    if (Sm.IsPieDebugPaused && Sm.HasBreakpointHit)
    {
        ImGui::SameLine();
        ImGui::TextColored({0.3f, 0.3f, 0.3f, 1.0f}, "|");
        ImGui::SameLine();
        ImGui::TextColored({1.0f, 0.2f, 0.2f, 1.0f}, "[BREAKPOINT]");
        ImGui::SameLine();

        auto DescAnsi = StringCast<ANSICHAR>(*Sm.BreakpointHitDescription);
        ImGui::TextColored({1.0f, 0.6f, 0.6f, 1.0f}, "%s", DescAnsi.Get());
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.5f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.69f, 0.31f, 1.0f});
        if (ImGui::SmallButton("Resume (F5)"))
        {
            UCk_Utils_EditorOnly_UE::Request_DebugResumeExecution();
        }
        ImGui::PopStyleColor(2);
    }
    else if (Sm.IsPieDebugPaused)
    {
        ImGui::SameLine();
        ImGui::TextColored({0.3f, 0.3f, 0.3f, 1.0f}, "|");
        ImGui::SameLine();
        ImGui::TextColored({1.0f, 0.6f, 0.2f, 1.0f}, "[PIE PAUSED]");
    }

    // --- Scrub mode indicator + Go Live button ---
    if (_ScrubState.ViewMode == ECkHfsmViewer_ViewMode::Scrub)
    {
        ImGui::SameLine();
        ImGui::TextColored({0.3f, 0.3f, 0.3f, 1.0f}, "|");
        ImGui::SameLine();
        ImGui::TextColored({1.0f, 0.6f, 0.2f, 1.0f}, "[SCRUB]");
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.5f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.69f, 0.31f, 1.0f});
        if (ImGui::SmallButton("Go Live"))
        {
            _ScrubState.ViewMode = ECkHfsmViewer_ViewMode::Live;
            _ScrubState.SelectedHistoryIndex = -1;
            _ScrubState.SelectedRunIndex = -1;
        }
        ImGui::PopStyleColor(2);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_Window::
    RenderTimeline(
        const FCkHfsmViewer_SmInfo& InSmInfo)
    -> void
{
    // Timeline rendering will be implemented in 6CD
    // For now, just reserve space for the timeline area
    const auto& ActiveRun = GetActiveRun(InSmInfo);

    if (ActiveRun.Segments.IsEmpty() && ActiveRun.History.IsEmpty())
    { return; }

    constexpr auto TimelineRowHeight = 24.0f;
    constexpr auto FrameLaneHeight = 14.0f;
    constexpr auto TotalRowHeight = TimelineRowHeight + FrameLaneHeight;
    constexpr auto TimeAxisHeight = 16.0f;
    constexpr auto LabelWidth = 60.0f;

    auto NumRuns = 1 + InSmInfo.CompletedRuns.Num();
    constexpr auto MaxVisibleRows = 6;
    auto VisibleRows = FMath::Min(NumRuns, MaxVisibleRows);
    auto TimelineHeight = TimeAxisHeight + TotalRowHeight * VisibleRows;

    ImGui::BeginChild("##TimelineArea", {0, TimelineHeight}, false);

    auto DrawList = ImGui::GetWindowDrawList();
    auto WindowPos = ImGui::GetCursorScreenPos();
    auto AvailWidth = ImGui::GetContentRegionAvail().x;

    auto BarLeft = WindowPos.x + LabelWidth;
    auto BarWidth = AvailWidth - LabelWidth;

    if (BarWidth < 10.0f)
    {
        ImGui::EndChild();
        return;
    }

    constexpr auto MinViewDuration = 0.1;
    auto ViewDuration = FMath::Max(_ScrubState.TimelineViewDuration, MinViewDuration);
    auto ViewStart = static_cast<double>(_ScrubState.TimelineScrollX);
    auto ViewEnd = ViewStart + ViewDuration;

    // --- Handle constants ---
    constexpr auto HandleHalfWidth = 8.0f;
    constexpr auto HandleHeight = 12.0f;
    constexpr auto HandleTopNarrow = 4.0f;
    constexpr auto HandleTopY = 2.0f;

    // --- Compute playhead position early (needed for handle input before rendering) ---
    auto PlayheadTime = 0.0;

    if (_ScrubState.ViewMode == ECkHfsmViewer_ViewMode::Live)
    {
        PlayheadTime = InSmInfo.CurrentRun.Duration;
    }
    else
    {
        PlayheadTime = _ScrubState.ScrubTime;
    }

    auto PlayheadVisible = (PlayheadTime >= ViewStart && PlayheadTime <= ViewEnd);
    auto PlayheadX = PlayheadVisible
        ? BarLeft + static_cast<float>((PlayheadTime - ViewStart) / ViewDuration * BarWidth)
        : 0.0f;

    // --- Handle input (BEFORE row rendering so it gets click priority) ---
    {
        auto MousePos = ImGui::GetMousePos();

        if (_IsDraggingPlayhead)
        {
            auto DraggedTime = ViewStart + (MousePos.x - BarLeft) / BarWidth * ViewDuration;
            const auto& ActiveRunForDrag = GetActiveRun(InSmInfo);
            DraggedTime = FMath::Clamp(DraggedTime, 0.0, ActiveRunForDrag.Duration);
            _ScrubState.ScrubTime = DraggedTime;
            _ScrubState.ViewMode = ECkHfsmViewer_ViewMode::Scrub;
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

            if (NOT ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                _IsDraggingPlayhead = false;
            }
        }
        else if (PlayheadVisible)
        {
            auto HandleTop = WindowPos.y + HandleTopY;
            auto HandleBottom = HandleTop + HandleHeight;

            auto IsOverHandle = MousePos.x >= PlayheadX - HandleHalfWidth
                && MousePos.x <= PlayheadX + HandleHalfWidth
                && MousePos.y >= HandleTop
                && MousePos.y <= HandleBottom;

            if (IsOverHandle)
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    _IsDraggingPlayhead = true;
                }
            }
        }
    }

    // --- Time axis ---
    {
        auto AxisY = WindowPos.y;

        auto TickInterval = 1.0;
        if (ViewDuration < 2.0) { TickInterval = 0.1; }
        else if (ViewDuration < 10.0) { TickInterval = 0.5; }
        else if (ViewDuration < 30.0) { TickInterval = 1.0; }
        else if (ViewDuration < 120.0) { TickInterval = 5.0; }
        else { TickInterval = 10.0; }

        auto FirstTick = FMath::CeilToDouble(ViewStart / TickInterval) * TickInterval;

        for (auto T = FirstTick; T <= ViewEnd; T += TickInterval)
        {
            auto X = BarLeft + static_cast<float>((T - ViewStart) / ViewDuration * BarWidth);
            DrawList->AddLine({X, AxisY}, {X, AxisY + TimeAxisHeight}, IM_COL32(80, 80, 80, 255));

            auto TickLabel = FString::Printf(TEXT("%.1fs"), T);
            auto TickAnsi = StringCast<ANSICHAR>(*TickLabel);
            DrawList->AddText({X + 2.0f, AxisY}, IM_COL32(120, 120, 120, 255), TickAnsi.Get());
        }
    }

    // Helper lambda: render one run row
    auto RenderRunRow = [&](const FCkHfsmViewer_RunInfo& InRun, int32 InRunIndex, float InRowY, bool InIsCurrentRun)
    {
        auto IsSelectedRun = (_ScrubState.SelectedRunIndex == InRunIndex)
            || (InIsCurrentRun && _ScrubState.SelectedRunIndex == -1);

        // Row background
        auto RowRect = ImVec2{WindowPos.x, InRowY};
        auto RowEnd = ImVec2{WindowPos.x + AvailWidth, InRowY + TimelineRowHeight};
        auto BgColor = IsSelectedRun ? IM_COL32(40, 40, 60, 255) : IM_COL32(25, 25, 35, 255);
        DrawList->AddRectFilled(RowRect, RowEnd, BgColor);

        // Label
        auto LabelText = InIsCurrentRun ? FString{TEXT("Current")} : FString::Printf(TEXT("Run #%d"), InRun.RunIndex);
        auto LabelAnsi = StringCast<ANSICHAR>(*LabelText);
        auto LabelColor = InIsCurrentRun ? IM_COL32(0x4C, 0xAF, 0x50, 255) : IM_COL32(160, 160, 160, 255);
        DrawList->AddText({WindowPos.x + 4.0f, InRowY + 4.0f}, LabelColor, LabelAnsi.Get());

        // Segments
        for (const auto& Segment : InRun.Segments)
        {
            auto SegStart = Segment.StartTime;
            auto SegEnd = Segment.EndTime > 0.0 ? Segment.EndTime : InRun.Duration;

            if (SegEnd <= ViewStart || SegStart >= ViewEnd)
            { continue; }

            auto ClampedStart = FMath::Max(SegStart, ViewStart);
            auto ClampedEnd = FMath::Min(SegEnd, ViewEnd);

            auto X0 = BarLeft + static_cast<float>((ClampedStart - ViewStart) / ViewDuration * BarWidth);
            auto X1 = BarLeft + static_cast<float>((ClampedEnd - ViewStart) / ViewDuration * BarWidth);

            constexpr auto MinSegmentPixels = 2.0f;
            X1 = FMath::Max(X1, X0 + MinSegmentPixels);

            constexpr auto SegmentPadding = 2.0f;
            DrawList->AddRectFilled(
                {X0, InRowY + SegmentPadding},
                {X1, InRowY + TimelineRowHeight - SegmentPadding},
                Segment.Color);

            auto SegAnsi = StringCast<ANSICHAR>(*Segment.StateName);
            DrawList->PushClipRect({X0, InRowY}, {X1, InRowY + TimelineRowHeight}, true);
            DrawList->AddText({X0 + 3.0f, InRowY + 4.0f}, IM_COL32(255, 255, 255, 200), SegAnsi.Get());
            DrawList->PopClipRect();
        }

        // Busy frame icons
        for (const auto& BusyFrame : InRun.BusyFrames)
        {
            if (BusyFrame.Time < ViewStart || BusyFrame.Time > ViewEnd)
            { continue; }

            auto X = BarLeft + static_cast<float>((BusyFrame.Time - ViewStart) / ViewDuration * BarWidth);
            auto CenterY = InRowY + TimelineRowHeight * 0.5f;
            constexpr auto DiamondSize = 4.0f;
            DrawList->AddQuadFilled(
                {X, CenterY - DiamondSize},
                {X + DiamondSize, CenterY},
                {X, CenterY + DiamondSize},
                {X - DiamondSize, CenterY},
                IM_COL32(255, 200, 50, 255));
        }

        // Transition boundary ticks
        for (const auto& HistEntry : InRun.History)
        {
            auto TransTime = HistEntry.RealTimeSeconds - InRun.StartTime;

            if (TransTime < ViewStart || TransTime > ViewEnd)
            { continue; }

            auto X = BarLeft + static_cast<float>((TransTime - ViewStart) / ViewDuration * BarWidth);

            DrawList->AddLine(
                {X, InRowY + 1.0f},
                {X, InRowY + TimelineRowHeight - 1.0f},
                IM_COL32(200, 200, 200, 60), 1.0f);
        }

        // Pause/breakpoint tear markers — zigzag lines
        for (const auto& Marker : InRun.PauseMarkers)
        {
            if (Marker.Time < ViewStart || Marker.Time > ViewEnd)
            { continue; }

            auto X = BarLeft + static_cast<float>((Marker.Time - ViewStart) / ViewDuration * BarWidth);
            auto TearColor = Marker.IsBreakpoint
                ? IM_COL32(0xEF, 0x33, 0x30, 220)
                : IM_COL32(0xFF, 0xA0, 0x30, 180);

            constexpr auto ZagWidth = 3.0f;
            constexpr auto ZagStep = 4.0f;

            for (auto Y = InRowY + 2.0f; Y < InRowY + TimelineRowHeight - 2.0f; Y += ZagStep)
            {
                auto Y1 = FMath::Min(Y + ZagStep, InRowY + TimelineRowHeight - 2.0f);
                auto XMid = X + ZagWidth * ((static_cast<int32>((Y - InRowY) / ZagStep) % 2 == 0) ? 1.0f : -1.0f);
                DrawList->AddLine({X, Y}, {XMid, (Y + Y1) * 0.5f}, TearColor, 2.0f);
                DrawList->AddLine({XMid, (Y + Y1) * 0.5f}, {X, Y1}, TearColor, 2.0f);
            }
        }

        // Frame lane — thin row below the state row
        {
            auto FrameLaneY = InRowY + TimelineRowHeight;
            auto FrameLaneBg = IM_COL32(20, 20, 30, 255);
            DrawList->AddRectFilled(
                {BarLeft, FrameLaneY},
                {BarLeft + BarWidth, FrameLaneY + FrameLaneHeight},
                FrameLaneBg);

            for (auto SegIdx = 0; SegIdx < InRun.FrameSegments.Num(); ++SegIdx)
            {
                const auto& FrameSeg = InRun.FrameSegments[SegIdx];

                auto SegStart = FrameSeg.StartTime;
                auto SegEnd = FrameSeg.EndTime;

                if (SegEnd <= ViewStart || SegStart >= ViewEnd)
                { continue; }

                auto ClampedStart = FMath::Max(SegStart, ViewStart);
                auto ClampedEnd = FMath::Min(SegEnd, ViewEnd);

                auto X0 = BarLeft + static_cast<float>((ClampedStart - ViewStart) / ViewDuration * BarWidth);
                auto X1 = BarLeft + static_cast<float>((ClampedEnd - ViewStart) / ViewDuration * BarWidth);

                auto AlternateBg = (SegIdx % 2 == 0)
                    ? IM_COL32(30, 30, 45, 255)
                    : IM_COL32(25, 25, 38, 255);

                DrawList->AddRectFilled(
                    {X0, FrameLaneY},
                    {X1, FrameLaneY + FrameLaneHeight},
                    AlternateBg);

                // Separator line at segment start
                DrawList->AddLine(
                    {X0, FrameLaneY},
                    {X0, FrameLaneY + FrameLaneHeight},
                    IM_COL32(60, 60, 80, 180), 1.0f);

                // Interpolated frame cell division lines
                auto FullX0 = BarLeft + static_cast<float>((SegStart - ViewStart) / ViewDuration * BarWidth);
                auto FullX1 = BarLeft + static_cast<float>((SegEnd - ViewStart) / ViewDuration * BarWidth);

                if (FrameSeg.EndFrame > FrameSeg.StartFrame)
                {
                    auto FrameCount = static_cast<float>(FrameSeg.EndFrame - FrameSeg.StartFrame);
                    constexpr auto MinCellPixels = 3.0f;
                    auto CellWidth = (FullX1 - FullX0) / FrameCount;

                    if (CellWidth >= MinCellPixels)
                    {
                        for (auto F = FrameSeg.StartFrame + 1; F < FrameSeg.EndFrame; ++F)
                        {
                            auto Fraction = static_cast<float>(F - FrameSeg.StartFrame) / FrameCount;
                            auto DivX = FullX0 + Fraction * (FullX1 - FullX0);

                            if (DivX < X0 || DivX > X1)
                            { continue; }

                            DrawList->AddLine(
                                {DivX, FrameLaneY + 1.0f},
                                {DivX, FrameLaneY + FrameLaneHeight - 1.0f},
                                IM_COL32(50, 50, 70, 120), 1.0f);
                        }
                    }
                }

                // Frame label
                auto FrameLabel = FString::Printf(TEXT("F%llu"), FrameSeg.StartFrame);
                auto FrameAnsi = StringCast<ANSICHAR>(*FrameLabel);
                DrawList->PushClipRect({X0, FrameLaneY}, {X1, FrameLaneY + FrameLaneHeight}, true);
                DrawList->AddText(
                    {X0 + 2.0f, FrameLaneY + 1.0f},
                    IM_COL32(140, 140, 160, 200),
                    FrameAnsi.Get());
                DrawList->PopClipRect();
            }
        }

        // Row click detection (guarded — handle drag takes priority)
        auto MousePos = ImGui::GetMousePos();
        auto FullRowEnd = ImVec2{WindowPos.x + AvailWidth, InRowY + TotalRowHeight};
        auto IsHovered = MousePos.x >= RowRect.x && MousePos.x < FullRowEnd.x
            && MousePos.y >= RowRect.y && MousePos.y < FullRowEnd.y;

        if (IsHovered && NOT _IsDraggingPlayhead && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            _ScrubState.SelectedRunIndex = InIsCurrentRun ? -1 : InRunIndex;

            auto ClickedTime = ViewStart + (MousePos.x - BarLeft) / BarWidth * ViewDuration;
            ClickedTime = FMath::Clamp(ClickedTime, 0.0, InRun.Duration);
            _ScrubState.ScrubTime = ClickedTime;
            _ScrubState.ViewMode = ECkHfsmViewer_ViewMode::Scrub;
            _ScrubState.SelectedHistoryIndex = -1;
        }

        if (IsHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && InIsCurrentRun)
        {
            _ScrubState.ViewMode = ECkHfsmViewer_ViewMode::Live;
            _ScrubState.SelectedHistoryIndex = -1;
            _ScrubState.SelectedRunIndex = -1;
        }

        // Selected row border
        if (IsSelectedRun)
        {
            DrawList->AddRect(RowRect, RowEnd, IM_COL32(100, 150, 255, 180));
        }
    };

    // Render rows: current run on top, completed runs below (newest first)
    auto RowY = WindowPos.y + TimeAxisHeight;

    RenderRunRow(InSmInfo.CurrentRun, -1, RowY, true);
    RowY += TotalRowHeight;

    for (auto RunIdx = InSmInfo.CompletedRuns.Num() - 1; RunIdx >= 0; --RunIdx)
    {
        if (RowY + TotalRowHeight > WindowPos.y + TimelineHeight)
        { break; }

        RenderRunRow(InSmInfo.CompletedRuns[RunIdx], RunIdx, RowY, false);
        RowY += TotalRowHeight;
    }

    // --- Playhead visual (rendered after rows so it draws on top) ---
    if (PlayheadVisible)
    {
        // Recompute X from current ScrubTime (may have changed via handle drag)
        auto CurrentPlayheadTime = (_ScrubState.ViewMode == ECkHfsmViewer_ViewMode::Live)
            ? InSmInfo.CurrentRun.Duration
            : _ScrubState.ScrubTime;
        auto FinalX = BarLeft + static_cast<float>((CurrentPlayheadTime - ViewStart) / ViewDuration * BarWidth);

        auto HandleTop = WindowPos.y + HandleTopY;
        auto LineTop = HandleTop + HandleHeight;
        auto LineBottom = WindowPos.y + TimelineHeight;

        auto HandleColor = _IsDraggingPlayhead
            ? IM_COL32(0x40, 0xE0, 0xD0, 255)
            : (_ScrubState.ViewMode == ECkHfsmViewer_ViewMode::Live
                ? IM_COL32(0x00, 0xBC, 0xD4, 240)
                : IM_COL32(0xFF, 0x98, 0x00, 240));

        auto LineColor = _ScrubState.ViewMode == ECkHfsmViewer_ViewMode::Live
            ? IM_COL32(0x00, 0xBC, 0xD4, 160)
            : IM_COL32(0xFF, 0x98, 0x00, 160);

        DrawList->AddLine({FinalX, LineTop}, {FinalX, LineBottom}, LineColor, 2.0f);

        // Handle — trapezoid (narrow top, wide bottom)
        ImVec2 HandlePoints[4] = {
            {FinalX - HandleTopNarrow, HandleTop},
            {FinalX + HandleTopNarrow, HandleTop},
            {FinalX + HandleHalfWidth, HandleTop + HandleHeight},
            {FinalX - HandleHalfWidth, HandleTop + HandleHeight}
        };
        DrawList->AddConvexPolyFilled(HandlePoints, 4, HandleColor);

        auto OutlineColor = _IsDraggingPlayhead
            ? IM_COL32(255, 255, 255, 255)
            : IM_COL32(0, 0, 0, 180);
        constexpr auto HandlePointCount = 4;
        for (auto PointIdx = 0; PointIdx < HandlePointCount; ++PointIdx)
        {
            auto NextIdx = (PointIdx + 1) % HandlePointCount;
            DrawList->AddLine(HandlePoints[PointIdx], HandlePoints[NextIdx], OutlineColor, 1.0f);
        }
    }

    // --- Zoom interaction (scroll wheel over timeline) ---
    {
        auto MousePos = ImGui::GetMousePos();
        auto IsOverTimeline = MousePos.x >= WindowPos.x && MousePos.x < WindowPos.x + AvailWidth
            && MousePos.y >= WindowPos.y && MousePos.y < WindowPos.y + TimelineHeight;

        if (IsOverTimeline && NOT _IsDraggingPlayhead)
        {
            auto Scroll = ImGui::GetIO().MouseWheel;

            if (FMath::Abs(Scroll) > 0.0f)
            {
                auto TimeUnderMouse = ViewStart + (MousePos.x - BarLeft) / BarWidth * ViewDuration;
                constexpr auto ZoomFactor = 1.15;

                if (Scroll > 0.0f)
                {
                    _ScrubState.TimelineViewDuration /= ZoomFactor;
                }
                else
                {
                    _ScrubState.TimelineViewDuration *= ZoomFactor;
                }

                constexpr auto MaxViewDuration = 600.0;
                _ScrubState.TimelineViewDuration = FMath::Clamp(_ScrubState.TimelineViewDuration, MinViewDuration, MaxViewDuration);

                auto NewViewDuration = _ScrubState.TimelineViewDuration;
                auto NewScrollX = static_cast<float>(TimeUnderMouse - (MousePos.x - BarLeft) / BarWidth * NewViewDuration);
                _ScrubState.TimelineScrollX = FMath::Max(NewScrollX, 0.0f);
            }

            // Middle-drag to pan
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
            {
                auto DeltaX = ImGui::GetIO().MouseDelta.x;
                auto TimeDelta = static_cast<float>(DeltaX / BarWidth * ViewDuration);
                _ScrubState.TimelineScrollX -= TimeDelta;
                _ScrubState.TimelineScrollX = FMath::Max(_ScrubState.TimelineScrollX, 0.0f);
            }

            // Left-drag for scrubbing (when not dragging handle)
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                if (_ScrubState.ViewMode == ECkHfsmViewer_ViewMode::Scrub)
                {
                    auto DraggedTime = ViewStart + (MousePos.x - BarLeft) / BarWidth * ViewDuration;

                    const auto& ActiveRun2 = GetActiveRun(InSmInfo);
                    DraggedTime = FMath::Clamp(DraggedTime, 0.0, ActiveRun2.Duration);
                    _ScrubState.ScrubTime = DraggedTime;
                }
            }
        }
    }

    // --- Live mode auto-scroll (smooth) ---
    if (_ScrubState.ViewMode == ECkHfsmViewer_ViewMode::Live)
    {
        auto LiveTime = InSmInfo.CurrentRun.Duration;
        constexpr auto LookAheadFraction = 0.8;
        auto RightEdgeTime = ViewStart + ViewDuration * LookAheadFraction;

        if (LiveTime > RightEdgeTime)
        {
            auto TargetScrollX = static_cast<float>(LiveTime - ViewDuration * LookAheadFraction);
            TargetScrollX = FMath::Max(TargetScrollX, 0.0f);

            constexpr auto ScrollSpeed = 8.0f;
            _ScrubState.TimelineScrollX = FMath::FInterpTo(
                _ScrubState.TimelineScrollX,
                TargetScrollX,
                _LastDeltaTime,
                ScrollSpeed);
        }
    }

    ImGui::Dummy({0, TimelineHeight});
    ImGui::EndChild();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_Window::
    RenderRunSelector(
        const FCkHfsmViewer_SmInfo& InSmInfo)
    -> void
{
    // Run selector is integrated into the timeline rows via click interaction
    // This method is reserved for future toolbar-based run selector if needed
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_Window::
    RenderHistory(
        const FCkHfsmViewer_SmInfo& InSmInfo)
    -> void
{
    auto ColorU32ToVec4 = [](ImU32 InColor) -> ImVec4
    {
        return {
            ((InColor >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f,
            ((InColor >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f,
            ((InColor >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f,
            ((InColor >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f
        };
    };

    const auto& HistoryToDisplay = GetActiveRunHistory(InSmInfo);

    if (HistoryToDisplay.IsEmpty())
    {
        ImGui::TextColored({0.6f, 0.6f, 0.6f, 1.0f}, "No transition history yet.");
        return;
    }

    auto RunLabel = _ScrubState.SelectedRunIndex >= 0
        ? FString::Printf(TEXT("History - Run #%d (%d)"),
            InSmInfo.CompletedRuns[_ScrubState.SelectedRunIndex].RunIndex,
            HistoryToDisplay.Num())
        : FString::Printf(TEXT("History (%d)"), HistoryToDisplay.Num());

    auto RunLabelAnsi = StringCast<ANSICHAR>(*RunLabel);
    ImGui::TextColored({0.56f, 0.79f, 0.98f, 1.0f}, "%s", RunLabelAnsi.Get());

    constexpr auto MaxDisplayedEntries = 50;
    auto StartIndex = FMath::Max(0, HistoryToDisplay.Num() - MaxDisplayedEntries);

    auto FirstEntryTime = HistoryToDisplay[0].RealTimeSeconds;

    auto RemainingHeight = ImGui::GetContentRegionAvail().y;

    constexpr auto ColumnCount = 5;
    if (ImGui::BeginTable("##HistoryTable", ColumnCount,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
        {0, RemainingHeight}))
    {
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Frame", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("From", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("To", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Transition", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (auto Index = HistoryToDisplay.Num() - 1; Index >= StartIndex; --Index)
        {
            const auto& Entry = HistoryToDisplay[Index];

            ImGui::TableNextRow();

            // Clickable row via Selectable spanning all columns
            ImGui::TableSetColumnIndex(0);
            auto IsSelected = (_ScrubState.ViewMode == ECkHfsmViewer_ViewMode::Scrub
                && _ScrubState.SelectedHistoryIndex == Index);

            auto SelectableId = FString::Printf(TEXT("##histrow_%d"), Index);
            auto SelectableIdAnsi = StringCast<ANSICHAR>(*SelectableId);

            if (ImGui::Selectable(SelectableIdAnsi.Get(), IsSelected,
                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap))
            {
                _ScrubState.ViewMode = ECkHfsmViewer_ViewMode::Scrub;
                _ScrubState.SelectedHistoryIndex = Index;
                _ScrubState.ScrubTime = Entry.RealTimeSeconds - FirstEntryTime;
            }

            // Highlight selected row
            if (IsSelected)
            {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, IM_COL32(60, 80, 120, 180));
            }

            ImGui::SameLine();

            auto ElapsedSeconds = Entry.RealTimeSeconds - FirstEntryTime;
            ImGui::Text("%.2fs", ElapsedSeconds);

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%llu", Entry.FrameNumber);

            ImGui::TableSetColumnIndex(2);
            auto FromLabel = StringCast<ANSICHAR>(*Entry.FromStateName);
            auto FromColor = ColorU32ToVec4(ComputeStateColor(Entry.FromStateName));
            ImGui::TextColored(FromColor, "%s", FromLabel.Get());

            ImGui::TableSetColumnIndex(3);
            auto ToLabel = StringCast<ANSICHAR>(*Entry.ToStateName);
            auto ToColor = ColorU32ToVec4(ComputeStateColor(Entry.ToStateName));
            ImGui::TextColored(ToColor, "%s", ToLabel.Get());

            ImGui::TableSetColumnIndex(4);
            if (Entry.TransitionOrder >= 0)
            {
                constexpr auto OrderColor = ImVec4{0.56f, 0.79f, 0.98f, 1.0f};
                constexpr auto CheckColor = ImVec4{0.3f, 0.69f, 0.31f, 1.0f};

                auto OrderLabel = FString::Printf(TEXT("#%d: "), Entry.TransitionOrder);
                auto OrderAnsi = StringCast<ANSICHAR>(*OrderLabel);
                ImGui::TextColored(OrderColor, "%s", OrderAnsi.Get());

                for (auto CondIdx = 0; CondIdx < Entry.ConditionNames.Num(); ++CondIdx)
                {
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::TextColored(CheckColor, "[+]");
                    ImGui::SameLine(0.0f, 2.0f);
                    auto CondAnsi = StringCast<ANSICHAR>(*Entry.ConditionNames[CondIdx]);
                    ImGui::TextColored(CheckColor, "%s", CondAnsi.Get());

                    if (CondIdx < Entry.ConditionNames.Num() - 1)
                    {
                        ImGui::SameLine(0.0f, 4.0f);
                    }
                }
            }
            else
            {
                ImGui::TextColored({0.6f, 0.6f, 0.6f, 1.0f}, "Forced");
            }
        }

        ImGui::EndTable();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_Window::
    ComputeScrubSnapshot(
        const FCkHfsmViewer_SmInfo& InSmInfo)
    -> FCkHfsmViewer_ScrubSnapshot
{
    auto Snapshot = FCkHfsmViewer_ScrubSnapshot{};

    const auto& History = GetActiveRunHistory(InSmInfo);

    if (History.IsEmpty())
    { return Snapshot; }

    auto RunStartTime = History[0].RealTimeSeconds;

    // Walk history forward to find which state was active at ScrubTime
    // Before the first transition, the initial state (History[0].ToState) was active
    auto ActiveStateName = History[0].FromStateName;
    auto EnteredAt = 0.0;
    auto LastHistoryIndex = -1;
    auto LastTransitionSourceName = FString{};
    auto LastTransitionTargetName = FString{};

    // The "From" state of the first entry was the initial state
    // It was active from time 0 until the first transition
    if (_ScrubState.ScrubTime < History[0].RealTimeSeconds - RunStartTime)
    {
        // Before the first transition: we're in the initial state (FromState of first entry)
        // But we don't have a "ToState" entry for entering that state
        // The initial state is the ToState of an implicit "start" event
        // In practice, History[0].FromStateName is what was active before the first recorded transition
        ActiveStateName = History[0].FromStateName;
        Snapshot.TimeInState = _ScrubState.ScrubTime;
    }
    else
    {
        // Walk through history to find which transition happened just before ScrubTime
        for (auto Index = 0; Index < History.Num(); ++Index)
        {
            auto TransitionTime = History[Index].RealTimeSeconds - RunStartTime;

            if (TransitionTime > _ScrubState.ScrubTime)
            {
                break;
            }

            ActiveStateName = History[Index].ToStateName;
            EnteredAt = TransitionTime;
            LastHistoryIndex = Index;
            LastTransitionSourceName = History[Index].FromStateName;
            LastTransitionTargetName = History[Index].ToStateName;
        }

        Snapshot.TimeInState = _ScrubState.ScrubTime - EnteredAt;
        Snapshot.HistoryIndex = LastHistoryIndex;
    }

    // Find the state index by name
    for (auto Index = 0; Index < InSmInfo.States.Num(); ++Index)
    {
        if (InSmInfo.States[Index].StateName == ActiveStateName)
        {
            Snapshot.ActiveStateIndex = Index;
            break;
        }
    }

    Snapshot.ActiveStateName = ActiveStateName;

    // Find transition indices for the taken transition highlight
    if (NOT LastTransitionSourceName.IsEmpty())
    {
        auto SourceIdx = -1;
        auto TargetIdx = -1;

        for (auto Index = 0; Index < InSmInfo.States.Num(); ++Index)
        {
            if (InSmInfo.States[Index].StateName == LastTransitionSourceName)
            {
                SourceIdx = Index;
            }
            if (InSmInfo.States[Index].StateName == LastTransitionTargetName)
            {
                TargetIdx = Index;
            }
        }

        Snapshot.TakenTransitionSourceIdx = SourceIdx;
        Snapshot.TakenTransitionTargetIdx = TargetIdx;
    }

    return Snapshot;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_Window::
    GetActiveRunHistory(
        const FCkHfsmViewer_SmInfo& InSmInfo) const
    -> const TArray<FCkHfsmViewer_HistoryEntry>&
{
    if (_ScrubState.SelectedRunIndex >= 0
        && _ScrubState.SelectedRunIndex < InSmInfo.CompletedRuns.Num())
    {
        return InSmInfo.CompletedRuns[_ScrubState.SelectedRunIndex].History;
    }

    return InSmInfo.CurrentRun.History;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_Window::
    GetActiveRun(
        const FCkHfsmViewer_SmInfo& InSmInfo) const
    -> const FCkHfsmViewer_RunInfo&
{
    if (_ScrubState.SelectedRunIndex >= 0
        && _ScrubState.SelectedRunIndex < InSmInfo.CompletedRuns.Num())
    {
        return InSmInfo.CompletedRuns[_ScrubState.SelectedRunIndex];
    }

    return InSmInfo.CurrentRun;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_Window::
    OnWindowClosed(
        const TSharedRef<SWindow>& InWindow)
    -> void
{
    ck::hfsmviewer::Log(TEXT("HFSM Viewer window closed by user"));
    Cleanup();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_Window::
    Cleanup()
    -> void
{
    if (_TickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(_TickerHandle);
        _TickerHandle.Reset();
    }

    _ImGuiContext.Reset();
    _SlateWindow.Reset();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_Window::
    Get_World() const
    -> UWorld*
{
    if (NOT GEngine)
    { return nullptr; }

    for (const auto& Context : GEngine->GetWorldContexts())
    {
        if (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE)
        {
            return Context.World();
        }
    }

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
