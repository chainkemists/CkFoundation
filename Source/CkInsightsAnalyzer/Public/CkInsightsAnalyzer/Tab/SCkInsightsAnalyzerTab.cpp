#include "CkInsightsAnalyzer/Tab/SCkInsightsAnalyzerTab.h"

#include "CkInsightsAnalyzer/Core/CkFrameAnalyzer.h"
#include "CkInsightsAnalyzer/Report/CkFrameReport.h"
#include "CkInsightsAnalyzer/Report/CkMultiFrameReport.h"
#include "CkInsightsAnalyzer_Log.h"

#include <Async/Async.h>
#include <DesktopPlatformModule.h>
#include <HAL/PlatformApplicationMisc.h>
#include <Styling/AppStyle.h>
#include <Widgets/Layout/SBorder.h>
#include <Widgets/Layout/SBox.h>
#include <Widgets/Layout/SSeparator.h>
#include <Widgets/Layout/SScrollBox.h>

// --------------------------------------------------------------------------------------------------------------------

namespace InsightsAnalyzerTab_Constants
{
    constexpr float PanelPadding = 8.0f;
    constexpr float ButtonWidth = 160.0f;
    constexpr float SectionSpacing = 8.0f;
    constexpr float ChartHeight = 200.0f;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkInsightsAnalyzerTab::
    Construct(const FArguments& InArgs)
    -> void
{
    _DepthOptions.Add(MakeShared<FString>(TEXT("Full")));
    _DepthOptions.Add(MakeShared<FString>(TEXT("Standard")));
    _DepthOptions.Add(MakeShared<FString>(TEXT("Concise")));
    _DepthOptions.Add(MakeShared<FString>(TEXT("Hot Paths Only")));

    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(InsightsAnalyzerTab_Constants::PanelPadding)
        [
            SNew(SVerticalBox)

            // Toolbar
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, InsightsAnalyzerTab_Constants::SectionSpacing)
            [
                DoCreateToolbar()
            ]

            // Separator
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, InsightsAnalyzerTab_Constants::SectionSpacing)
            [
                SNew(SSeparator)
            ]

            // Frame bar chart
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, InsightsAnalyzerTab_Constants::SectionSpacing)
            [
                SNew(SBox)
                .HeightOverride(InsightsAnalyzerTab_Constants::ChartHeight)
                [
                    SAssignNew(_FrameBarChart, SCkFrameBarChart)
                    .TargetFrameMs(16.67)
                    .OnFrameSelectionChanged(
                        FOnFrameSelectionChanged::CreateSP(this, &SCkInsightsAnalyzerTab::DoOnFrameSelectionChanged))
                ]
            ]

            // Separator
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, InsightsAnalyzerTab_Constants::SectionSpacing)
            [
                SNew(SSeparator)
            ]

            // Results panel
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            [
                DoCreateResultsPanel()
            ]
        ]
    ];
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkInsightsAnalyzerTab::
    DoCreateToolbar()
    -> TSharedRef<SWidget>
{
    return SNew(SHorizontalBox)

        // Open .utrace button
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0, 0, InsightsAnalyzerTab_Constants::SectionSpacing, 0)
        [
            SNew(SBox)
            .WidthOverride(InsightsAnalyzerTab_Constants::ButtonWidth)
            [
                SNew(SButton)
                .Text(FText::FromString(TEXT("Open .utrace...")))
                .ToolTipText(FText::FromString(TEXT("Open an Unreal Insights trace file for analysis")))
                .OnClicked(this, &SCkInsightsAnalyzerTab::DoOnOpenTraceClicked)
                .HAlign(HAlign_Center)
            ]
        ]

        // Depth dropdown
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(0, 0, InsightsAnalyzerTab_Constants::SectionSpacing, 0)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 4, 0)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Depth:")))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SBox)
                .WidthOverride(120.0f)
                [
                    SAssignNew(_DepthCombo, STextComboBox)
                    .OptionsSource(&_DepthOptions)
                    .InitiallySelectedItem(_DepthOptions[1]) // Standard
                    .OnSelectionChanged(this, &SCkInsightsAnalyzerTab::DoOnDepthSelectionChanged)
                ]
            ]
        ]

        // Analyze Worst 10
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0, 0, InsightsAnalyzerTab_Constants::SectionSpacing, 0)
        [
            SNew(SBox)
            .WidthOverride(InsightsAnalyzerTab_Constants::ButtonWidth)
            [
                SNew(SButton)
                .Text(FText::FromString(TEXT("Analyze Worst 10")))
                .ToolTipText(FText::FromString(TEXT("Find and analyze the 10 worst frames in the trace")))
                .OnClicked(this, &SCkInsightsAnalyzerTab::DoOnAnalyzeWorstClicked)
                .HAlign(HAlign_Center)
            ]
        ]

        // Copy to Clipboard
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0, 0, InsightsAnalyzerTab_Constants::SectionSpacing, 0)
        [
            SNew(SBox)
            .WidthOverride(InsightsAnalyzerTab_Constants::ButtonWidth)
            [
                SNew(SButton)
                .Text(FText::FromString(TEXT("Copy to Clipboard")))
                .ToolTipText(FText::FromString(TEXT("Copy the current report to the system clipboard")))
                .OnClicked(this, &SCkInsightsAnalyzerTab::DoOnCopyToClipboardClicked)
                .HAlign(HAlign_Center)
            ]
        ]

        // Status text (fills remaining space)
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .VAlign(VAlign_Center)
        .Padding(InsightsAnalyzerTab_Constants::SectionSpacing, 0, 0, 0)
        [
            SAssignNew(_StatusText, STextBlock)
            .Text(FText::FromString(TEXT("No trace loaded. Click \"Open .utrace...\" to begin.")))
        ];
}

