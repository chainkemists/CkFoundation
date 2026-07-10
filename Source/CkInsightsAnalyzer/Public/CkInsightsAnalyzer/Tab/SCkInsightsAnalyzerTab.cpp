#include "CkInsightsAnalyzer/Tab/SCkInsightsAnalyzerTab.h"

#include "CkInsightsAnalyzer/Core/CkFrameAnalyzer.h"
#include "CkInsightsAnalyzer/Core/CkTimerCategorizer.h"
#include "CkInsightsAnalyzer_Log.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEditorTools/Style/CkStyle.h"

#include <Async/Async.h>
#include <DesktopPlatformModule.h>
#include <HAL/PlatformApplicationMisc.h>
#include <Styling/AppStyle.h>
#include <Styling/CoreStyle.h>
#include <Widgets/Images/SImage.h>
#include <Widgets/Layout/SBorder.h>
#include <Widgets/Layout/SBox.h>
#include <Widgets/Layout/SExpandableArea.h>
#include <Widgets/Layout/SScrollBox.h>
#include <Widgets/Layout/SSplitter.h>
#include <Widgets/SOverlay.h>
#include <Widgets/Views/SExpanderArrow.h>
#include <Widgets/Views/SHeaderRow.h>
#include <Widgets/Views/STableRow.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_insights_analyzer_tab
{
    constexpr float PanelPadding = 8.0f;
    constexpr float SectionSpacing = 8.0f;
    constexpr float ChartHeight = 200.0f;
    constexpr float SideBarWidth = 90.0f;    // proportion bar width in side panels
    constexpr double TargetFrameMs = 16.67;
    constexpr int32 TopTimerCount = 15;

    // ---- Fonts (sizes read live from the style settings at construct time) ----

    auto BodyFont()     -> FSlateFontInfo { return FCoreStyle::GetDefaultFontStyle("Regular", CkStyle::FontSizeBody()); }
    auto BodyBoldFont() -> FSlateFontInfo { return FCoreStyle::GetDefaultFontStyle("Bold", CkStyle::FontSizeBody()); }
    auto SmallFont()    -> FSlateFontInfo { return FCoreStyle::GetDefaultFontStyle("Regular", CkStyle::FontSizeSmall()); }
    auto ItalicFont()   -> FSlateFontInfo { return FCoreStyle::GetDefaultFontStyle("Italic", CkStyle::FontSizeSmall()); }
    auto MicroFont()    -> FSlateFontInfo { return FCoreStyle::GetDefaultFontStyle("Regular", CkStyle::FontSizeMicro()); }
    auto TileFont()     -> FSlateFontInfo { return FCoreStyle::GetDefaultFontStyle("Bold", CkStyle::FontSizeH2()); }

    // ---- Colors ----

    // Same thresholds as FCk_TimerCategorizer::SeverityIcon so text and icons agree.
    auto SeverityColor(double InMs) -> FLinearColor
    {
        if (InMs >= 5.0) { return CkStyle::Err(); }
        if (InMs >= 2.0) { return CkStyle::Warn(); }
        if (InMs >= 1.0) { return CkStyle::Info(); }
        return CkStyle::TextDim();
    }

    auto FrameBudgetColor(double InFrameMs) -> FLinearColor
    {
        if (InFrameMs > TargetFrameMs * 2.0) { return CkStyle::Err(); }
        if (InFrameMs > TargetFrameMs)       { return CkStyle::Warn(); }
        return CkStyle::Ok();
    }

    auto FrameBudgetTone(double InFrameMs) -> ECk_Tone
    {
        if (InFrameMs > TargetFrameMs * 2.0) { return ECk_Tone::Err; }
        if (InFrameMs > TargetFrameMs)       { return ECk_Tone::Warn; }
        return ECk_Tone::Ok;
    }

    // Stable per-category accent (hash into a small style-derived palette).
    auto CategoryColor(const FString& InName) -> FLinearColor
    {
        switch (GetTypeHash(InName) % 8u)
        {
            case 0:  return CkStyle::Info();
            case 1:  return CkStyle::Ok();
            case 2:  return CkStyle::Value_Tag();
            case 3:  return CkStyle::Accent();
            case 4:  return CkStyle::Value_Handle();
            case 5:  return CkStyle::Relationship();
            case 6:  return CkStyle::Value_String();
            default: return CkStyle::Value_Numeric();
        }
    }

    // ---- Small widget builders ----

    auto MakeHeading(const FString& InText) -> TSharedRef<SWidget>
    {
        return SNew(STextBlock)
            .Text(FText::FromString(InText.ToUpper()))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", CkStyle::PaneHeadingFontSize()))
            .ColorAndOpacity(CkStyle::PaneHeadingColor());
    }

    auto MakeDot(const FLinearColor& InColor, float InSize = 7.0f) -> TSharedRef<SWidget>
    {
        return SNew(SBox)
            .WidthOverride(InSize)
            .HeightOverride(InSize)
            [
                SNew(SImage)
                .Image(CkStyle::GetFilledBrush())
                .ColorAndOpacity(InColor)
            ];
    }

    auto MakeProportionBar(double InPct, const FLinearColor& InFillColor, float InWidth) -> TSharedRef<SWidget>
    {
        const float Frac = FMath::Clamp(static_cast<float>(InPct) / 100.0f, 0.0f, 1.0f);
        return SNew(SBox)
            .WidthOverride(InWidth)
            .HeightOverride(6.0f)
            .VAlign(VAlign_Fill)
            [
                SNew(SOverlay)
                + SOverlay::Slot()
                [
                    SNew(SImage)
                    .Image(CkStyle::GetFilledBrush())
                    .ColorAndOpacity(CkStyle::Bg3())
                ]
                + SOverlay::Slot()
                .HAlign(HAlign_Left)
                [
                    SNew(SBox)
                    .WidthOverride(FMath::Max(1.0f, InWidth * Frac))
                    [
                        SNew(SImage)
                        .Image(CkStyle::GetFilledBrush())
                        .ColorAndOpacity(InFillColor)
                    ]
                ]
            ];
    }

    auto MakeStatTile(const FString& InLabel, const FString& InValue, const FLinearColor& InValueColor) -> TSharedRef<SWidget>
    {
        return SNew(SBorder)
            .BorderImage(CkStyle::GetFilledBrush())
            .BorderBackgroundColor(CkStyle::Bg2())
            .Padding(FMargin(CkStyle::SpaceL, CkStyle::SpaceS))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(InLabel.ToUpper()))
                    .Font(MicroFont())
                    .ColorAndOpacity(CkStyle::TextMute())
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 1.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(InValue))
                    .Font(TileFont())
                    .ColorAndOpacity(InValueColor)
                ]
            ];
    }

    auto MakePanel(const FString& InHeading, const TSharedRef<SWidget>& InContent) -> TSharedRef<SWidget>
    {
        return SNew(SBorder)
            .BorderImage(CkStyle::GetFilledBrush())
            .BorderBackgroundColor(CkStyle::Bg1())
            .Padding(CkStyle::SpaceM)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, CkStyle::SpaceS)
                [
                    MakeHeading(InHeading)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    InContent
                ]
            ];
    }

    /** Format an integer with comma separators (e.g. 4160 → "4,160"). */
    auto FormatWithCommas(uint64 Value) -> FString
    {
        FString Raw = FString::Printf(TEXT("%llu"), Value);
        FString Result;
        const int32 Len = Raw.Len();
        for (int32 i = 0; i < Len; ++i)
        {
            if (i > 0 && (Len - i) % 3 == 0)
            {
                Result.AppendChar(TEXT(','));
            }
            Result.AppendChar(Raw[i]);
        }
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Hot-path tree row
// --------------------------------------------------------------------------------------------------------------------

class SCkInsights_HotPathRow : public SMultiColumnTableRow<TSharedPtr<FCk_HotPathNode>>
{
public:
    SLATE_BEGIN_ARGS(SCkInsights_HotPathRow)
        : _FrameDurationMs(0.0)
    {}
        SLATE_ARGUMENT(TSharedPtr<FCk_HotPathNode>, Node)
        SLATE_ARGUMENT(double, FrameDurationMs)
    SLATE_END_ARGS()

    auto Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable) -> void
    {
        _Node = InArgs._Node;
        _FrameDurationMs = InArgs._FrameDurationMs;

        SMultiColumnTableRow<TSharedPtr<FCk_HotPathNode>>::Construct(
            FSuperRowType::FArguments()
                .Padding(FMargin(0.0f, 1.0f))
                .ShowSelection(true),
            InOwnerTable);
    }

    virtual auto GenerateWidgetForColumn(const FName& InColumnName) -> TSharedRef<SWidget> override
    {
        using namespace ck_insights_analyzer_tab;

        if (ck::Is_NOT_Valid(_Node))
        {
            return SNullWidget::NullWidget;
        }

        if (InColumnName == TEXT("Timer"))
        {
            const TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);

            Row->AddSlot()
               .AutoWidth()
               [
                   SNew(SExpanderArrow, SharedThis(this))
                   .IndentAmount(12.0f)
               ];

            Row->AddSlot()
               .AutoWidth()
               .VAlign(VAlign_Center)
               .Padding(0.0f, 0.0f, CkStyle::SpaceS, 0.0f)
               [
                   MakeDot(SeverityColor(_Node->InclusiveMs))
               ];

            Row->AddSlot()
               .AutoWidth()
               .VAlign(VAlign_Center)
               [
                   SNew(STextBlock)
                   .Text(FText::FromString(_Node->DisplayName))
                   .Font(BodyFont())
                   .ColorAndOpacity(CkStyle::Text())
                   .ToolTipText(FText::FromString(_Node->RawName))
               ];

            const FString Breadcrumb = DoBuildBreadcrumbSuffix();
            if (NOT Breadcrumb.IsEmpty())
            {
                Row->AddSlot()
                   .AutoWidth()
                   .VAlign(VAlign_Center)
                   [
                       SNew(STextBlock)
                       .Text(FText::FromString(Breadcrumb))
                       .Font(ItalicFont())
                       .ColorAndOpacity(CkStyle::TextMute())
                   ];
            }

            return Row;
        }

        if (InColumnName == TEXT("Incl"))
        {
            return DoMakeCell(
                FCk_TimerCategorizer::FormatMs(_Node->InclusiveMs),
                BodyBoldFont(), SeverityColor(_Node->InclusiveMs));
        }

        if (InColumnName == TEXT("Self"))
        {
            const bool ShowSelf = _Node->ExclusiveMs > 0.05;
            return DoMakeCell(
                ShowSelf ? FCk_TimerCategorizer::FormatMs(_Node->ExclusiveMs) : FString(),
                BodyFont(), CkStyle::TextDim());
        }

        if (InColumnName == TEXT("Count"))
        {
            return DoMakeCell(
                (_Node->Count > 1) ? FCk_TimerCategorizer::FormatCount(_Node->Count) : FString(),
                BodyFont(), CkStyle::TextMute());
        }

        if (InColumnName == TEXT("Pct"))
        {
            const double Pct = (_FrameDurationMs > 0.0)
                ? (_Node->InclusiveMs / _FrameDurationMs) * 100.0
                : 0.0;

            return SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(CkStyle::SpaceS, 0.0f)
                [
                    MakeProportionBar(Pct, CkStyle::OverlayOf(SeverityColor(_Node->InclusiveMs), 0.85f), 60.0f)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(FString::Printf(TEXT("%.0f%%"), Pct)))
                    .Font(SmallFont())
                    .ColorAndOpacity(CkStyle::TextDim())
                ];
        }

        return SNullWidget::NullWidget;
    }

