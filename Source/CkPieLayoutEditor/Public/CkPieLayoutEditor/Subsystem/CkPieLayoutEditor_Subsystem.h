#pragma once

#include "CkCore/Chrono/CkChrono.h"
#include "CkCore/Macros/CkMacros.h"

#include "Containers/Ticker.h"

#include <CoreMinimal.h>
#include <EditorSubsystem.h>

#include "CkPieLayoutEditor_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class SWindow;

// A PIE window's screen rect captured before this plugin first moved it, so it can be restored on Stop.
struct FCk_PieLayout_WindowRestoreEntry
{
    TWeakPtr<SWindow> Window;
    FVector2D         Position = FVector2D::ZeroVector;
    FVector2D         Size     = FVector2D::ZeroVector;
};

// --------------------------------------------------------------------------------------------------------------------

// Editor subsystem that arranges Play-In-Editor windows into a monitor-aware grid.
//
// On PostPIEStarted it (optionally) runs an initial arrange pass after a short delay, then keeps a brief
// retry window alive so client windows created shortly after Play are also placed. The arrange logic itself
// lives in the ck::pie_layout namespace in the .cpp.
UCLASS()
class CKPIELAYOUTEDITOR_API UCk_PieLayout_Subsystem_UE : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_PieLayout_Subsystem_UE);

public:
    auto
    Initialize(
        FSubsystemCollectionBase& InCollection) -> void override;

    auto
    Deinitialize() -> void override;

public:
    // Arrange the currently-open PIE windows immediately. Backs the "Arrange PIE Windows Now" menu action
    // and is safe to call manually at any time. Arranges even a single window (ignores Arrange Single Window).
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|PieLayout",
              DisplayName = "[Ck][PieLayout] Arrange PIE Windows Now")
    void
    Request_ArrangeNow();

private:
    auto
    OnPostPIEStarted(
        bool InIsSimulating) -> void;

    auto
    OnPrePIEEnded(
        bool InIsSimulating) -> void;

    auto
    OnInitialArrangeTick(
        float InDeltaTime) -> bool;

    auto
    OnRetryTick(
        float InDeltaTime) -> bool;

    auto
    DoStopTickers() -> void;

private:
    FDelegateHandle _PostPIEStarted_DelegateHandle;
    FDelegateHandle _PrePIEEnded_DelegateHandle;

    FTSTicker::FDelegateHandle _InitialDelayTickerHandle;
    FTSTicker::FDelegateHandle _RetryTickerHandle;

    FCk_Chrono _RetryChrono;

    // Pre-arrange window rects captured at PostPIEStarted; restored on EndPIE when the setting is enabled.
    TArray<FCk_PieLayout_WindowRestoreEntry> _OriginalWindowRects;
};

// --------------------------------------------------------------------------------------------------------------------