auto
    SCkInsightsAnalyzerTab::
    DoCreateResultsPanel()
    -> TSharedRef<SWidget>
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
        .Padding(4.0f)
        [
            SNew(SScrollBox)
            .ConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible)
            + SScrollBox::Slot()
            [
                SAssignNew(_ReportText, SMultiLineEditableText)
                .IsReadOnly(true)
                .AutoWrapText(false)
                .Text(FText::GetEmpty())
            ]
        ];
}

// --------------------------------------------------------------------------------------------------------------------
// Button Handlers
// --------------------------------------------------------------------------------------------------------------------

auto
    SCkInsightsAnalyzerTab::
    DoOnOpenTraceClicked()
    -> FReply
{
    if (DoIsLoading())
    {
        DoSetStatus(TEXT("Loading in progress..."));
        return FReply::Handled();
    }

    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform)
    {
        DoSetStatus(TEXT("Desktop platform not available."));
        return FReply::Handled();
    }

    TArray<FString> OutFiles;
    const bool bOpened = DesktopPlatform->OpenFileDialog(
        FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
        TEXT("Open Unreal Insights Trace"),
        FPaths::ProjectSavedDir() / TEXT("TraceSessions"),
        TEXT(""),
        TEXT("Unreal Trace Files (*.utrace)|*.utrace"),
        EFileDialogFlags::None,
        OutFiles);

    if (!bOpened || OutFiles.Num() == 0)
    {
        return FReply::Handled();
    }

    // Close any existing session
    _Session.Close();
    _FrameBarChart->ClearFrameData();
    DoSetReport(TEXT(""));

    DoStartAsyncOpen(OutFiles[0]);
    return FReply::Handled();
}

auto
    SCkInsightsAnalyzerTab::
    DoOnAnalyzeWorstClicked()
    -> FReply
{
    if (!_Session.IsOpen() || DoIsLoading())
    {
        DoSetStatus(TEXT("No trace loaded. Open a .utrace file first."));
        return FReply::Handled();
    }

    DoSetStatus(TEXT("Analyzing worst 10 frames..."));

    FCk_MultiFrameReportConfig Config;
    Config.TargetFrameMs = 16.67;
    Config.WorstFrameCount = 10;
    Config.Depth = _ReportDepth;
    Config.ApplyDepth();
    Config.WorstFrameCount = 10; // Override depth's default worst count

    FCk_MultiFrameReport MultiReport(Config);
    const FString Report = MultiReport.AnalyzeWorstFrames(_Session, 10);

    DoSetReport(Report);
    DoSetStatus(TEXT("Worst 10 frames analysis complete."));

    return FReply::Handled();
}

auto
    SCkInsightsAnalyzerTab::
    DoOnCopyToClipboardClicked()
    -> FReply
{
    if (_CurrentReport.IsEmpty())
    {
        DoSetStatus(TEXT("No report to copy."));
        return FReply::Handled();
    }

    FPlatformApplicationMisc::ClipboardCopy(*_CurrentReport);
    DoSetStatus(TEXT("Report copied to clipboard."));

    return FReply::Handled();
}

// --------------------------------------------------------------------------------------------------------------------
// Depth Selector
// --------------------------------------------------------------------------------------------------------------------

auto
    SCkInsightsAnalyzerTab::
    DoOnDepthSelectionChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
    -> void
{
    if (!NewValue.IsValid()) return;

    if (*NewValue == TEXT("Full"))           _ReportDepth = ECkReportDepth::Full;
    else if (*NewValue == TEXT("Standard"))  _ReportDepth = ECkReportDepth::Standard;
    else if (*NewValue == TEXT("Concise"))   _ReportDepth = ECkReportDepth::Concise;
    else                                    _ReportDepth = ECkReportDepth::HotPathsOnly;
}