private:
    auto DoBuildBreadcrumbSuffix() const -> FString
    {
        // Show at most the last two collapsed wrappers, like the markdown report
        FString Breadcrumb;
        const int32 Start = FMath::Max(0, _Node->Breadcrumbs.Num() - 2);
        for (int32 i = Start; i < _Node->Breadcrumbs.Num(); ++i)
        {
            const FString Simplified = FCk_TimerCategorizer::SimplifyName(_Node->Breadcrumbs[i]);
            if (Simplified.Len() > 30)
            {
                continue;
            }
            if (NOT Breadcrumb.IsEmpty())
            {
                Breadcrumb += TEXT(" → ");
            }
            Breadcrumb += Simplified;
        }
        return Breadcrumb.IsEmpty() ? Breadcrumb : FString::Printf(TEXT("  (%s)"), *Breadcrumb);
    }

    auto DoMakeCell(const FString& InText, const FSlateFontInfo& InFont, const FLinearColor& InColor) const -> TSharedRef<SWidget>
    {
        return SNew(SBox)
            .Padding(FMargin(ck_insights_analyzer_tab::SectionSpacing * 0.5f, 0.0f))
            .VAlign(VAlign_Center)
            .HAlign(HAlign_Right)
            [
                SNew(STextBlock)
                .Text(FText::FromString(InText))
                .Font(InFont)
                .ColorAndOpacity(InColor)
            ];
    }

