#include "CkHfsmViewer_GraphRenderer.h"
#include "CkHfsmViewer_GraphRenderer_Constants.h"

#include "CkStateMachine/CkStateMachine_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_GraphRenderer::
    RenderDetailPanel(
        const FCkHfsmViewer_SmInfo& InSmInfo)
    -> FCkHfsmViewer_Command
{
    auto Command = FCkHfsmViewer_Command{};

    // Resolve which node to display: explicit selection overrides, otherwise current state
    auto DisplayIndex = _SelectedNodeIndex;

    if (DisplayIndex < 0 || DisplayIndex >= InSmInfo.States.Num())
    {
        DisplayIndex = InSmInfo.CurrentStateIndex;
    }

    if (DisplayIndex < 0 || DisplayIndex >= InSmInfo.States.Num())
    { return Command; }

    const auto& State = InSmInfo.States[DisplayIndex];

    // Compound nodes (sub-SM boxes) have no meaningful detail panel content
    if (State.IsCompoundNode)
    {
        ImGui::TextColored({0.56f, 0.56f, 0.56f, 1.0f}, "Sub-SM Container");
        return Command;
    }

    // State header
    auto NameAnsi = StringCast<ANSICHAR>(*State.StateName);
    ImGui::TextColored({0.88f, 0.88f, 0.88f, 1.0f}, "%s", NameAnsi.Get());

    if (State.IsCurrentState)
    {
        ImGui::SameLine();
        ImGui::TextColored({0.3f, 0.69f, 0.31f, 1.0f}, "(Active)");
    }
    else
    {
        ImGui::SameLine();
        ImGui::TextColored({0.56f, 0.56f, 0.56f, 1.0f}, "(Selected)");
    }

    // Dwell time
    if (State.HasBeenVisited)
    {
        if (State.IsCurrentDwellLive)
        {
            ImGui::TextColored({0.3f, 0.69f, 0.31f, 1.0f}, "Active for %.2fs", State.DwellTimeSeconds);
        }
        else
        {
            ImGui::TextColored({0.56f, 0.56f, 0.56f, 1.0f}, "Was active for %.2fs", State.DwellTimeSeconds);
        }
    }

    ImGui::Separator();

    // Breakpoints section
    {
        ImGui::TextColored({0.94f, 0.33f, 0.31f, 1.0f}, "Breakpoints");

        auto EntryBp = State.HasEntryBreakpoint;
        if (ImGui::Checkbox("Break on Entry", &EntryBp))
        {
            Command.Type = FCkHfsmViewer_Command::EType::ToggleStateEntryBreakpoint;
            Command.StateIndex = DisplayIndex;
        }

        auto ExitBp = State.HasExitBreakpoint;
        if (ImGui::Checkbox("Break on Exit", &ExitBp))
        {
            Command.Type = FCkHfsmViewer_Command::EType::ToggleStateExitBreakpoint;
            Command.StateIndex = DisplayIndex;
        }

        ImGui::Separator();
    }

    // Tasks section
    if (NOT State.Tasks.IsEmpty())
    {
        ImGui::TextColored({0.56f, 0.79f, 0.98f, 1.0f}, "Tasks (%d)", State.Tasks.Num());

        for (const auto& Task : State.Tasks)
        {
            auto TaskNameAnsi = StringCast<ANSICHAR>(*Task.ClassName);

            auto StatusColor = ImVec4{0.56f, 0.56f, 0.56f, 1.0f};
            auto StatusLabel = "?";

            switch (Task.LastResult)
            {
            case ECk_SmTaskResult::Running:   StatusColor = {1.0f, 0.76f, 0.03f, 1.0f}; StatusLabel = "Running";   break;
            case ECk_SmTaskResult::Succeeded: StatusColor = {0.3f, 0.69f, 0.31f, 1.0f}; StatusLabel = "Succeeded"; break;
            case ECk_SmTaskResult::Failed:    StatusColor = {0.94f, 0.33f, 0.31f, 1.0f}; StatusLabel = "Failed";    break;
            default: break;
            }

            ImGui::TextColored(StatusColor, "  [%s]", StatusLabel);
            ImGui::SameLine();
            ImGui::Text("%s", TaskNameAnsi.Get());

            auto ModeLabel = Task.Mode == ECk_SmTaskMode::EnterExitOnly ? "EnterExit" : "Continuous";
            ImGui::SameLine();
            ImGui::TextColored({0.45f, 0.45f, 0.45f, 1.0f}, "(%s)", ModeLabel);

            // Sub-SM status label
            if (Task.HasSubStateMachine)
            {
                ImGui::SameLine();

                if (ck::IsValid(Task.SubSmHandle))
                {
                    auto RunStatus = UCk_Utils_StateMachine_UE::Get_RunStatus(Task.SubSmHandle);
                    auto RunStatusText = RunStatus == ECk_SmRunStatus::Running ? "Running"
                        : RunStatus == ECk_SmRunStatus::Stopped ? "Stopped"
                        : "Paused";

                    ImGui::TextColored({0.26f, 0.65f, 0.96f, 1.0f}, "[Sub-SM: %s]", RunStatusText);
                }
                else
                {
                    ImGui::TextColored({0.45f, 0.45f, 0.45f, 1.0f}, "[Sub-SM: N/A]");
                }
            }
        }

        ImGui::Separator();
    }

    // Outgoing transitions
    auto HasOutgoing = false;
    auto TransitionIdx = 0;

    for (const auto& Transition : InSmInfo.Transitions)
    {
        if (Transition.SourceStateIndex == DisplayIndex)
        {
            if (NOT HasOutgoing)
            {
                ImGui::TextColored({0.56f, 0.79f, 0.98f, 1.0f}, "Outgoing Transitions");
                HasOutgoing = true;
            }

            auto TargetNameAnsi = StringCast<ANSICHAR>(*Transition.TargetStateName);
            auto AllSatisfied = Transition.AreAllConditionsSatisfied;
            auto HeaderColor = AllSatisfied
                ? ImVec4{0.3f, 0.69f, 0.31f, 1.0f}
                : ImVec4{0.56f, 0.56f, 0.56f, 1.0f};

            ImGui::TextColored(HeaderColor, "  -> %s  (%d/%d)",
                TargetNameAnsi.Get(), Transition.SatisfiedCount, Transition.TotalCount);

            // Transition breakpoint checkbox
            ImGui::SameLine();
            auto TransBp = Transition.HasBreakpoint;
            auto CheckboxId = FString::Printf(TEXT("##TransBp_%d"), TransitionIdx);
            auto CheckboxIdAnsi = StringCast<ANSICHAR>(*CheckboxId);

            if (ImGui::Checkbox(CheckboxIdAnsi.Get(), &TransBp))
            {
                Command.Type = FCkHfsmViewer_Command::EType::ToggleTransitionBreakpoint;
                Command.TransitionIndex = TransitionIdx;
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Break on transition fire");
            }

            for (const auto& Cond : Transition.Conditions)
            {
                auto CondNameAnsi = StringCast<ANSICHAR>(*Cond.ClassName);
                auto HasLiveData = ck::IsValid(Cond.Handle);

                if (HasLiveData)
                {
                    auto CondColor = Cond.IsSatisfied
                        ? ImVec4{0.3f, 0.69f, 0.31f, 1.0f}
                        : ImVec4{0.94f, 0.33f, 0.31f, 1.0f};
                    auto CondIcon = Cond.IsSatisfied ? "+" : "-";

                    ImGui::TextColored(CondColor, "    [%s]", CondIcon);
                    ImGui::SameLine();
                    ImGui::Text("%s", CondNameAnsi.Get());
                }
                else
                {
                    ImGui::TextColored({0.56f, 0.56f, 0.56f, 1.0f}, "    [?]");
                    ImGui::SameLine();
                    ImGui::TextColored({0.56f, 0.56f, 0.56f, 1.0f}, "%s", CondNameAnsi.Get());
                }

                auto ModeText = Cond.Mode == ECk_SmConditionMode::Polled ? "Polled" : "Event";
                ImGui::SameLine();
                ImGui::TextColored({0.45f, 0.45f, 0.45f, 1.0f}, "(%s)", ModeText);
            }
        }

        ++TransitionIdx;
    }

    if (HasOutgoing)
    {
        ImGui::Separator();
    }

    // Incoming transitions
    auto HasIncoming = false;
    for (const auto& Transition : InSmInfo.Transitions)
    {
        if (Transition.TargetStateIndex == DisplayIndex)
        {
            if (NOT HasIncoming)
            {
                ImGui::TextColored({0.56f, 0.79f, 0.98f, 1.0f}, "Incoming Transitions");
                HasIncoming = true;
            }

            if (Transition.SourceStateIndex >= 0 && Transition.SourceStateIndex < InSmInfo.States.Num())
            {
                auto SourceNameAnsi = StringCast<ANSICHAR>(*InSmInfo.States[Transition.SourceStateIndex].StateName);
                ImGui::TextColored({0.56f, 0.56f, 0.56f, 1.0f}, "  <- %s  (%d conditions)",
                    SourceNameAnsi.Get(), Transition.TotalCount);
            }
        }
    }

    return Command;
}

// --------------------------------------------------------------------------------------------------------------------
