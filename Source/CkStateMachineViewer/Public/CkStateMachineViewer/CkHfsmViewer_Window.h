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
};

// --------------------------------------------------------------------------------------------------------------------