private:
    TSharedPtr<FCk_HotPathNode> _Node;
    double _FrameDurationMs = 0.0;
};

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkInsightsAnalyzerTab::
    Construct(const FArguments& InArgs)
    -> void
{
    using namespace ck_insights_analyzer_tab;

    _DepthOptions.Add(MakeShared<FString>(TEXT("Full")));
    _DepthOptions.Add(MakeShared<FString>(TEXT("Standard")));
    _DepthOptions.Add(MakeShared<FString>(TEXT("Concise")));
    _DepthOptions.Add(MakeShared<FString>(TEXT("Hot Paths Only")));

    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(CkStyle::GetFilledBrush())
        .BorderBackgroundColor(CkStyle::BgRoot())
        .Padding(PanelPadding)
        [
            SNew(SVerticalBox)

            // Toolbar
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, SectionSpacing)
            [
                DoCreateToolbar()
            ]

            // Summary strip (stat tiles; hidden until a trace is loaded)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, SectionSpacing)
            [
                DoCreateSummaryStrip()
            ]

            // Frame bar chart
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, SectionSpacing)
            [
                SNew(SBox)
                .HeightOverride(ChartHeight)
                [
                    SAssignNew(_FrameBarChart, SCkFrameBarChart)
                    .TargetFrameMs(TargetFrameMs)
                    .OnFrameSelectionChanged(
                        FOnFrameSelectionChanged::CreateSP(this, &SCkInsightsAnalyzerTab::DoOnFrameSelectionChanged))
                ]
            ]

            // Results (tree + side panels)
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(0.0f, 0.0f, 0.0f, SectionSpacing)
            [
                DoCreateResultsArea()
            ]

            // Raw markdown report (collapsed)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                DoCreateRawReportArea()
            ]
        ]
    ];
}

// --------------------------------------------------------------------------------------------------------------------
// UI Construction
// --------------------------------------------------------------------------------------------------------------------

auto
    SCkInsightsAnalyzerTab::
    DoCreateToolbar()
    -> TSharedRef<SWidget>
{
    using namespace ck_insights_analyzer_tab;

    return SNew(SHorizontalBox)

        // Open .utrace button
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0.0f, 0.0f, SectionSpacing, 0.0f)
        [
            SNew(SButton)
            .Text(FText::FromString(TEXT("Open .utrace...")))
            .ToolTipText(FText::FromString(TEXT("Open an Unreal Insights trace file for analysis")))
            .OnClicked(this, &SCkInsightsAnalyzerTab::DoOnOpenTraceClicked)
            .HAlign(HAlign_Center)
        ]

        // Depth dropdown
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(0.0f, 0.0f, SectionSpacing, 0.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0.0f, 0.0f, CkStyle::SpaceS, 0.0f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Depth:")))
                .Font(BodyFont())
                .ColorAndOpacity(CkStyle::TextDim())
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
        .Padding(0.0f, 0.0f, SectionSpacing, 0.0f)
        [
            SNew(SButton)
            .Text(FText::FromString(TEXT("Analyze Worst 10")))
            .ToolTipText(FText::FromString(TEXT("Find and analyze the 10 worst frames in the trace")))
            .OnClicked(this, &SCkInsightsAnalyzerTab::DoOnAnalyzeWorstClicked)
            .HAlign(HAlign_Center)
        ]

        // Copy Report
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0.0f, 0.0f, SectionSpacing, 0.0f)
        [
            SNew(SButton)
            .Text(FText::FromString(TEXT("Copy Report")))
            .ToolTipText(FText::FromString(TEXT("Copy the current markdown report to the system clipboard")))
            .OnClicked(this, &SCkInsightsAnalyzerTab::DoOnCopyToClipboardClicked)
            .HAlign(HAlign_Center)
        ]

        // Status pill (fills remaining space)
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .VAlign(VAlign_Center)
        .Padding(SectionSpacing, 0.0f, 0.0f, 0.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0.0f, 0.0f, CkStyle::SpaceS, 0.0f)
            [
                SNew(SBox)
                .WidthOverride(8.0f)
                .HeightOverride(8.0f)
                [
                    SNew(SImage)
                    .Image(CkStyle::GetFilledBrush())
                    .ColorAndOpacity_Lambda([this]() -> FSlateColor
                    {
                        return CkStyle::GetToneColor(_StatusTone);
                    })
                ]
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                SAssignNew(_StatusText, STextBlock)
                .Text(FText::FromString(TEXT("No trace loaded. Click \"Open .utrace...\" to begin.")))
                .Font(BodyFont())
                .ColorAndOpacity(CkStyle::TextDim())
            ]
        ];
}

auto
    SCkInsightsAnalyzerTab::
    DoCreateSummaryStrip()
    -> TSharedRef<SWidget>
{
    SAssignNew(_SummaryBox, SHorizontalBox);

    return SNew(SBox)
        .Visibility_Lambda([this]() -> EVisibility
        {
            return (ck::IsValid(_SummaryBox) && _SummaryBox->NumSlots() > 0)
                ? EVisibility::Visible
                : EVisibility::Collapsed;
        })
        [
            _SummaryBox.ToSharedRef()
        ];
}

