#pragma once

#include "CoreMinimal.h"

#include "CkInsightsAnalyzer/Core/CkTraceSession.h"
#include "CkInsightsAnalyzer/Report/CkFrameReport.h"
#include "CkInsightsAnalyzer/Tab/SCkFrameBarChart.h"

#include <Async/Future.h>
#include <Containers/Ticker.h>
#include <Widgets/SCompoundWidget.h>
#include <Widgets/Text/STextBlock.h>
#include <Widgets/Text/SMultiLineEditableText.h>
#include <Widgets/Input/SButton.h>
#include <Widgets/Input/STextComboBox.h>

// --------------------------------------------------------------------------------------------------------------------

/**
 * Main editor tab for the Insights Analyzer.
 *
 * Layout (top to bottom):
 *   1. Toolbar  — [Open .utrace...] [Depth ▾] [Analyze Worst 10] [Copy to Clipboard]  Status text
 *   2. Frame bar chart (SCkFrameBarChart, ~200px)
 *   3. Separator
 *   4. Results panel (SScrollBox with read-only multi-line text, fills remaining space)
 */
class CKINSIGHTSANALYZER_API SCkInsightsAnalyzerTab : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCkInsightsAnalyzerTab) {}
    SLATE_END_ARGS()

    auto Construct(const FArguments& InArgs) -> void;

private:

    // ---- UI Construction ----

    auto DoCreateToolbar() -> TSharedRef<SWidget>;
    auto DoCreateResultsPanel() -> TSharedRef<SWidget>;

    // ---- Button Handlers ----

    auto DoOnOpenTraceClicked() -> FReply;
    auto DoOnAnalyzeWorstClicked() -> FReply;
    auto DoOnCopyToClipboardClicked() -> FReply;

    // ---- Depth Selector ----

    auto DoOnDepthSelectionChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo) -> void;
    auto DoGetReportDepth() const -> ECkReportDepth;

    // ---- Chart Delegate ----

    auto DoOnFrameSelectionChanged(uint64 StartFrame, uint64 EndFrame) -> void;

    // ---- Helpers ----

    auto DoSetStatus(const FString& Text) -> void;
    auto DoSetReport(const FString& ReportText) -> void;
    auto DoAnalyzeSingleFrame(uint64 FrameIndex) -> void;
    auto DoAnalyzeFrameRange(uint64 StartFrame, uint64 EndFrame) -> void;

    // ---- Async Loading ----

    enum class ELoadingState : uint8
    {
        Idle,
        Opening,
        ReadingFrames,
    };

    auto DoStartAsyncOpen(const FString& TracePath) -> void;
    auto DoOnLoadingTick(float DeltaTime) -> bool;
    auto DoLoadFrameChunk() -> bool;
    auto DoFinishLoading() -> void;
    auto DoCancelLoading() -> void;
    auto DoIsLoading() const -> bool { return _LoadingState != ELoadingState::Idle; }

private:
    FCk_TraceSession _Session;

    TSharedPtr<SCkFrameBarChart> _FrameBarChart;
    TSharedPtr<STextBlock> _StatusText;
    TSharedPtr<SMultiLineEditableText> _ReportText;

    // Depth dropdown
    TArray<TSharedPtr<FString>> _DepthOptions;
    TSharedPtr<STextComboBox> _DepthCombo;
    ECkReportDepth _ReportDepth = ECkReportDepth::Standard;

    FString _CurrentReport;

    // Async loading state
    ELoadingState _LoadingState = ELoadingState::Idle;
    FTSTicker::FDelegateHandle _LoadingTickerHandle;
    TFuture<bool> _OpenFuture;
    FString _PendingTracePath;
    TSharedPtr<FCk_TraceSession> _PendingSession;
    uint64 _TotalFrameCount = 0;
    uint64 _LoadedFrameCount = 0;
    TArray<double> _PendingFrameDurations;
};

// --------------------------------------------------------------------------------------------------------------------
