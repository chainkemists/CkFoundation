#pragma once

#include "CoreMinimal.h"

#include "CkInsightsAnalyzer/Core/CkTraceSession.h"
#include "CkInsightsAnalyzer/Report/CkFrameReport.h"
#include "CkInsightsAnalyzer/Report/CkMultiFrameReport.h"
#include "CkInsightsAnalyzer/Tab/SCkFrameBarChart.h"

#include "CkEditorTools/Style/CkStyle.h"

#include <Async/Future.h>
#include <Containers/Ticker.h>
#include <Widgets/SCompoundWidget.h>
#include <Widgets/SBoxPanel.h>
#include <Widgets/Text/STextBlock.h>
#include <Widgets/Text/SMultiLineEditableText.h>
#include <Widgets/Input/SButton.h>
#include <Widgets/Input/STextComboBox.h>
#include <Widgets/Views/STreeView.h>

// --------------------------------------------------------------------------------------------------------------------

/**
 * Main editor tab for the Insights Analyzer.
 *
 * Layout (top to bottom):
 *   1. Toolbar       — [Open .utrace...] [Depth ▾] [Analyze Worst 10] [Copy Report]  Status pill
 *   2. Summary strip — stat tiles (trace info; frame/aggregate numbers after analysis)
 *   3. Frame bar chart (SCkFrameBarChart, ~200px)
 *   4. Results       — splitter: hot-path tree (left) | categories / top timers /
 *                      worst frames / category averages (right)
 *   5. Raw report    — collapsed expandable area with the markdown text (also what
 *                      "Copy Report" puts on the clipboard)
 *
 * All colors/fonts route through CkStyle:: (CkEditorTools). Side panels are
 * rebuild-on-analysis (not SListView) — they are small, and rebuilding avoids
 * the listview-inside-scrollbox desired-size pitfalls.
 */
class CKINSIGHTSANALYZER_API SCkInsightsAnalyzerTab : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCkInsightsAnalyzerTab) {}
    SLATE_END_ARGS()

    auto Construct(const FArguments& InArgs) -> void;

private:

    // ---- Results mode ----

    enum class EResultsMode : uint8
    {
        None,
        SingleFrame,
        MultiFrame,
    };

    // ---- UI Construction ----

    auto DoCreateToolbar() -> TSharedRef<SWidget>;
    auto DoCreateSummaryStrip() -> TSharedRef<SWidget>;
    auto DoCreateResultsArea() -> TSharedRef<SWidget>;
    auto DoCreateHotPathPanel() -> TSharedRef<SWidget>;
    auto DoCreateSidePanels() -> TSharedRef<SWidget>;
    auto DoCreateRawReportArea() -> TSharedRef<SWidget>;

    // ---- Hot-path tree ----

    auto DoGenerateHotPathRow(TSharedPtr<FCk_HotPathNode> InNode,
                              const TSharedRef<STableViewBase>& InOwnerTable) -> TSharedRef<ITableRow>;
    auto DoExpandHotPathDefaults() -> void;

    // ---- Side panel row rebuilds ----

    auto DoRebuildCategoryRows() -> void;
    auto DoRebuildTopTimerRows() -> void;
    auto DoRebuildWorstFrameRows() -> void;
    auto DoRebuildCategoryAvgRows() -> void;

    // ---- Button Handlers ----

    auto DoOnOpenTraceClicked() -> FReply;
    auto DoOnAnalyzeWorstClicked() -> FReply;
    auto DoOnCopyToClipboardClicked() -> FReply;
    auto DoOnWorstFrameClicked(uint64 FrameIndex) -> FReply;

    // ---- Depth Selector ----

    auto DoOnDepthSelectionChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo) -> void;

    // ---- Chart Delegate ----

    auto DoOnFrameSelectionChanged(uint64 StartFrame, uint64 EndFrame) -> void;

    // ---- Helpers ----

    auto DoSetStatus(const FString& Text, ECk_Tone InTone = ECk_Tone::Neutral) -> void;
    auto DoSetReport(const FString& ReportText) -> void;
    auto DoClearResults() -> void;
    auto DoAnalyzeSingleFrame(uint64 FrameIndex) -> void;
    auto DoAnalyzeFrameRange(uint64 StartFrame, uint64 EndFrame) -> void;
    auto DoRerunCurrentSelection() -> void;
    auto DoPopulateMultiFrame(const FCk_MultiFrameStats& Stats) -> void;

    // ---- Summary strip ----

    auto DoAddSummaryTile(const FString& InLabel, const FString& InValue, const FLinearColor& InValueColor) -> void;
    auto DoRebuildSummaryStrip_TraceInfo() -> void;
    auto DoRebuildSummaryStrip_SingleFrame(const FCk_FrameAnalysisResult& Result) -> void;
    auto DoRebuildSummaryStrip_MultiFrame(const FCk_MultiFrameStats& Stats) -> void;

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

    // Status pill tone (drives the dot color next to the status text)
    ECk_Tone _StatusTone = ECk_Tone::Neutral;

    // Depth dropdown
    TArray<TSharedPtr<FString>> _DepthOptions;
    TSharedPtr<STextComboBox> _DepthCombo;
    ECkReportDepth _ReportDepth = ECkReportDepth::Standard;

    FString _CurrentReport;

    // Summary strip (rebuilt per analysis)
    TSharedPtr<SHorizontalBox> _SummaryBox;

    // Results state
    EResultsMode _ResultsMode = EResultsMode::None;
    double _AnalyzedFrameMs = 0.0; // frame duration backing the hot-path %-of-frame column

    // Hot-path tree
    TArray<TSharedPtr<FCk_HotPathNode>> _HotPathRoots;
    TSharedPtr<STreeView<TSharedPtr<FCk_HotPathNode>>> _HotPathTree;

    // Side panel data (rebuild-on-analysis)
    TArray<FCk_CategorySummaryEntry> _Categories;
    TArray<FCk_TopTimerEntry> _TopTimers;
    TArray<FCk_FrameSummary> _WorstFrames;          // persists across single-frame drills
    TArray<FCk_MultiFrameStats::FCategoryStats> _CategoryAverages;

    // Side panel row containers
    TSharedPtr<SVerticalBox> _CategoryRowsBox;
    TSharedPtr<SVerticalBox> _TopTimerRowsBox;
    TSharedPtr<SVerticalBox> _WorstFrameRowsBox;
    TSharedPtr<SVerticalBox> _CategoryAvgRowsBox;

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