auto
    SCkInsightsAnalyzerTab::
    DoCreateResultsArea()
    -> TSharedRef<SWidget>
{
    return SNew(SSplitter)
        .Orientation(Orient_Horizontal)
        .PhysicalSplitterHandleSize(2.0f)

        + SSplitter::Slot()
        .Value(0.62f)
        [
            DoCreateHotPathPanel()
        ]

        + SSplitter::Slot()
        .Value(0.38f)
        [
            DoCreateSidePanels()
        ];
}

auto
    SCkInsightsAnalyzerTab::
    DoCreateHotPathPanel()
    -> TSharedRef<SWidget>
{
    using namespace ck_insights_analyzer_tab;

    SAssignNew(_HotPathTree, STreeView<TSharedPtr<FCk_HotPathNode>>)
        .TreeItemsSource(&_HotPathRoots)
        .SelectionMode(ESelectionMode::Single)
        .OnGenerateRow(this, &SCkInsightsAnalyzerTab::DoGenerateHotPathRow)
        .OnGetChildren_Lambda([](TSharedPtr<FCk_HotPathNode> InNode, TArray<TSharedPtr<FCk_HotPathNode>>& OutChildren)
        {
            if (InNode.IsValid())
            {
                OutChildren = InNode->Children;
            }
        })
        .HeaderRow
        (
            SNew(SHeaderRow)
            + SHeaderRow::Column(TEXT("Timer"))
              .DefaultLabel(FText::FromString(TEXT("Timer")))
              .FillWidth(1.0f)
            + SHeaderRow::Column(TEXT("Incl"))
              .DefaultLabel(FText::FromString(TEXT("Incl")))
              .FixedWidth(76.0f)
              .HAlignHeader(HAlign_Right)
            + SHeaderRow::Column(TEXT("Self"))
              .DefaultLabel(FText::FromString(TEXT("Self")))
              .FixedWidth(76.0f)
              .HAlignHeader(HAlign_Right)
            + SHeaderRow::Column(TEXT("Count"))
              .DefaultLabel(FText::FromString(TEXT("Count")))
              .FixedWidth(64.0f)
              .HAlignHeader(HAlign_Right)
            + SHeaderRow::Column(TEXT("Pct"))
              .DefaultLabel(FText::FromString(TEXT("% Frame")))
              .FixedWidth(120.0f)
        );

    return SNew(SBorder)
        .BorderImage(CkStyle::GetFilledBrush())
        .BorderBackgroundColor(CkStyle::Bg1())
        .Padding(CkStyle::SpaceM)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, CkStyle::SpaceS)
            [
                MakeHeading(TEXT("Game Thread Hot Paths"))
            ]
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            [
                SNew(SOverlay)
                + SOverlay::Slot()
                [
                    _HotPathTree.ToSharedRef()
                ]
                + SOverlay::Slot()
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Font(BodyFont())
                    .ColorAndOpacity(CkStyle::TextMute())
                    .Visibility_Lambda([this]() -> EVisibility
                    {
                        return (_HotPathRoots.Num() == 0)
                            ? EVisibility::HitTestInvisible
                            : EVisibility::Collapsed;
                    })
                    .Text_Lambda([this]() -> FText
                    {
                        switch (_ResultsMode)
                        {
                            case EResultsMode::MultiFrame:
                                return FText::FromString(TEXT("Range analyzed — click a Worst Frame row or a single chart bar to see its hot paths."));
                            case EResultsMode::SingleFrame:
                                return FText::FromString(TEXT("No hot paths found for this frame."));
                            default:
                                return FText::FromString(TEXT("Open a .utrace, then click a frame bar (or drag a range) in the chart."));
                        }
                    })
                ]
            ]
        ];
}

auto
    SCkInsightsAnalyzerTab::
    DoCreateSidePanels()
    -> TSharedRef<SWidget>
{
    using namespace ck_insights_analyzer_tab;

    SAssignNew(_CategoryRowsBox, SVerticalBox);
    SAssignNew(_TopTimerRowsBox, SVerticalBox);
    SAssignNew(_WorstFrameRowsBox, SVerticalBox);
    SAssignNew(_CategoryAvgRowsBox, SVerticalBox);

    const auto MakeGatedPanel =
        [this](const FString& InHeading, const TSharedRef<SWidget>& InRowsBox, TFunction<bool()> InHasData) -> TSharedRef<SWidget>
    {
        return SNew(SBox)
            .Visibility_Lambda([InHasData = MoveTemp(InHasData)]() -> EVisibility
            {
                return InHasData() ? EVisibility::Visible : EVisibility::Collapsed;
            })
            .Padding(FMargin(SectionSpacing, 0.0f, 0.0f, SectionSpacing))
            [
                MakePanel(InHeading, InRowsBox)
            ];
    };

    return SNew(SScrollBox)
        .ConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible)

        + SScrollBox::Slot()
        [
            MakeGatedPanel(TEXT("Worst Frames"), _WorstFrameRowsBox.ToSharedRef(),
                [this]() { return _WorstFrames.Num() > 0; })
        ]

        + SScrollBox::Slot()
        [
            MakeGatedPanel(TEXT("Category Averages"), _CategoryAvgRowsBox.ToSharedRef(),
                [this]() { return _CategoryAverages.Num() > 0; })
        ]

        + SScrollBox::Slot()
        [
            MakeGatedPanel(TEXT("Categories (exclusive time)"), _CategoryRowsBox.ToSharedRef(),
                [this]() { return _Categories.Num() > 0; })
        ]

        + SScrollBox::Slot()
        [
            MakeGatedPanel(TEXT("Top Timers (exclusive time)"), _TopTimerRowsBox.ToSharedRef(),
                [this]() { return _TopTimers.Num() > 0; })
        ];
}

auto
    SCkInsightsAnalyzerTab::
    DoCreateRawReportArea()
    -> TSharedRef<SWidget>
{
    using namespace ck_insights_analyzer_tab;

    return SNew(SBox)
        .Visibility_Lambda([this]() -> EVisibility
        {
            return _CurrentReport.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
        })
        [
            SNew(SExpandableArea)
            .InitiallyCollapsed(true)
            .HeaderContent()
            [
                MakeHeading(TEXT("Raw Markdown Report"))
            ]
            .BodyContent()
            [
                SNew(SBox)
                .MaxDesiredHeight(260.0f)
                [
                    SNew(SBorder)
                    .BorderImage(CkStyle::GetFilledBrush())
                    .BorderBackgroundColor(CkStyle::Bg1())
                    .Padding(CkStyle::SpaceS)
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
                    ]
                ]
            ]
        ];
}

