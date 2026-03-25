#include "CkHfsmViewer_Window.h"

#include "CkStateMachineViewer/CkHfsmViewer_Log.h"

#include "CkCore/Ensure/CkEnsure.h"

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

    if (AllSms.IsEmpty())
    {
        ImGui::Begin("##HfsmViewer");
        ImGui::TextColored({0.6f, 0.6f, 0.6f, 1.0f}, "No state machines found in world.");
        ImGui::End();
        return;
    }

    _SelectedSmIndex = FMath::Clamp(_SelectedSmIndex, 0, AllSms.Num() - 1);

    RenderSmSelector();

    // Copy selected SM so the graph renderer can mutate layout positions
    auto SelectedSmData = AllSms[_SelectedSmIndex];

    auto Command = _GraphRenderer.Render(SelectedSmData);

    if (Command.Type == FCkHfsmViewer_Command::EType::ForceTransition)
    {
        auto SmHandle = AllSms[_SelectedSmIndex].Handle;
        UCk_Utils_StateMachine_UE::Request_Transition(SmHandle, Command.TargetStateClass);
    }

    RenderHistory(AllSms[_SelectedSmIndex]);
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

    ImGui::Begin("##SmSelector", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);

    if (AllSms.Num() > 1)
    {
        ImGui::Text("State Machine:");
        ImGui::SameLine();

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

    ImGui::End();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_Window::
    RenderHistory(
        const FCkHfsmViewer_SmInfo& InSmInfo)
    -> void
{
    if (InSmInfo.History.IsEmpty())
    { return; }

    ImGui::Begin("History##SmHistory");

    ImGui::Text("State Transition History");
    ImGui::Separator();

    constexpr auto MaxDisplayedEntries = 50;
    auto StartIndex = FMath::Max(0, InSmInfo.History.Num() - MaxDisplayedEntries);

    if (ImGui::BeginTable("##HistoryTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
    {
        ImGui::TableSetupColumn("Frame", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("From", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("To", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (auto Index = InSmInfo.History.Num() - 1; Index >= StartIndex; --Index)
        {
            const auto& Entry = InSmInfo.History[Index];

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%llu", Entry.FrameNumber);

            ImGui::TableSetColumnIndex(1);
            auto FromLabel = StringCast<ANSICHAR>(*Entry.FromStateName);
            ImGui::TextColored({0.6f, 0.6f, 0.6f, 1.0f}, "%s", FromLabel.Get());

            ImGui::TableSetColumnIndex(2);
            auto ToLabel = StringCast<ANSICHAR>(*Entry.ToStateName);
            ImGui::TextColored({0.3f, 0.69f, 0.31f, 1.0f}, "%s", ToLabel.Get());
        }

        ImGui::EndTable();
    }

    ImGui::End();
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