auto
    SCkInsightsAnalyzerTab::
    DoGetReportDepth() const
    -> ECkReportDepth
{
    return _ReportDepth;
}

// --------------------------------------------------------------------------------------------------------------------
// Chart Delegate
// --------------------------------------------------------------------------------------------------------------------

auto
    SCkInsightsAnalyzerTab::
    DoOnFrameSelectionChanged(uint64 StartFrame, uint64 EndFrame)
    -> void
{
    // Allow frame selection during ReadingFrames (user can inspect already-loaded frames).
    // Only block during Opening when the session isn't ready yet.
    if (!_Session.IsOpen() || _LoadingState == ELoadingState::Opening) return;

    if (StartFrame == EndFrame)
    {
        DoAnalyzeSingleFrame(StartFrame);
    }
    else
    {
        DoAnalyzeFrameRange(StartFrame, EndFrame);
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------------------------------------------------

auto
    SCkInsightsAnalyzerTab::
    DoSetStatus(const FString& Text)
    -> void
{
    if (_StatusText.IsValid())
    {
        _StatusText->SetText(FText::FromString(Text));
    }
}

auto
    SCkInsightsAnalyzerTab::
    DoSetReport(const FString& ReportText)
    -> void
{
    _CurrentReport = ReportText;
    if (_ReportText.IsValid())
    {
        _ReportText->SetText(FText::FromString(ReportText));
    }
}

auto
    SCkInsightsAnalyzerTab::
    DoAnalyzeSingleFrame(uint64 FrameIndex)
    -> void
{
    DoSetStatus(FString::Printf(TEXT("Analyzing frame %llu..."), FrameIndex));

    FCk_FrameAnalysisResult Result = FCk_FrameAnalyzer::AnalyzeFrame(_Session, FrameIndex);
    if (!Result.IsValid())
    {
        DoSetStatus(FString::Printf(TEXT("Frame %llu produced no analysis data."), FrameIndex));
        DoSetReport(TEXT(""));
        return;
    }

    FCk_FrameReportConfig Config;
    Config.TargetFrameMs = 16.67;
    Config.Depth = _ReportDepth;
    Config.ApplyDepth();

    FCk_FrameReport FrameReport(Config);
    const FString Report = FrameReport.Generate(_Session, Result);

    DoSetReport(Report);
    DoSetStatus(FString::Printf(TEXT("Frame %llu: %.2fms"), FrameIndex, Result.FrameDurationMs));
}

auto
    SCkInsightsAnalyzerTab::
    DoAnalyzeFrameRange(uint64 StartFrame, uint64 EndFrame)
    -> void
{
    const uint64 Count = EndFrame - StartFrame + 1;
    DoSetStatus(FString::Printf(TEXT("Analyzing frames %llu-%llu (%llu frames)..."), StartFrame, EndFrame, Count));

    FCk_MultiFrameReportConfig Config;
    Config.TargetFrameMs = 16.67;
    Config.Depth = _ReportDepth;
    Config.ApplyDepth();

    FCk_MultiFrameReport MultiReport(Config);
    // EndFrame is inclusive from the chart, but AnalyzeAndGenerate expects exclusive
    const FString Report = MultiReport.AnalyzeAndGenerate(_Session, StartFrame, EndFrame + 1);

    DoSetReport(Report);
    DoSetStatus(FString::Printf(TEXT("Analyzed frames %llu-%llu (%llu frames)"), StartFrame, EndFrame, Count));
}

// --------------------------------------------------------------------------------------------------------------------
// Async Loading
// --------------------------------------------------------------------------------------------------------------------

auto
    SCkInsightsAnalyzerTab::
    DoStartAsyncOpen(const FString& TracePath)
    -> void
{
    _PendingTracePath = TracePath;
    _LoadingState = ELoadingState::Opening;

    DoSetStatus(FString::Printf(TEXT("Opening: %s ..."), *FPaths::GetCleanFilename(TracePath)));

    // Prepare analysis service on game thread (FModuleManager is not thread-safe)
    _PendingSession = MakeShared<FCk_TraceSession>();
    if (!_PendingSession->PrepareAnalysisService())
    {
        DoSetStatus(TEXT("Failed to create analysis service."));
        _PendingSession.Reset();
        _LoadingState = ELoadingState::Idle;
        return;
    }

    // Launch heavy Analyze() on background thread
    TSharedPtr<FCk_TraceSession> SessionForAsync = _PendingSession;
    FString PathCopy = TracePath;

    _OpenFuture = Async(EAsyncExecution::ThreadPool,
        [SessionForAsync, PathCopy]() -> bool
        {
            return SessionForAsync->AnalyzeFile(PathCopy);
        });

    // Start polling ticker
    _LoadingTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateSP(this, &SCkInsightsAnalyzerTab::DoOnLoadingTick),
        0.1f);
}

auto
    SCkInsightsAnalyzerTab::
    DoOnLoadingTick(float DeltaTime)
    -> bool
{
    switch (_LoadingState)
    {
    case ELoadingState::Opening:
    {
        if (!_OpenFuture.IsReady())
        {
            return true; // keep ticking
        }

        const bool bSuccess = _OpenFuture.Get();
        if (!bSuccess)
        {
            DoSetStatus(FString::Printf(TEXT("Failed to open: %s"),
                *FPaths::GetCleanFilename(_PendingTracePath)));
            _PendingSession.Reset();
            _LoadingState = ELoadingState::Idle;
            return false; // stop ticking
        }

        // Move session from background to member
        _Session = MoveTemp(*_PendingSession);
        _PendingSession.Reset();

        // Transition to frame reading
        _TotalFrameCount = _Session.GetFrameCount();
        _LoadedFrameCount = 0;
        _PendingFrameDurations.Reset();
        _PendingFrameDurations.Reserve(static_cast<int32>(_TotalFrameCount));

        if (_TotalFrameCount == 0)
        {
            DoFinishLoading();
            return false;
        }

        _LoadingState = ELoadingState::ReadingFrames;

        DoSetStatus(FString::Printf(
            TEXT("Reading frames: 0 / %llu ..."), _TotalFrameCount));

        return true; // continue ticking for frame reading
    }

    case ELoadingState::ReadingFrames:
    {
        if (DoLoadFrameChunk())
        {
            DoFinishLoading();
            return false; // stop ticking
        }
        return true; // keep ticking
    }

    default:
        return false;
    }
}

auto
    SCkInsightsAnalyzerTab::
    DoLoadFrameChunk()
    -> bool
{
    constexpr uint64 ChunkSize = 2000;

    TraceServices::FAnalysisSessionReadScope ReadScope = _Session.CreateReadScope();

    const uint64 EndFrame = FMath::Min(_LoadedFrameCount + ChunkSize, _TotalFrameCount);

    // Collect this chunk's durations separately so we can push them to the chart
    TArray<double> ChunkDurations;
    ChunkDurations.Reserve(static_cast<int32>(EndFrame - _LoadedFrameCount));

    for (uint64 i = _LoadedFrameCount; i < EndFrame; ++i)
    {
        const TraceServices::FFrame* Frame = _Session.GetFrame(i);
        if (Frame)
        {
            const double DurationMs = (Frame->EndTime - Frame->StartTime) * 1000.0;
            ChunkDurations.Add(DurationMs);
        }
        else
        {
            ChunkDurations.Add(0.0);
        }
    }

    _PendingFrameDurations.Append(ChunkDurations);

    // Push to bar chart progressively — bars fill up as frames are read
    if (_FrameBarChart.IsValid())
    {
        _FrameBarChart->AppendFrameData(ChunkDurations);
    }

    _LoadedFrameCount = EndFrame;

    DoSetStatus(FString::Printf(
        TEXT("Reading frames: %llu / %llu ..."),
        _LoadedFrameCount, _TotalFrameCount));

    return _LoadedFrameCount >= _TotalFrameCount;
}

auto
    SCkInsightsAnalyzerTab::
    DoFinishLoading()
    -> void
{
    // Chart already has all frame data from progressive AppendFrameData() calls.
    // Recalculate display max with proper P95 (outlier-resistant) now that all data is loaded.
    // This preserves the user's viewport/zoom unlike SetFrameData().
    if (_FrameBarChart.IsValid())
    {
        _FrameBarChart->RecalculateDisplayMax();
    }
    _PendingFrameDurations.Reset();

    const double DurationSec = _Session.GetDurationSeconds();
    DoSetStatus(FString::Printf(
        TEXT("Loaded: %s  |  %llu frames  |  %.1fs duration"),
        *FPaths::GetCleanFilename(_PendingTracePath), _TotalFrameCount, DurationSec));

    _LoadingState = ELoadingState::Idle;
    _LoadingTickerHandle.Reset();
}

auto
    SCkInsightsAnalyzerTab::
    DoCancelLoading()
    -> void
{
    if (_LoadingTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(_LoadingTickerHandle);
        _LoadingTickerHandle.Reset();
    }

    _PendingSession.Reset();
    _PendingFrameDurations.Reset();
    _LoadingState = ELoadingState::Idle;
}

// --------------------------------------------------------------------------------------------------------------------