// --------------------------------------------------------------------------------------------------------------------
// Hot-path tree
// --------------------------------------------------------------------------------------------------------------------

auto
    SCkInsightsAnalyzerTab::
    DoGenerateHotPathRow(TSharedPtr<FCk_HotPathNode> InNode,
                         const TSharedRef<STableViewBase>& InOwnerTable)
    -> TSharedRef<ITableRow>
{
    return SNew(SCkInsights_HotPathRow, InOwnerTable)
        .Node(InNode)
        .FrameDurationMs(_AnalyzedFrameMs);
}

auto
    SCkInsightsAnalyzerTab::
    DoExpandHotPathDefaults()
    -> void
{
    if (ck::Is_NOT_Valid(_HotPathTree))
    {
        return;
    }

    // Expand roots and their direct children so the interesting depth is visible immediately
    for (const auto& Root : _HotPathRoots)
    {
        _HotPathTree->SetItemExpansion(Root, true);
        for (const auto& Child : Root->Children)
        {
            _HotPathTree->SetItemExpansion(Child, true);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Side panel row rebuilds
// --------------------------------------------------------------------------------------------------------------------

auto
    SCkInsightsAnalyzerTab::
    DoRebuildCategoryRows()
    -> void
{
    using namespace ck_insights_analyzer_tab;

    if (ck::Is_NOT_Valid(_CategoryRowsBox))
    {
        return;
    }
    _CategoryRowsBox->ClearChildren();

    for (const FCk_CategorySummaryEntry& Cat : _Categories)
    {
        _CategoryRowsBox->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 1.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0.0f, 0.0f, CkStyle::SpaceS, 0.0f)
            [
                MakeDot(CategoryColor(Cat.Name))
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(FText::FromString(Cat.Name))
                .Font(BodyFont())
                .ColorAndOpacity(CkStyle::Text())
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(CkStyle::SpaceS, 0.0f)
            [
                MakeProportionBar(Cat.PctOfFrame, CkStyle::OverlayOf(CategoryColor(Cat.Name), 0.85f), SideBarWidth)
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SNew(SBox)
                .WidthOverride(60.0f)
                .HAlign(HAlign_Right)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(FCk_TimerCategorizer::FormatMs(Cat.ExclusiveMs)))
                    .Font(BodyBoldFont())
                    .ColorAndOpacity(SeverityColor(Cat.ExclusiveMs))
                ]
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SNew(SBox)
                .WidthOverride(44.0f)
                .HAlign(HAlign_Right)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(FString::Printf(TEXT("%.0f%%"), Cat.PctOfFrame)))
                    .Font(SmallFont())
                    .ColorAndOpacity(CkStyle::TextMute())
                ]
            ]
        ];
    }
}

auto
    SCkInsightsAnalyzerTab::
    DoRebuildTopTimerRows()
    -> void
{
    using namespace ck_insights_analyzer_tab;

    if (ck::Is_NOT_Valid(_TopTimerRowsBox))
    {
        return;
    }
    _TopTimerRowsBox->ClearChildren();

    int32 Rank = 0;
    for (const FCk_TopTimerEntry& Timer : _TopTimers)
    {
        ++Rank;

        _TopTimerRowsBox->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 1.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0.0f, 0.0f, CkStyle::SpaceS, 0.0f)
            [
                SNew(SBox)
                .WidthOverride(22.0f)
                .HAlign(HAlign_Right)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(FString::Printf(TEXT("%d."), Rank)))
                    .Font(SmallFont())
                    .ColorAndOpacity(CkStyle::TextMute())
                ]
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(FText::FromString(Timer.Name))
                .Font(BodyFont())
                .ColorAndOpacity(CkStyle::Text())
                .OverflowPolicy(ETextOverflowPolicy::Ellipsis)
                .ToolTipText(FText::FromString(FString::Printf(
                    TEXT("%s\nExclusive: %s   Inclusive: %s   Count: %s"),
                    *Timer.Name,
                    *FCk_TimerCategorizer::FormatMs(Timer.ExclusiveMs),
                    *FCk_TimerCategorizer::FormatMs(Timer.InclusiveMs),
                    *FCk_TimerCategorizer::FormatCount(Timer.Count))))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SNew(SBox)
                .WidthOverride(60.0f)
                .HAlign(HAlign_Right)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(FCk_TimerCategorizer::FormatMs(Timer.ExclusiveMs)))
                    .Font(BodyBoldFont())
                    .ColorAndOpacity(SeverityColor(Timer.ExclusiveMs))
                ]
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SNew(SBox)
                .WidthOverride(52.0f)
                .HAlign(HAlign_Right)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString((Timer.Count > 1) ? FCk_TimerCategorizer::FormatCount(Timer.Count) : FString()))
                    .Font(SmallFont())
                    .ColorAndOpacity(CkStyle::TextMute())
                ]
            ]
        ];
    }
}

