#pragma once

#include "CkHfsmViewer_DataCollector.h"
#include "CkHfsmViewer_GraphRenderer.h"
#include "CkHfsmViewer_Types.h"

#include "CkStateMachine/CkStateMachine_Utils.h"

#include "CoreMinimal.h"
#include "Containers/Ticker.h"

// --------------------------------------------------------------------------------------------------------------------

class FImGuiContext;
class SWindow;

// --------------------------------------------------------------------------------------------------------------------

class FCkHfsmViewer_Window : public TSharedFromThis<FCkHfsmViewer_Window>
{
public:
    auto
    Open() -> void;

    auto
    Close() -> void;

    auto
    IsOpen() const -> bool;

private:
    auto
    Tick(
        float InDeltaTime) -> bool;

    auto
    RenderImGui() -> void;

    auto
    RenderSmSelector() -> void;

    auto
    RenderHistory(
        const FCkHfsmViewer_SmInfo& InSmInfo) -> void;

    auto
    RenderTimeline(
        const FCkHfsmViewer_SmInfo& InSmInfo) -> void;

    auto
    RenderRunSelector(
        const FCkHfsmViewer_SmInfo& InSmInfo) -> void;

    auto
    ComputeScrubSnapshot(
        const FCkHfsmViewer_SmInfo& InSmInfo) -> FCkHfsmViewer_ScrubSnapshot;

    auto
    GetActiveRunHistory(
        const FCkHfsmViewer_SmInfo& InSmInfo) const -> const TArray<FCkHfsmViewer_HistoryEntry>&;

    auto
    GetActiveRun(
        const FCkHfsmViewer_SmInfo& InSmInfo) const -> const FCkHfsmViewer_RunInfo&;

    auto
    OnWindowClosed(
        const TSharedRef<SWindow>& InWindow) -> void;

    auto
    Cleanup() -> void;

    auto
    Get_World() const -> UWorld*;

private:
    TSharedPtr<SWindow> _SlateWindow;
    TSharedPtr<FImGuiContext> _ImGuiContext;
    FTSTicker::FDelegateHandle _TickerHandle;

    FCkHfsmViewer_DataCollector _DataCollector;
    FCkHfsmViewer_GraphRenderer _GraphRenderer;

    int32 _SelectedSmIndex = 0;
    float _LastDeltaTime = 0.0f;

    float _DetailPanelWidth = 320.0f;
    float _HistoryHeight = 160.0f;

    FCkHfsmViewer_ScrubState _ScrubState;
    bool _IsDraggingPlayhead = false;
};

// --------------------------------------------------------------------------------------------------------------------