auto
    SCkInsightsAnalyzerTab::
    DoRebuildWorstFrameRows()
    -> void
{
    using namespace ck_insights_analyzer_tab;

    if (ck::Is_NOT_Valid(_WorstFrameRowsBox))
    {
        return;
    }
    _WorstFrameRowsBox->ClearChildren();

    for (const FCk_FrameSummary& Frame : _WorstFrames)
    {
        _WorstFrameRowsBox->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 1.0f)
        [
            SNew(SButton)
            .ButtonStyle(FAppStyle::Get(), "SimpleButton")
            .ContentPadding(FMargin(CkStyle::SpaceS, 2.0f))
            .ToolTipText(FText::FromString(TEXT("Analyze this frame (selects it in the chart)")))
            .OnClicked(this, &SCkInsightsAnalyzerTab::DoOnWorstFrameClicked, Frame.FrameIndex)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(SBox)
                    .WidthOverride(76.0f)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(FString::Printf(TEXT("#%s"), *FormatWithCommas(Frame.FrameIndex))))
                        .Font(BodyBoldFont())
                        .ColorAndOpacity(CkStyle::Accent())
                    ]
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(SBox)
                    .WidthOverride(64.0f)
                    .HAlign(HAlign_Right)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(FCk_TimerCategorizer::FormatMs(Frame.DurationMs)))
                        .Font(BodyBoldFont())
                        .ColorAndOpacity(FrameBudgetColor(Frame.DurationMs))
                    ]
                ]
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .VAlign(VAlign_Center)
                .Padding(CkStyle::SpaceM, 0.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(Frame.DominantCost))
                    .Font(ItalicFont())
                    .ColorAndOpacity(CkStyle::TextDim())
                    .OverflowPolicy(ETextOverflowPolicy::Ellipsis)
                ]
            ]
        ];
    }
}

auto
    SCkInsightsAnalyzerTab::
    DoRebuildCategoryAvgRows()
    -> void
{
    using namespace ck_insights_analyzer_tab;

    if (ck::Is_NOT_Valid(_CategoryAvgRowsBox))
    {
        return;
    }
    _CategoryAvgRowsBox->ClearChildren();

    for (const FCk_MultiFrameStats::FCategoryStats& Cat : _CategoryAverages)
    {
        _CategoryAvgRowsBox->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 1.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0.0f, 0.0f, CkStyle::SpaceS, 0.0f)
            [
                MakeDot(CategoryColor(Cat.Name))
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(FText::FromString(Cat.Name))
                .Font(BodyFont())
                .ColorAndOpacity(CkStyle::Text())
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(CkStyle::SpaceS, 0.0f)
            [
                MakeProportionBar(Cat.TotalPct, CkStyle::OverlayOf(CategoryColor(Cat.Name), 0.85f), SideBarWidth)
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SNew(SBox)
                .WidthOverride(64.0f)
                .HAlign(HAlign_Right)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(FString::Printf(TEXT("%s avg"), *FCk_TimerCategorizer::FormatMs(Cat.AvgExclMs))))
                    .Font(BodyBoldFont())
                    .ColorAndOpacity(SeverityColor(Cat.AvgExclMs))
                ]
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SNew(SBox)
                .WidthOverride(64.0f)
                .HAlign(HAlign_Right)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(FString::Printf(TEXT("%s p95"), *FCk_TimerCategorizer::FormatMs(Cat.P95ExclMs))))
                    .Font(SmallFont())
                    .ColorAndOpacity(CkStyle::TextMute())
                ]
            ]
        ];
    }
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
        DoSetStatus(TEXT("Loading in progress..."), ECk_Tone::Warn);
        return FReply::Handled();
    }

    const auto DesktopPlatform = FDesktopPlatformModule::Get();
    if (ck::Is_NOT_Valid(DesktopPlatform, ck::IsValid_Policy_NullptrOnly{}))
    {
        DoSetStatus(TEXT("Desktop platform not available."), ECk_Tone::Err);
        return FReply::Handled();
    }

    TArray<FString> OutFiles;
    const bool Opened = DesktopPlatform->OpenFileDialog(
        FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
        TEXT("Open Unreal Insights Trace"),
        FPaths::ProjectSavedDir() / TEXT("TraceSessions"),
        TEXT(""),
        TEXT("Unreal Trace Files (*.utrace)|*.utrace"),
        EFileDialogFlags::None,
        OutFiles);

    if (NOT Opened || OutFiles.Num() == 0)
    {
        return FReply::Handled();
    }

    // Close any existing session
    _Session.Close();
    _FrameBarChart->ClearFrameData();
    DoClearResults();

    DoStartAsyncOpen(OutFiles[0]);
    return FReply::Handled();
}

auto
    SCkInsightsAnalyzerTab::
    DoOnAnalyzeWorstClicked()
    -> FReply
{
    using namespace ck_insights_analyzer_tab;

    if (NOT _Session.IsOpen() || DoIsLoading())
    {
        DoSetStatus(TEXT("No trace loaded. Open a .utrace file first."), ECk_Tone::Warn);
        return FReply::Handled();
    }

    DoSetStatus(TEXT("Analyzing worst 10 frames..."), ECk_Tone::Info);

    FCk_MultiFrameReportConfig Config;
    Config.TargetFrameMs = TargetFrameMs;
    Config.Depth = _ReportDepth;
    Config.ApplyDepth();
    Config.WorstFrameCount = 10; // Override depth's default worst count

    FCk_MultiFrameReport MultiReport(Config);
    DoSetReport(MultiReport.AnalyzeWorstFrames(_Session, 10));
    DoPopulateMultiFrame(MultiReport.GetStats());

    DoSetStatus(TEXT("Worst 10 frames analysis complete. Click a Worst Frame row to drill in."), ECk_Tone::Ok);

    return FReply::Handled();
}

auto
    SCkInsightsAnalyzerTab::
    DoOnCopyToClipboardClicked()
    -> FReply
{
    if (_CurrentReport.IsEmpty())
    {
        DoSetStatus(TEXT("No report to copy."), ECk_Tone::Warn);
        return FReply::Handled();
    }

    FPlatformApplicationMisc::ClipboardCopy(*_CurrentReport);
    DoSetStatus(TEXT("Report copied to clipboard."), ECk_Tone::Ok);

    return FReply::Handled();
}

auto
    SCkInsightsAnalyzerTab::
    DoOnWorstFrameClicked(uint64 FrameIndex)
    -> FReply
{
    if (NOT _Session.IsOpen() || DoIsLoading())
    {
        return FReply::Handled();
    }

    if (ck::IsValid(_FrameBarChart))
    {
        _FrameBarChart->SetSelection(FrameIndex, FrameIndex);
    }
    DoAnalyzeSingleFrame(FrameIndex);

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
    if (ck::Is_NOT_Valid(NewValue)) return;

    if (*NewValue == TEXT("Full"))           _ReportDepth = ECkReportDepth::Full;
    else if (*NewValue == TEXT("Standard"))  _ReportDepth = ECkReportDepth::Standard;
    else if (*NewValue == TEXT("Concise"))   _ReportDepth = ECkReportDepth::Concise;
    else                                    _ReportDepth = ECkReportDepth::HotPathsOnly;

    // Re-run the current selection so the depth change is immediately visible
    DoRerunCurrentSelection();
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
    if (NOT _Session.IsOpen() || _LoadingState == ELoadingState::Opening) return;

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
    DoSetStatus(const FString& Text, ECk_Tone InTone)
    -> void
{
    _StatusTone = InTone;
    if (ck::IsValid(_StatusText))
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
    if (ck::IsValid(_ReportText))
    {
        _ReportText->SetText(FText::FromString(ReportText));
    }
}

auto
    SCkInsightsAnalyzerTab::
    DoClearResults()
    -> void
{
    DoSetReport(TEXT(""));

    _ResultsMode = EResultsMode::None;
    _AnalyzedFrameMs = 0.0;

    _HotPathRoots.Reset();
    _Categories.Reset();
    _TopTimers.Reset();
    _WorstFrames.Reset();
    _CategoryAverages.Reset();

    if (ck::IsValid(_HotPathTree)) { _HotPathTree->RequestTreeRefresh(); }
    DoRebuildCategoryRows();
    DoRebuildTopTimerRows();
    DoRebuildWorstFrameRows();
    DoRebuildCategoryAvgRows();

    if (ck::IsValid(_SummaryBox)) { _SummaryBox->ClearChildren(); }
}

auto
    SCkInsightsAnalyzerTab::
    DoAnalyzeSingleFrame(uint64 FrameIndex)
    -> void
{
    using namespace ck_insights_analyzer_tab;

    DoSetStatus(FString::Printf(TEXT("Analyzing frame %llu..."), FrameIndex), ECk_Tone::Info);

    FCk_FrameAnalysisResult Result = FCk_FrameAnalyzer::AnalyzeFrame(_Session, FrameIndex);
    if (NOT Result.IsValid())
    {
        DoSetStatus(FString::Printf(TEXT("Frame %llu produced no analysis data."), FrameIndex), ECk_Tone::Warn);

        _ResultsMode = EResultsMode::None;
        _AnalyzedFrameMs = 0.0;
        _HotPathRoots.Reset();
        _Categories.Reset();
        _TopTimers.Reset();
        if (ck::IsValid(_HotPathTree)) { _HotPathTree->RequestTreeRefresh(); }
        DoRebuildCategoryRows();
        DoRebuildTopTimerRows();
        DoSetReport(TEXT(""));
        return;
    }

    FCk_FrameReportConfig Config;
    Config.TargetFrameMs = TargetFrameMs;
    Config.Depth = _ReportDepth;
    Config.ApplyDepth();

    FCk_FrameReport FrameReport(Config);
    DoSetReport(FrameReport.Generate(_Session, Result));

    // Structured views
    _ResultsMode = EResultsMode::SingleFrame;
    _AnalyzedFrameMs = Result.FrameDurationMs;
    _HotPathRoots = FrameReport.BuildHotPathTree(_Session, Result);

    {
        TraceServices::FAnalysisSessionReadScope ReadScope = _Session.CreateReadScope();
        const FCk_FrameReport::FTimerNameMap TimerNames = FCk_FrameReport::BuildTimerNameMap(_Session);
        _Categories = FrameReport.ComputeCategorySummary(Result, TimerNames);
        _TopTimers = FrameReport.ComputeTopTimers(Result, TimerNames, TopTimerCount);
    }

    if (ck::IsValid(_HotPathTree)) { _HotPathTree->RequestTreeRefresh(); }
    DoExpandHotPathDefaults();
    DoRebuildCategoryRows();
    DoRebuildTopTimerRows();
    DoRebuildSummaryStrip_SingleFrame(Result);

    DoSetStatus(FString::Printf(TEXT("Frame %llu: %.2fms"), FrameIndex, Result.FrameDurationMs),
        FrameBudgetTone(Result.FrameDurationMs));
}

auto
    SCkInsightsAnalyzerTab::
    DoAnalyzeFrameRange(uint64 StartFrame, uint64 EndFrame)
    -> void
{
    using namespace ck_insights_analyzer_tab;

    const uint64 Count = EndFrame - StartFrame + 1;
    DoSetStatus(FString::Printf(TEXT("Analyzing frames %llu-%llu (%llu frames)..."), StartFrame, EndFrame, Count),
        ECk_Tone::Info);

    FCk_MultiFrameReportConfig Config;
    Config.TargetFrameMs = TargetFrameMs;
    Config.Depth = _ReportDepth;
    Config.ApplyDepth();

    FCk_MultiFrameReport MultiReport(Config);
    // EndFrame is inclusive from the chart, but AnalyzeAndGenerate expects exclusive
    DoSetReport(MultiReport.AnalyzeAndGenerate(_Session, StartFrame, EndFrame + 1));
    DoPopulateMultiFrame(MultiReport.GetStats());

    DoSetStatus(FString::Printf(TEXT("Analyzed frames %llu-%llu (%llu frames)"), StartFrame, EndFrame, Count),
        ECk_Tone::Ok);
}

auto
    SCkInsightsAnalyzerTab::
    DoRerunCurrentSelection()
    -> void
{
    if (NOT _Session.IsOpen() || DoIsLoading() ||
        ck::Is_NOT_Valid(_FrameBarChart) || NOT _FrameBarChart->HasSelection())
    {
        return;
    }

    DoOnFrameSelectionChanged(_FrameBarChart->GetSelectionStart(), _FrameBarChart->GetSelectionEnd());
}

auto
    SCkInsightsAnalyzerTab::
    DoPopulateMultiFrame(const FCk_MultiFrameStats& Stats)
    -> void
{
    _ResultsMode = EResultsMode::MultiFrame;
    _AnalyzedFrameMs = 0.0;

    // Multi-frame analysis has no single-frame tree/categories
    _HotPathRoots.Reset();
    _Categories.Reset();
    _TopTimers.Reset();

    _WorstFrames = Stats.WorstFrames;
    _CategoryAverages = Stats.CategoryAverages;

    if (ck::IsValid(_HotPathTree)) { _HotPathTree->RequestTreeRefresh(); }
    DoRebuildCategoryRows();
    DoRebuildTopTimerRows();
    DoRebuildWorstFrameRows();
    DoRebuildCategoryAvgRows();
    DoRebuildSummaryStrip_MultiFrame(Stats);
}

// --------------------------------------------------------------------------------------------------------------------
// Summary strip
// --------------------------------------------------------------------------------------------------------------------

auto
    SCkInsightsAnalyzerTab::
    DoAddSummaryTile(const FString& InLabel, const FString& InValue, const FLinearColor& InValueColor)
    -> void
{
    using namespace ck_insights_analyzer_tab;

    if (ck::Is_NOT_Valid(_SummaryBox))
    {
        return;
    }

    _SummaryBox->AddSlot()
    .AutoWidth()
    .Padding(0.0f, 0.0f, CkStyle::SpaceM, 0.0f)
    [
        MakeStatTile(InLabel, InValue, InValueColor)
    ];
}

auto
    SCkInsightsAnalyzerTab::
    DoRebuildSummaryStrip_TraceInfo()
    -> void
{
    using namespace ck_insights_analyzer_tab;

    if (ck::Is_NOT_Valid(_SummaryBox))
    {
        return;
    }
    _SummaryBox->ClearChildren();

    if (NOT _Session.IsOpen())
    {
        return;
    }

    DoAddSummaryTile(TEXT("Trace"), FPaths::GetCleanFilename(_Session.GetFilePath()), CkStyle::Text());
    DoAddSummaryTile(TEXT("Frames"), FormatWithCommas(_Session.GetFrameCount()), CkStyle::Text());
    DoAddSummaryTile(TEXT("Duration"), FString::Printf(TEXT("%.1fs"), _Session.GetDurationSeconds()), CkStyle::Text());
    DoAddSummaryTile(TEXT("Budget"), FString::Printf(TEXT("%.1fms"), TargetFrameMs), CkStyle::TextDim());
}

auto
    SCkInsightsAnalyzerTab::
    DoRebuildSummaryStrip_SingleFrame(const FCk_FrameAnalysisResult& Result)
    -> void
{
    using namespace ck_insights_analyzer_tab;

    DoRebuildSummaryStrip_TraceInfo();

    const double FrameMs = Result.FrameDurationMs;
    const double OverBudget = FrameMs / TargetFrameMs;

    DoAddSummaryTile(TEXT("Frame"), FString::Printf(TEXT("#%s"), *FormatWithCommas(Result.FrameIndex)), CkStyle::Accent());
    DoAddSummaryTile(TEXT("Frame Time"), FString::Printf(TEXT("%.2fms"), FrameMs), FrameBudgetColor(FrameMs));
    DoAddSummaryTile(TEXT("Vs Budget"), FString::Printf(TEXT("%.1fx"), OverBudget), FrameBudgetColor(FrameMs));
}

auto
    SCkInsightsAnalyzerTab::
    DoRebuildSummaryStrip_MultiFrame(const FCk_MultiFrameStats& Stats)
    -> void
{
    using namespace ck_insights_analyzer_tab;

    DoRebuildSummaryStrip_TraceInfo();

    DoAddSummaryTile(TEXT("Analyzed"), FormatWithCommas(Stats.FrameCount), CkStyle::Accent());
    DoAddSummaryTile(TEXT("Avg"), FString::Printf(TEXT("%.2fms"), Stats.AvgFrameMs), FrameBudgetColor(Stats.AvgFrameMs));
    DoAddSummaryTile(TEXT("P95"), FString::Printf(TEXT("%.2fms"), Stats.P95FrameMs), FrameBudgetColor(Stats.P95FrameMs));
    DoAddSummaryTile(TEXT("P99"), FString::Printf(TEXT("%.2fms"), Stats.P99FrameMs), FrameBudgetColor(Stats.P99FrameMs));
    DoAddSummaryTile(TEXT("Max"), FString::Printf(TEXT("%.2fms"), Stats.MaxFrameMs), FrameBudgetColor(Stats.MaxFrameMs));
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

    DoSetStatus(FString::Printf(TEXT("Opening: %s ..."), *FPaths::GetCleanFilename(TracePath)), ECk_Tone::Info);

    // Prepare analysis service on game thread (FModuleManager is not thread-safe)
    _PendingSession = MakeShared<FCk_TraceSession>();
    if (NOT _PendingSession->PrepareAnalysisService())
    {
        DoSetStatus(TEXT("Failed to create analysis service."), ECk_Tone::Err);
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
        if (NOT _OpenFuture.IsReady())
        {
            return true; // keep ticking
        }

        const bool Success = _OpenFuture.Get();
        if (NOT Success)
        {
            DoSetStatus(FString::Printf(TEXT("Failed to open: %s"),
                *FPaths::GetCleanFilename(_PendingTracePath)), ECk_Tone::Err);
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
            TEXT("Reading frames: 0 / %llu ..."), _TotalFrameCount), ECk_Tone::Info);

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
        const auto Frame = _Session.GetFrame(i);
        if (ck::IsValid(Frame, ck::IsValid_Policy_NullptrOnly{}))
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
    if (ck::IsValid(_FrameBarChart))
    {
        _FrameBarChart->AppendFrameData(ChunkDurations);
    }

    _LoadedFrameCount = EndFrame;

    DoSetStatus(FString::Printf(
        TEXT("Reading frames: %llu / %llu ..."),
        _LoadedFrameCount, _TotalFrameCount), ECk_Tone::Info);

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
    if (ck::IsValid(_FrameBarChart))
    {
        _FrameBarChart->RecalculateDisplayMax();
    }
    _PendingFrameDurations.Reset();

    const double DurationSec = _Session.GetDurationSeconds();
    DoSetStatus(FString::Printf(
        TEXT("Loaded: %s  |  %llu frames  |  %.1fs duration"),
        *FPaths::GetCleanFilename(_PendingTracePath), _TotalFrameCount, DurationSec), ECk_Tone::Ok);

    DoRebuildSummaryStrip_TraceInfo();

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
