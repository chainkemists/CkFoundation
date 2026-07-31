#include "CkPathNetworkEditor/Designer/CkPathNetwork_DesignerToolkit.h"

#include "CkPathNetworkEditor/Designer/CkPathNetwork_DesignerEdMode.h"
#include "CkPathNetworkEditor/Designer/CkPathNetwork_DesignerSession.h"

#include "CkEditorTools/Style/CkStyle.h"

#include "CkCore/Validation/CkIsValid.h"

#include <DetailLayoutBuilder.h>
#include <Editor.h>
#include <IDetailsView.h>
#include <Modules/ModuleManager.h>
#include <PropertyEditorModule.h>
#include <Styling/AppStyle.h>
#include <Widgets/Input/SButton.h>
#include <Widgets/Input/SComboBox.h>
#include <Widgets/Input/SEditableTextBox.h>
#include <Widgets/Layout/SBorder.h>
#include <Widgets/Layout/SBox.h>
#include <Widgets/Layout/SScrollBox.h>
#include <Widgets/Layout/SSeparator.h>
#include <Widgets/Layout/SUniformGridPanel.h>
#include <Widgets/SBoxPanel.h>
#include <Widgets/Text/STextBlock.h>

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "Ck_PathNetworkDesigner_Toolkit"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork_editor::designer::toolkit_style
{
    auto
    Make_Card(
        const FText& InTitle,
        TSharedRef<SWidget> InBody) -> TSharedRef<SWidget>
    {
        return SNew(SBorder)
            .BorderImage(CkStyle::GetRoundedBrush_Large())
            .BorderBackgroundColor(FSlateColor{CkStyle::Border()})
            .Padding(1.0f)
            [
                SNew(SBorder)
                .BorderImage(CkStyle::GetRoundedBrush_Large())
                .BorderBackgroundColor(FSlateColor{CkStyle::Bg2()})
                .Padding(CkStyle::SpaceL)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, CkStyle::SpaceM)
                    [
                        SNew(STextBlock)
                        .Text(InTitle)
                        .Font(CkStyle::BoldFont(CkStyle::FontSizeSmall()))
                        .ColorAndOpacity(FSlateColor{CkStyle::TextStrong()})
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        InBody
                    ]
                ]
            ];
    }

    auto
    Make_ActionButton(
        const FText& InLabel,
        const FText& InTooltip,
        const FOnClicked& InOnClicked,
        const TAttribute<bool>& InIsEnabled,
        const FLinearColor& InTint) -> TSharedRef<SWidget>
    {
        return SNew(SButton)
            .ContentPadding(FMargin{CkStyle::SpaceL, CkStyle::SpaceM})
            .ButtonColorAndOpacity(FSlateColor{InTint})
            .ToolTipText(InTooltip)
            .OnClicked(InOnClicked)
            .IsEnabled(InIsEnabled)
            [
                SNew(STextBlock)
                .Text(InLabel)
                .Justification(ETextJustify::Center)
                .Font(CkStyle::BoldFont(CkStyle::FontSizeBody()))
                .ColorAndOpacity(FSlateColor{CkStyle::TextStrong()})
            ];
    }
}

// --------------------------------------------------------------------------------------------------------------------

FCk_PathNetworkDesigner_Toolkit::
    ~FCk_PathNetworkDesigner_Toolkit()
{
    if (GEditor != nullptr)
    { GEditor->UnregisterForUndo(this); }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_PathNetworkDesigner_Toolkit::
    Init(
        const TSharedPtr<IToolkitHost>& InToolkitHost,
        TWeakObjectPtr<UEdMode> InOwningMode)
    -> void
{
    _OwningMode = InOwningMode;

    for (auto Preset : ck::pathnetwork_editor::designer::Get_Presets())
    { _PresetItems.Add(MakeShared<ck::pathnetwork_editor::designer::FPreset>(MoveTemp(Preset))); }

    if (const auto* Session = Get_Session())
    {
        const auto* SelectedPreset = _PresetItems.FindByPredicate(
            [&](const TSharedPtr<ck::pathnetwork_editor::designer::FPreset>& InPreset)
            {
                return InPreset.IsValid()
                    && InPreset->_Owner == Session->Get_ActivePresetOwner()
                    && InPreset->_Id == Session->Get_ActivePresetId();
            });
        if (SelectedPreset != nullptr)
        { _SelectedPreset = *SelectedPreset; }
    }

    auto& PropertyEditor =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
    auto DetailsArgs = FDetailsViewArgs{};
    DetailsArgs.bAllowSearch = true;
    DetailsArgs.bHideSelectionTip = true;
    DetailsArgs.bLockable = false;
    DetailsArgs.bShowOptions = false;
    DetailsArgs.bShowScrollBar = false;
    DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    _DetailsView = PropertyEditor.CreateDetailView(DetailsArgs);
    _DetailsView->SetObject(Get_Session());
    _DetailsView->OnFinishedChangingProperties().AddSP(
        this, &FCk_PathNetworkDesigner_Toolkit::On_ConfigurationChanged);
    Refresh_RouteWatchItems();

    namespace style = ck::pathnetwork_editor::designer::toolkit_style;

    const auto PresetCardBody =
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SComboBox<TSharedPtr<ck::pathnetwork_editor::designer::FPreset>>)
            .OptionsSource(&_PresetItems)
            .InitiallySelectedItem(_SelectedPreset)
            .OnGenerateWidget(this, &FCk_PathNetworkDesigner_Toolkit::Make_PresetRow)
            .OnSelectionChanged(this, &FCk_PathNetworkDesigner_Toolkit::On_PresetSelected)
            [
                SNew(STextBlock)
                .Text(this, &FCk_PathNetworkDesigner_Toolkit::Get_ActivePresetText)
                .Font(CkStyle::BoldFont(CkStyle::FontSizeBody()))
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, CkStyle::SpaceM, 0.0f, 0.0f)
        [
            SNew(STextBlock)
            .Text(this, &FCk_PathNetworkDesigner_Toolkit::Get_DetectorText)
            .ColorAndOpacity(FSlateColor{CkStyle::TextDim()})
            .AutoWrapText(true)
        ];

    const auto TargetCardBody =
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(STextBlock)
            .Text(this, &FCk_PathNetworkDesigner_Toolkit::Get_TargetLevelText)
            .Font(CkStyle::MonoFont(CkStyle::FontSizeSmall()))
            .ColorAndOpacity(FSlateColor{CkStyle::Info()})
            .AutoWrapText(true)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, CkStyle::SpaceM, 0.0f, 0.0f)
        [
            SNew(SUniformGridPanel)
            .SlotPadding(FMargin{CkStyle::SpaceXS})
            + SUniformGridPanel::Slot(0, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("UseCurrentLevel", "Use Current Level"))
                .ToolTipText(LOCTEXT(
                    "UseCurrentLevelTooltip",
                    "Apply into the editor world's current level. Nothing is created until Apply."))
                .OnClicked(this, &FCk_PathNetworkDesigner_Toolkit::On_UseCurrentLevel)
            ]
            + SUniformGridPanel::Slot(1, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("LoadSelectedNetwork", "Load Selected Network"))
                .ToolTipText(LOCTEXT(
                    "LoadSelectedNetworkTooltip",
                    "Load one selected Ck Path Network actor for editing and preview."))
                .OnClicked(this, &FCk_PathNetworkDesigner_Toolkit::On_LoadSelectedNetwork)
            ]
            + SUniformGridPanel::Slot(0, 1)
            [
                SNew(SButton)
                .Text(LOCTEXT("FitLoadedWorld", "Fit Loaded World"))
                .ToolTipText(LOCTEXT(
                    "FitLoadedWorldTooltip",
                    "Fit detection bounds around component bounds in all currently loaded editor levels."))
                .OnClicked(this, &FCk_PathNetworkDesigner_Toolkit::On_FitLoadedWorld)
            ]
            + SUniformGridPanel::Slot(1, 1)
            [
                SNew(SButton)
                .Text(LOCTEXT("FitSelection", "Fit Selection"))
                .ToolTipText(LOCTEXT(
                    "FitSelectionTooltip",
                    "Fit detection bounds to selected non-path-network actors."))
                .OnClicked(this, &FCk_PathNetworkDesigner_Toolkit::On_FitSelection)
            ]
        ];

    const auto RouteCardBody =
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(STextBlock)
            .Text(LOCTEXT(
                "RoutePreviewHelp",
                "Save named route watches on the level path network, then refresh one or all of them to spot routing regressions. Route watches are stored with the map. Apply or load a path network before adding them."))
            .ColorAndOpacity(FSlateColor{CkStyle::TextDim()})
            .AutoWrapText(true)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, CkStyle::SpaceM, 0.0f, 0.0f)
        [
            SAssignNew(
                _RouteWatchComboBox,
                SComboBox<TSharedPtr<FName>>)
            .OptionsSource(&_RouteWatchItems)
            .InitiallySelectedItem(_SelectedRouteWatchItem)
            .OnGenerateWidget(
                this,
                &FCk_PathNetworkDesigner_Toolkit::Make_RouteWatchRow)
            .OnSelectionChanged(
                this,
                &FCk_PathNetworkDesigner_Toolkit::On_RouteWatchSelected)
            [
                SNew(STextBlock)
                .Text(
                    this,
                    &FCk_PathNetworkDesigner_Toolkit::
                        Get_ActiveRouteWatchText)
                .Font(CkStyle::BoldFont(CkStyle::FontSizeBody()))
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, CkStyle::SpaceXS, 0.0f, 0.0f)
        [
            SAssignNew(_RouteWatchNameTextBox, SEditableTextBox)
            .Text(_SelectedRouteWatchItem.IsValid()
                ? FText::FromName(*_SelectedRouteWatchItem)
                : FText::GetEmpty())
            .HintText(LOCTEXT(
                "RouteWatchNameHint",
                "Route watch name"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, CkStyle::SpaceXS, 0.0f, 0.0f)
        [
            SNew(SUniformGridPanel)
            .SlotPadding(FMargin{CkStyle::SpaceXS})
            + SUniformGridPanel::Slot(0, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("AddRouteWatch", "Add"))
                .ToolTipText(LOCTEXT(
                    "AddRouteWatchTooltip",
                    "Save the current start and goal as a new named route watch on the level path network."))
                .OnClicked(
                    this,
                    &FCk_PathNetworkDesigner_Toolkit::On_AddRouteWatch)
                .IsEnabled(
                    this,
                    &FCk_PathNetworkDesigner_Toolkit::
                        Get_CanAddRouteWatch)
            ]
            + SUniformGridPanel::Slot(1, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("SaveRouteWatch", "Save"))
                .ToolTipText(LOCTEXT(
                    "SaveRouteWatchTooltip",
                    "Update the selected route watch with the current name, start, and goal."))
                .OnClicked(
                    this,
                    &FCk_PathNetworkDesigner_Toolkit::
                        On_SaveRouteWatch)
                .IsEnabled(
                    this,
                    &FCk_PathNetworkDesigner_Toolkit::
                        Get_CanSaveRouteWatch)
            ]
            + SUniformGridPanel::Slot(2, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("RemoveRouteWatch", "Remove"))
                .ToolTipText(LOCTEXT(
                    "RemoveRouteWatchTooltip",
                    "Remove the selected saved route watch."))
                .OnClicked(
                    this,
                    &FCk_PathNetworkDesigner_Toolkit::
                        On_RemoveRouteWatch)
                .IsEnabled(
                    this,
                    &FCk_PathNetworkDesigner_Toolkit::
                        Get_CanSaveRouteWatch)
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, CkStyle::SpaceM, 0.0f, 0.0f)
        [
            SNew(SUniformGridPanel)
            .SlotPadding(FMargin{CkStyle::SpaceXS})
            + SUniformGridPanel::Slot(0, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("CaptureRouteStart", "Capture Start"))
                .ToolTipText(LOCTEXT(
                    "CaptureRouteStartTooltip",
                    "Use the location of the one selected editor actor as the representative route start."))
                .OnClicked(this, &FCk_PathNetworkDesigner_Toolkit::On_CaptureRouteStart)
            ]
            + SUniformGridPanel::Slot(1, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("CaptureRouteGoal", "Capture Goal"))
                .ToolTipText(LOCTEXT(
                    "CaptureRouteGoalTooltip",
                    "Use the location of the one selected editor actor as the representative route goal."))
                .OnClicked(this, &FCk_PathNetworkDesigner_Toolkit::On_CaptureRouteGoal)
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, CkStyle::SpaceM, 0.0f, 0.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                style::Make_ActionButton(
                    LOCTEXT(
                        "PreviewRouteButton",
                        "REFRESH SELECTED"),
                    LOCTEXT(
                        "PreviewRouteButtonTooltip",
                        "Replan the working route against the cached detector preview. The level remains unchanged."),
                    FOnClicked::CreateSP(
                        this,
                        &FCk_PathNetworkDesigner_Toolkit::
                            On_PreviewRoute),
                    TAttribute<bool>::CreateSP(
                        this,
                        &FCk_PathNetworkDesigner_Toolkit::
                            Get_CanPreviewRoute),
                    CkStyle::InfoDim())
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(CkStyle::SpaceM, 0.0f, 0.0f, 0.0f)
            [
                style::Make_ActionButton(
                    LOCTEXT(
                        "PreviewAllRouteWatchesButton",
                        "REFRESH ALL"),
                    LOCTEXT(
                        "PreviewAllRouteWatchesButtonTooltip",
                        "Replan every saved route watch and draw all results in the viewport."),
                    FOnClicked::CreateSP(
                        this,
                        &FCk_PathNetworkDesigner_Toolkit::
                            On_RefreshAllRouteWatches),
                    TAttribute<bool>::CreateSP(
                        this,
                        &FCk_PathNetworkDesigner_Toolkit::
                            Get_CanRefreshAllRouteWatches),
                    CkStyle::AccentDim())
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, CkStyle::SpaceM, 0.0f, 0.0f)
        [
            SNew(STextBlock)
            .Text(
                this,
                &FCk_PathNetworkDesigner_Toolkit::
                    Get_RouteWatchSummaryText)
            .Font(CkStyle::MonoFont(CkStyle::FontSizeSmall()))
            .ColorAndOpacity(FSlateColor{CkStyle::TextDim()})
            .AutoWrapText(true)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, CkStyle::SpaceM, 0.0f, 0.0f)
        [
            SNew(STextBlock)
            .Text(this, &FCk_PathNetworkDesigner_Toolkit::Get_RoutePreviewText)
            .Font(CkStyle::MonoFont(CkStyle::FontSizeSmall()))
            .ColorAndOpacity(this, &FCk_PathNetworkDesigner_Toolkit::Get_RoutePreviewColor)
            .AutoWrapText(true)
        ];

    const auto ConnectivityCardBody =
        SNew(STextBlock)
        .Text(this, &FCk_PathNetworkDesigner_Toolkit::Get_ConnectivityText)
        .Font(CkStyle::MonoFont(CkStyle::FontSizeSmall()))
        .ColorAndOpacity(this, &FCk_PathNetworkDesigner_Toolkit::Get_ConnectivityColor)
        .AutoWrapText(true);

    const auto StatusBody =
        SNew(SBorder)
        .BorderImage(CkStyle::GetRoundedBrush())
        .BorderBackgroundColor(this, &FCk_PathNetworkDesigner_Toolkit::Get_StatusBackgroundColor)
        .Padding(CkStyle::SpaceM)
        [
            SNew(STextBlock)
            .Text(this, &FCk_PathNetworkDesigner_Toolkit::Get_StatusText)
            .ColorAndOpacity(this, &FCk_PathNetworkDesigner_Toolkit::Get_StatusColor)
            .AutoWrapText(true)
        ];

    SAssignNew(_InlineContent, SBorder)
        .BorderImage(CkStyle::GetFilledBrush())
        .BorderBackgroundColor(FSlateColor{CkStyle::BgRoot()})
        .Padding(CkStyle::SpaceM)
        [
            SNew(SScrollBox)
            + SScrollBox::Slot()
            .Padding(0.0f, 0.0f, 0.0f, CkStyle::SpaceL)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("HeroTitle", "CK PATH NETWORK DESIGNER"))
                    .Font(CkStyle::BoldFont(CkStyle::FontSizeH2()))
                    .ColorAndOpacity(FSlateColor{CkStyle::Accent()})
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, CkStyle::SpaceXS, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT(
                        "HeroSubtitle",
                        "Detect, tune, test, and apply reusable navigation ribbons entirely in the editor."))
                    .ColorAndOpacity(FSlateColor{CkStyle::TextDim()})
                    .AutoWrapText(true)
                ]
            ]
            + SScrollBox::Slot()
            .Padding(0.0f, 0.0f, 0.0f, CkStyle::SpaceM)
            [
                style::Make_Card(LOCTEXT("PresetCard", "WORKFLOW"), PresetCardBody)
            ]
            + SScrollBox::Slot()
            .Padding(0.0f, 0.0f, 0.0f, CkStyle::SpaceM)
            [
                style::Make_Card(LOCTEXT("TargetCard", "TARGET & BOUNDS"), TargetCardBody)
            ]
            + SScrollBox::Slot()
            .Padding(0.0f, 0.0f, 0.0f, CkStyle::SpaceM)
            [
                style::Make_Card(
                    LOCTEXT("OptionsCard", "DETECTION, GENERATION, ROUTE TUNING & OVERLAY"),
                    _DetailsView.ToSharedRef())
            ]
            + SScrollBox::Slot()
            .Padding(0.0f, 0.0f, 0.0f, CkStyle::SpaceM)
            [
                style::Make_Card(
                    LOCTEXT("ConnectivityCard", "CONNECTIVITY (PREVIEW)"),
                    ConnectivityCardBody)
            ]
            + SScrollBox::Slot()
            .Padding(0.0f, 0.0f, 0.0f, CkStyle::SpaceM)
            [
                style::Make_Card(
                    LOCTEXT("RouteCard", "ROUTE WATCHLIST"),
                    RouteCardBody)
            ]
            + SScrollBox::Slot()
            .Padding(0.0f, 0.0f, 0.0f, CkStyle::SpaceM)
            [
                StatusBody
            ]
            + SScrollBox::Slot()
            .Padding(0.0f, 0.0f, 0.0f, CkStyle::SpaceM)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    style::Make_ActionButton(
                        LOCTEXT("PreviewButton", "PREVIEW"),
                        LOCTEXT(
                            "PreviewButtonTooltip",
                            "Run the detector and draw its mask/ribbons without changing the level."),
                        FOnClicked::CreateSP(this, &FCk_PathNetworkDesigner_Toolkit::On_Preview),
                        TAttribute<bool>::CreateSP(this, &FCk_PathNetworkDesigner_Toolkit::Get_CanPreview),
                        CkStyle::InfoDim())
                ]
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(CkStyle::SpaceM, 0.0f, 0.0f, 0.0f)
                [
                    style::Make_ActionButton(
                        LOCTEXT("ApplyButton", "APPLY"),
                        LOCTEXT(
                            "ApplyButtonTooltip",
                            "Create or update the target-level path network in one undoable transaction."),
                        FOnClicked::CreateSP(this, &FCk_PathNetworkDesigner_Toolkit::On_Apply),
                        TAttribute<bool>::CreateSP(this, &FCk_PathNetworkDesigner_Toolkit::Get_CanApply),
                        CkStyle::AccentDim())
                ]
            ]
            + SScrollBox::Slot()
            [
                SNew(SButton)
                .Text(LOCTEXT("ClearPreview", "Clear Preview"))
                .HAlign(HAlign_Center)
                .ToolTipText(LOCTEXT(
                    "ClearPreviewTooltip",
                    "Remove transient preview geometry. Applied path-network data is unchanged."))
                .OnClicked(this, &FCk_PathNetworkDesigner_Toolkit::On_ClearPreview)
            ]
        ];

    FModeToolkit::Init(InToolkitHost, InOwningMode);
    if (GEditor != nullptr)
    { GEditor->RegisterForUndo(this); }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_PathNetworkDesigner_Toolkit::
    GetToolPaletteNames(
        TArray<FName>& OutPaletteNames) const
    -> void
{
    OutPaletteNames.Add(TEXT("Path Network"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_PathNetworkDesigner_Toolkit::
    PostUndo(
        const bool InSuccess)
    -> void
{
    if (NOT InSuccess)
    { return; }

    if (auto* Session = Get_Session())
    { Session->Synchronize_RouteWatchesFromActor(); }
    Refresh_RouteWatchItems();
    Refresh_Details();
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    PostRedo(
        const bool InSuccess)
    -> void
{
    PostUndo(InSuccess);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_PathNetworkDesigner_Toolkit::
    GetToolkitFName() const
    -> FName
{
    return TEXT("CkPathNetworkDesigner");
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    GetBaseToolkitName() const
    -> FText
{
    return LOCTEXT("ToolkitName", "Ck Path Network Designer");
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    GetInlineContent() const
    -> TSharedPtr<SWidget>
{
    return _InlineContent;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_PathNetworkDesigner_Toolkit::
    Get_Mode() const
    -> UCk_PathNetworkDesigner_EdMode*
{
    return Cast<UCk_PathNetworkDesigner_EdMode>(_OwningMode.Get());
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    Get_Session() const
    -> UCk_PathNetworkDesigner_Session_UE*
{
    const auto* Mode = Get_Mode();
    return Mode != nullptr ? Mode->Get_Session() : nullptr;
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    Refresh_Details()
    -> void
{
    if (_DetailsView.IsValid())
    {
        _DetailsView->SetObject(Get_Session(), true);
        _DetailsView->ForceRefresh();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_PathNetworkDesigner_Toolkit::
    Refresh_RouteWatchItems()
    -> void
{
    _RouteWatchItems.Reset();
    _SelectedRouteWatchItem.Reset();

    const auto* Session = Get_Session();
    if (Session != nullptr)
    {
        for (const auto Name : Session->Get_RouteWatchNames())
        { _RouteWatchItems.Add(MakeShared<FName>(Name)); }

        const auto ActiveIndex = Session->Get_ActiveRouteWatchIndex();
        if (_RouteWatchItems.IsValidIndex(ActiveIndex))
        { _SelectedRouteWatchItem = _RouteWatchItems[ActiveIndex]; }
    }

    if (_RouteWatchComboBox.IsValid())
    {
        _RouteWatchComboBox->RefreshOptions();
        if (_SelectedRouteWatchItem.IsValid())
        { _RouteWatchComboBox->SetSelectedItem(_SelectedRouteWatchItem); }
        else
        { _RouteWatchComboBox->ClearSelection(); }
    }

    if (_RouteWatchNameTextBox.IsValid())
    {
        _RouteWatchNameTextBox->SetText(
            _SelectedRouteWatchItem.IsValid()
                ? FText::FromName(*_SelectedRouteWatchItem)
                : FText::GetEmpty());
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto FCk_PathNetworkDesigner_Toolkit::On_UseCurrentLevel() -> FReply
{
    if (auto* Session = Get_Session())
    { Session->Use_CurrentLevel(); }
    Refresh_RouteWatchItems();
    Refresh_Details();
    return FReply::Handled();
}

auto FCk_PathNetworkDesigner_Toolkit::On_FitLoadedWorld() -> FReply
{
    if (auto* Session = Get_Session())
    { Session->Fit_BoundsToLoadedWorld(); }
    Refresh_Details();
    return FReply::Handled();
}

auto FCk_PathNetworkDesigner_Toolkit::On_FitSelection() -> FReply
{
    if (auto* Session = Get_Session())
    { Session->Fit_BoundsToSelection(); }
    Refresh_Details();
    return FReply::Handled();
}

auto FCk_PathNetworkDesigner_Toolkit::On_LoadSelectedNetwork() -> FReply
{
    if (auto* Session = Get_Session())
    { Session->Load_SelectedPathNetwork(); }
    Refresh_RouteWatchItems();
    Refresh_Details();
    return FReply::Handled();
}

auto FCk_PathNetworkDesigner_Toolkit::On_CaptureRouteStart() -> FReply
{
    if (auto* Session = Get_Session())
    { Session->Capture_RouteStartFromSelection(); }
    Refresh_Details();
    return FReply::Handled();
}

auto FCk_PathNetworkDesigner_Toolkit::On_CaptureRouteGoal() -> FReply
{
    if (auto* Session = Get_Session())
    { Session->Capture_RouteGoalFromSelection(); }
    Refresh_Details();
    return FReply::Handled();
}

auto FCk_PathNetworkDesigner_Toolkit::On_PreviewRoute() -> FReply
{
    if (auto* Session = Get_Session())
    { Session->Run_RoutePreview(); }
    Refresh_Details();
    return FReply::Handled();
}

auto FCk_PathNetworkDesigner_Toolkit::On_AddRouteWatch() -> FReply
{
    if (auto* Session = Get_Session();
        Session != nullptr && _RouteWatchNameTextBox.IsValid())
    {
        const auto NameText =
            _RouteWatchNameTextBox->GetText().ToString().TrimStartAndEnd();
        Session->Add_RouteWatch(FName{*NameText});
    }
    Refresh_RouteWatchItems();
    Refresh_Details();
    return FReply::Handled();
}

auto FCk_PathNetworkDesigner_Toolkit::On_SaveRouteWatch() -> FReply
{
    if (auto* Session = Get_Session();
        Session != nullptr && _RouteWatchNameTextBox.IsValid())
    {
        const auto NameText =
            _RouteWatchNameTextBox->GetText().ToString().TrimStartAndEnd();
        Session->Save_ActiveRouteWatch(FName{*NameText});
    }
    Refresh_RouteWatchItems();
    Refresh_Details();
    return FReply::Handled();
}

auto FCk_PathNetworkDesigner_Toolkit::On_RemoveRouteWatch() -> FReply
{
    if (auto* Session = Get_Session())
    { Session->Remove_ActiveRouteWatch(); }
    Refresh_RouteWatchItems();
    Refresh_Details();
    return FReply::Handled();
}

auto FCk_PathNetworkDesigner_Toolkit::On_RefreshAllRouteWatches() -> FReply
{
    if (auto* Session = Get_Session())
    { Session->Refresh_AllRouteWatches(); }
    Refresh_Details();
    return FReply::Handled();
}

auto FCk_PathNetworkDesigner_Toolkit::On_Preview() -> FReply
{
    if (auto* Session = Get_Session())
    { Session->Run_Preview(); }
    Refresh_Details();
    return FReply::Handled();
}

auto FCk_PathNetworkDesigner_Toolkit::On_Apply() -> FReply
{
    if (auto* Session = Get_Session())
    { Session->Apply_ToLevel(); }
    Refresh_RouteWatchItems();
    Refresh_Details();
    return FReply::Handled();
}

auto FCk_PathNetworkDesigner_Toolkit::On_ClearPreview() -> FReply
{
    if (auto* Session = Get_Session())
    {
        Session->Clear_Preview();
        Session->Notify_ConfigurationEdited();
    }
    return FReply::Handled();
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    On_ConfigurationChanged(
        const FPropertyChangedEvent& InEvent)
    -> void
{
    const auto PropertyName = InEvent.GetPropertyName();
    const auto MemberPropertyName = InEvent.GetMemberPropertyName();
    const bool IsOverlayProperty =
        PropertyName == TEXT("_DrawBounds")
        || PropertyName == TEXT("_DrawMask")
        || PropertyName == TEXT("_DrawPreviewRibbons")
        || PropertyName == TEXT("_DrawAuthoredRibbons")
        || PropertyName == TEXT("_DrawGeneratedRibbons")
        || PropertyName == TEXT("_DrawComponentTransfers")
        || PropertyName == TEXT("_DrawRoutePreview")
        || PropertyName == TEXT("_MaxMaskCellsToDraw");
    const bool IsRoutePreviewProperty =
        MemberPropertyName == TEXT("_RecommendedFollowerTuning")
        || PropertyName == TEXT("_UseRecommendedFollowerTuning")
        || PropertyName == TEXT("_RoutePreviewStart")
        || PropertyName == TEXT("_RoutePreviewGoal");
    if (NOT IsOverlayProperty && NOT IsRoutePreviewProperty)
    {
        if (auto* Session = Get_Session())
        { Session->Notify_ConfigurationEdited(); }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_PathNetworkDesigner_Toolkit::
    On_PresetSelected(
        TSharedPtr<ck::pathnetwork_editor::designer::FPreset> InPreset,
        ESelectInfo::Type InSelectInfo)
    -> void
{
    if (NOT InPreset.IsValid())
    { return; }

    _SelectedPreset = InPreset;
    if (auto* Session = Get_Session())
    { Session->Apply_Preset(InPreset->_Owner, InPreset->_Id); }
    Refresh_Details();
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    Make_PresetRow(
        TSharedPtr<ck::pathnetwork_editor::designer::FPreset> InPreset) const
    -> TSharedRef<SWidget>
{
    return SNew(STextBlock)
        .Text(InPreset.IsValid()
            ? InPreset->_DisplayName
            : LOCTEXT("MissingPreset", "Missing preset"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_PathNetworkDesigner_Toolkit::
    On_RouteWatchSelected(
        TSharedPtr<FName> InRouteWatch,
        const ESelectInfo::Type InSelectInfo)
    -> void
{
    if (NOT InRouteWatch.IsValid())
    { return; }

    _SelectedRouteWatchItem = InRouteWatch;
    const auto RouteIndex =
        _RouteWatchItems.IndexOfByKey(InRouteWatch);
    if (InSelectInfo != ESelectInfo::Direct)
    {
        if (auto* Session = Get_Session())
        { Session->Select_RouteWatch(RouteIndex); }
    }

    if (_RouteWatchNameTextBox.IsValid())
    {
        _RouteWatchNameTextBox->SetText(
            FText::FromName(*InRouteWatch));
    }
    Refresh_Details();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_PathNetworkDesigner_Toolkit::
    Make_RouteWatchRow(
        TSharedPtr<FName> InRouteWatch) const
    -> TSharedRef<SWidget>
{
    return SNew(STextBlock)
        .Text(InRouteWatch.IsValid()
            ? FText::FromName(*InRouteWatch)
            : LOCTEXT("MissingRouteWatch", "Missing route watch"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_PathNetworkDesigner_Toolkit::
    Get_TargetLevelText() const
    -> FText
{
    const auto* Session = Get_Session();
    const auto* Level = Session != nullptr ? Session->Get_TargetLevel() : nullptr;
    return Level != nullptr
        ? FText::FromString(Level->GetOutermost()->GetName())
        : LOCTEXT("NoTargetLevel", "No target level");
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    Get_DetectorText() const
    -> FText
{
    const auto* Session = Get_Session();
    const auto* Detector = Session != nullptr ? Session->Get_DetectorTemplate() : nullptr;
    return Detector != nullptr
        ? FText::Format(
            LOCTEXT("DetectorClassText", "Detector: {0}"),
            Detector->GetClass()->GetDisplayNameText())
        : LOCTEXT("NoDetector", "Detector: none");
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    Get_ConnectivityText() const
    -> FText
{
    const auto* Session = Get_Session();
    if (Session == nullptr || NOT Session->Get_HasTopologyAnalysis())
    {
        return LOCTEXT(
            "ConnectivityPending",
            "Run Preview to inspect the prospective path-network topology.");
    }

    const auto& Analysis = Session->Get_TopologyAnalysis();
    const auto Get_Percent = [](const int32 InNumerator, const int32 InDenominator)
    {
        return InDenominator > 0
            ? FMath::RoundToInt(100.0 * static_cast<double>(InNumerator) / static_cast<double>(InDenominator))
            : 0;
    };
    const auto LargestNodePercent = Get_Percent(
        Analysis._LargestComponentNodeCount,
        Analysis._RoutableNodeCount);
    const auto LargestEdgePercent = Get_Percent(
        Analysis._LargestComponentEdgeCount,
        Analysis._EdgeCount);
    const auto IsolatedSuffix = Analysis._IsolatedNodeCount > 0
        ? FText::Format(
            LOCTEXT("ConnectivityIsolatedSuffix", " | {0} isolated nodes"),
            FText::AsNumber(Analysis._IsolatedNodeCount))
        : FText::GetEmpty();
    auto ComponentMessage = LOCTEXT(
            "ConnectivityUnified",
            "One routable island in this preview.");
    if (Analysis._ComponentCount > 1)
    {
        const auto TransferMaxDistance =
            Session->Get_ComponentTransferMaxDistance();
        ComponentMessage = TransferMaxDistance > 0.0f
            ? FText::Format(
                LOCTEXT(
                    "ConnectivitySplitWithTransfers",
                    "Separate islands may transfer across navmesh-valid gaps up to {0} cm."),
                FText::AsNumber(TransferMaxDistance))
            : LOCTEXT(
                "ConnectivitySplitWithoutTransfers",
                "Separate islands need authored connections or an enabled Maximum Component Transfer Distance.");
    }

    return FText::Format(
        LOCTEXT(
            "ConnectivitySummary",
            "Node snap: {0} cm\n{1} nodes | {2} edges\n{3} routable components — {4}\nLargest: {5}/{6} routable nodes ({7}%) | {8}/{9} edges ({10}%)\n{11} dead ends{12}"),
        FText::AsNumber(static_cast<double>(Session->Get_NodeSnapRadius())),
        FText::AsNumber(Analysis._NodeCount),
        FText::AsNumber(Analysis._EdgeCount),
        FText::AsNumber(Analysis._ComponentCount),
        ComponentMessage,
        FText::AsNumber(Analysis._LargestComponentNodeCount),
        FText::AsNumber(Analysis._RoutableNodeCount),
        FText::AsNumber(LargestNodePercent),
        FText::AsNumber(Analysis._LargestComponentEdgeCount),
        FText::AsNumber(Analysis._EdgeCount),
        FText::AsNumber(LargestEdgePercent),
        FText::AsNumber(Analysis._DeadEndNodeCount),
        IsolatedSuffix);
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    Get_ConnectivityColor() const
    -> FSlateColor
{
    const auto* Session = Get_Session();
    if (Session == nullptr || NOT Session->Get_HasTopologyAnalysis())
    { return FSlateColor{CkStyle::TextDim()}; }

    return Session->Get_TopologyAnalysis()._ComponentCount > 1
        ? FSlateColor{CkStyle::Warn()}
        : FSlateColor{CkStyle::Info()};
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    Get_RoutePreviewText() const
    -> FText
{
    const auto* Session = Get_Session();
    if (Session == nullptr)
    { return LOCTEXT("NoRouteSession", "Route preview unavailable."); }

    const auto& RoutePreview = Session->Get_RoutePreview();
    if (RoutePreview._Succeeded)
    {
        const auto Get_DistanceText = [](const float InDistance)
        {
            return InDistance >= 0.0f
                ? FText::Format(
                    LOCTEXT("RoutePreviewDistance", "{0} cm"),
                    FText::AsNumber(FMath::RoundToInt(InDistance)))
                : LOCTEXT("RoutePreviewNoDistance", "none found");
        };
        const auto Get_NetworkDiagnosticText = [](const FString& InOutcome)
        {
            if (InOutcome == TEXT("SelectedPlanUsesNetwork"))
            {
                return LOCTEXT(
                    "RoutePreviewDiagnosticSelectedNetwork",
                    "selected route uses sidewalks");
            }
            if (InOutcome == TEXT("NotRunNoStartJoin"))
            {
                return LOCTEXT(
                    "RoutePreviewDiagnosticNoStartJoin",
                    "not run; start cannot join a sidewalk");
            }
            if (InOutcome == TEXT("NotRunNoGoalJoin"))
            {
                return LOCTEXT(
                    "RoutePreviewDiagnosticNoGoalJoin",
                    "not run; goal cannot join a sidewalk");
            }
            if (InOutcome == TEXT("Complete"))
            { return LOCTEXT("RoutePreviewDiagnosticComplete", "completed"); }
            if (InOutcome == TEXT("Failed"))
            {
                return LOCTEXT(
                    "RoutePreviewDiagnosticFailed",
                    "completed; no traversable path");
            }
            if (InOutcome == TEXT("InProgress"))
            {
                return LOCTEXT(
                    "RoutePreviewDiagnosticInProgress",
                    "search limit reached");
            }
            if (InOutcome == TEXT("CostThresholdReached"))
            {
                return LOCTEXT(
                    "RoutePreviewDiagnosticCostThreshold",
                    "cost threshold reached");
            }
            if (InOutcome == TEXT("InvalidInput"))
            { return LOCTEXT("RoutePreviewDiagnosticInvalidInput", "invalid input"); }
            return LOCTEXT("RoutePreviewDiagnosticNotRun", "not run");
        };
        const auto BestNetworkText =
            RoutePreview._HasNetworkAlternative
            ? FText::Format(
                LOCTEXT("RoutePreviewBestNetwork", "Best sidewalk cost: {0}"),
                FText::AsNumber(FMath::RoundToInt(
                    RoutePreview._BestNetworkEstimatedCost)))
            : RoutePreview._Decision
                == ck::pathnetwork_editor::designer::
                    ERoutePreviewDecision::NetworkDiagnosticIncomplete
                ? LOCTEXT(
                    "RoutePreviewNetworkDiagnosticIncomplete",
                    "Best sidewalk cost: unresolved (search limit reached)")
                : LOCTEXT(
                    "RoutePreviewNoNetworkAlternative",
                    "Best sidewalk cost: no connected alternative");
        const auto CandidateText = FText::Format(
            LOCTEXT(
                "RoutePreviewCandidates",
                "Start: {0} candidates, nearest {1} | Goal: {2} candidates, nearest {3}"),
            FText::AsNumber(RoutePreview._StartCandidateCount),
            Get_DistanceText(RoutePreview._NearestStartNetworkDistance),
            FText::AsNumber(RoutePreview._GoalCandidateCount),
            Get_DistanceText(RoutePreview._NearestGoalNetworkDistance));
        auto PercentageFormat = FNumberFormattingOptions{};
        PercentageFormat.SetMaximumFractionalDigits(2);
        const auto MinimumDirectSavingsText =
            FText::AsNumber(
                RoutePreview._DirectRouteMinimumSavingsFraction * 100.0f,
                &PercentageFormat);
        const auto DirectSavingsText =
            RoutePreview._HasNetworkAlternative
            ? FText::Format(
                LOCTEXT(
                    "RoutePreviewDirectSavings",
                    "{0}% (minimum {1}%)"),
                FText::AsNumber(
                    RoutePreview._DirectRouteSavingsFraction * 100.0f,
                    &PercentageFormat),
                MinimumDirectSavingsText)
            : FText::Format(
                LOCTEXT(
                    "RoutePreviewDirectSavingsUnavailable",
                    "not available (minimum {0}%)"),
                MinimumDirectSavingsText);
        const auto DirectGraceText =
            RoutePreview._DirectTripGraceApplied
            ? LOCTEXT("RoutePreviewDirectGraceApplied", "applied")
            : LOCTEXT("RoutePreviewDirectGraceNotApplied", "not applied");
        const auto EndpointJoinLimitText =
            RoutePreview._EndpointJoinMaxDistance > 0.0f
            ? FText::Format(
                LOCTEXT("RoutePreviewEndpointJoinLimit", "{0} cm"),
                FText::AsNumber(FMath::RoundToInt(
                    RoutePreview._EndpointJoinMaxDistance)))
            : LOCTEXT("RoutePreviewEndpointJoinUnlimited", "unlimited");
        const auto ComponentTransferLimitText =
            RoutePreview._ComponentTransferMaxDistance > 0.0f
            ? FText::Format(
                LOCTEXT("RoutePreviewComponentTransferLimit", "{0} cm"),
                FText::AsNumber(FMath::RoundToInt(
                    RoutePreview._ComponentTransferMaxDistance)))
            : LOCTEXT("RoutePreviewComponentTransferDisabled", "disabled");
        const auto ComponentTransferEvidenceText =
            RoutePreview._ComponentTransferMaxDistance <= 0.0f
            ? LOCTEXT(
                "RoutePreviewComponentTransferDisabledEvidence",
                "Component transfer: disabled")
            : RoutePreview._ComponentTransferLegCount > 0
                ? FText::Format(
                    LOCTEXT(
                        "RoutePreviewComponentTransferUsed",
                        "Component transfer: used {0}, longest {1} cm; {2} admitted from {3} candidates, including {4} edge-interior opportunities ({5} rejected by cell cap)"),
                    FText::AsNumber(RoutePreview._ComponentTransferLegCount),
                    FText::AsNumber(FMath::RoundToInt(
                        RoutePreview._LongestComponentTransferDistance)),
                    FText::AsNumber(RoutePreview._ComponentTransferLinkCount),
                    FText::AsNumber(RoutePreview._ComponentTransferCandidateCount),
                    FText::AsNumber(
                        RoutePreview
                            ._ComponentTransferEdgeInteriorCandidateCount),
                    FText::AsNumber(RoutePreview._ComponentTransferRejectedByCellCapCount))
                : RoutePreview._ComponentTransferLinkCount > 0
                    ? FText::Format(
                        LOCTEXT(
                            "RoutePreviewComponentTransferAvailable",
                            "Component transfer: {0} admitted, none selected from {1} candidates, including {2} edge-interior opportunities ({3} rejected by cell cap)"),
                        FText::AsNumber(RoutePreview._ComponentTransferLinkCount),
                        FText::AsNumber(RoutePreview._ComponentTransferCandidateCount),
                        FText::AsNumber(
                            RoutePreview
                                ._ComponentTransferEdgeInteriorCandidateCount),
                        FText::AsNumber(RoutePreview._ComponentTransferRejectedByCellCapCount))
                    : FText::Format(
                        LOCTEXT(
                            "RoutePreviewComponentTransferUnavailable",
                            "Component transfer: none admitted from {0} candidates, including {1} edge-interior opportunities ({2} rejected by cell cap)"),
                        FText::AsNumber(RoutePreview._ComponentTransferCandidateCount),
                        FText::AsNumber(
                            RoutePreview
                                ._ComponentTransferEdgeInteriorCandidateCount),
                        FText::AsNumber(RoutePreview._ComponentTransferRejectedByCellCapCount));
        const auto NetworkGapCostEvidenceText =
            RoutePreview._ConfiguredNetworkGapCostMultiplier > 0.0f
            ? FText::Format(
                LOCTEXT(
                    "RoutePreviewNetworkGapCostConfigured",
                    "Network gap cost: configured {0}, effective {1}. At runtime it prices only navmesh-validated network-to-network jumps, including component transfers and local crossings; it does not price arbitrary off-network or direct travel."),
                FText::AsNumber(RoutePreview._ConfiguredNetworkGapCostMultiplier),
                FText::AsNumber(RoutePreview._EffectiveNetworkGapCostMultiplier))
            : FText::Format(
                LOCTEXT(
                    "RoutePreviewNetworkGapCostInherited",
                    "Network gap cost: inherits Far/Direct Cost Multiplier, effective {0}. At runtime it prices only navmesh-validated network-to-network jumps, including component transfers and local crossings; it does not price arbitrary off-network or direct travel."),
                FText::AsNumber(RoutePreview._EffectiveNetworkGapCostMultiplier));
        const auto LocalShortcutEvidenceText =
            RoutePreview._LocalNetworkShortcutMaxDistance <= 0.0f
            ? LOCTEXT(
                "RoutePreviewLocalShortcutDisabled",
                "Local sidewalk crossing: disabled")
            : RoutePreview._LocalNetworkShortcutBudgetExceeded
                ? FText::Format(
                    LOCTEXT(
                        "RoutePreviewLocalShortcutBudgetExceeded",
                        "Local sidewalk crossing: skipped because {0} nearby pairs from {1} source nodes exceed the safety budget. Lower Maximum Local Sidewalk Gap."),
                    FText::AsNumber(
                        RoutePreview
                            ._LocalNetworkShortcutCandidateCount),
                    FText::AsNumber(
                        RoutePreview
                            ._LocalNetworkShortcutCandidateSourceCount))
            : RoutePreview._LocalNetworkShortcutLegCount > 0
                ? FText::Format(
                    LOCTEXT(
                        "RoutePreviewLocalShortcutUsed",
                        "Local sidewalk crossing: used {0}, longest {1} cm (limit {2} cm)"),
                    FText::AsNumber(
                        RoutePreview._LocalNetworkShortcutLegCount),
                    FText::AsNumber(FMath::RoundToInt(
                        RoutePreview
                            ._LongestLocalNetworkShortcutDistance)),
                    FText::AsNumber(FMath::RoundToInt(
                        RoutePreview
                            ._LocalNetworkShortcutMaxDistance)))
                : RoutePreview._LocalNetworkShortcutLinkCount > 0
                    ? FText::Format(
                        LOCTEXT(
                            "RoutePreviewLocalShortcutAvailable",
                            "Local sidewalk crossing: {0} available, none selected (limit {1} cm)"),
                        FText::AsNumber(
                            RoutePreview
                                ._LocalNetworkShortcutLinkCount),
                        FText::AsNumber(FMath::RoundToInt(
                            RoutePreview
                                ._LocalNetworkShortcutMaxDistance)))
                    : FText::Format(
                        LOCTEXT(
                            "RoutePreviewLocalShortcutUnavailable",
                            "Local sidewalk crossing: none available within {0} cm"),
                        FText::AsNumber(FMath::RoundToInt(
                            RoutePreview
                                ._LocalNetworkShortcutMaxDistance)));
        const auto EvidenceText = FText::Format(
            LOCTEXT(
                "RoutePreviewEvidence",
                "Selected cost: {0} | Direct cost: {1}\n{2}\nDirect saving: {3} | Short-trip grace: {4}\n{5}\nNetwork diagnostic: {6}\nNetwork components: {7} | Component-transfer links: {8}\nEndpoint join limit: {9} | Component-transfer limit: {10}\n{11}\n{12}\n{13}\n{14} sidewalk legs | {15} off-network legs\nGraph-only preview: off-network spans are not navmesh-validated."),
            FText::AsNumber(FMath::RoundToInt(RoutePreview._EstimatedCost)),
            FText::AsNumber(FMath::RoundToInt(RoutePreview._DirectEstimatedCost)),
            BestNetworkText,
            DirectSavingsText,
            DirectGraceText,
            CandidateText,
            Get_NetworkDiagnosticText(RoutePreview._NetworkDiagnosticOutcome),
            FText::AsNumber(RoutePreview._NetworkComponentCount),
            FText::AsNumber(RoutePreview._ComponentTransferLinkCount),
            EndpointJoinLimitText,
            ComponentTransferLimitText,
            ComponentTransferEvidenceText,
            NetworkGapCostEvidenceText,
            LocalShortcutEvidenceText,
            FText::AsNumber(RoutePreview._OnNetworkLegCount),
            FText::AsNumber(RoutePreview._OffNetworkLegCount));

        switch (RoutePreview._Decision)
        {
            case ck::pathnetwork_editor::designer::ERoutePreviewDecision::NetworkSelected:
            {
                return FText::Format(
                    LOCTEXT("RoutePreviewNetworkSelected", "SIDEWALK ROUTE\n{0}"),
                    EvidenceText);
            }
            case ck::pathnetwork_editor::designer::
                ERoutePreviewDecision::NetworkPreferredByMinimumSavings:
            {
                return FText::Format(
                    LOCTEXT(
                        "RoutePreviewNetworkPreferredByMinimumSavings",
                        "SIDEWALK ROUTE: direct saving below minimum\n{0}"),
                    EvidenceText);
            }
            case ck::pathnetwork_editor::designer::ERoutePreviewDecision::DirectCostWon:
            {
                const auto DirectDecision =
                    RoutePreview._DirectTripGraceApplied
                    ? LOCTEXT(
                        "RoutePreviewDirectGraceWon",
                        "DIRECT FALLBACK: short-trip grace")
                    : FText::Format(
                        LOCTEXT(
                            "RoutePreviewDirectSavingsWon",
                            "DIRECT FALLBACK: saves {0}% (requires {1}%)"),
                        FText::AsNumber(
                            RoutePreview._DirectRouteSavingsFraction * 100.0f,
                            &PercentageFormat),
                        MinimumDirectSavingsText);
                return FText::Format(
                    LOCTEXT("RoutePreviewDirectCostWon", "{0}\n{1}"),
                    DirectDecision,
                    EvidenceText);
            }
            case ck::pathnetwork_editor::designer::ERoutePreviewDecision::NoStartJoin:
            {
                return FText::Format(
                    LOCTEXT("RoutePreviewNoStartJoin", "DIRECT FALLBACK: start has no sidewalk join\n{0}"),
                    EvidenceText);
            }
            case ck::pathnetwork_editor::designer::ERoutePreviewDecision::NoGoalJoin:
            {
                return FText::Format(
                    LOCTEXT("RoutePreviewNoGoalJoin", "DIRECT FALLBACK: goal has no sidewalk join\n{0}"),
                    EvidenceText);
            }
            case ck::pathnetwork_editor::designer::ERoutePreviewDecision::NoConnectedNetworkRoute:
            {
                return FText::Format(
                    LOCTEXT(
                        "RoutePreviewNoConnectedNetworkRoute",
                        "DIRECT FALLBACK: no traversable sidewalk alternative\n{0}"),
                    EvidenceText);
            }
            case ck::pathnetwork_editor::designer::ERoutePreviewDecision::NetworkDiagnosticIncomplete:
            {
                return FText::Format(
                    LOCTEXT(
                        "RoutePreviewNetworkDiagnosticIncompleteDecision",
                        "DIRECT FALLBACK: sidewalk diagnosis reached its search limit\n{0}"),
                    EvidenceText);
            }
            case ck::pathnetwork_editor::designer::ERoutePreviewDecision::Unavailable:
            default:
            {
                return FText::Format(
                    LOCTEXT("RoutePreviewUnknownDecision", "ROUTE DECISION UNAVAILABLE\n{0}"),
                    EvidenceText);
            }
        }
    }

    if (NOT RoutePreview._FailureReason.IsEmpty())
    { return FText::FromString(RoutePreview._FailureReason); }

    if (Session->Get_Preview()._Succeeded)
    {
        const auto AvailableTransferText =
            RoutePreview._ComponentTransferMaxDistance > 0.0f
            ? FText::Format(
                LOCTEXT(
                    "RoutePreviewPendingWithTransfers",
                    "AVAILABLE JUMPS (PREVIEW)\n{0} admitted from {1} candidates, including {2} edge-interior opportunities ({3} rejected by cell cap), maximum gap {4} cm.\nNetwork gap cost: {5}; effective {6}. Zero inherits Far/Direct Cost Multiplier. At runtime it prices navmesh-validated network-to-network jumps only, not arbitrary off-network or direct travel.\nThin amber links in the viewport are usable component transfers, including crossings between long ribbon interiors.\nCapture or enter distinct start and goal locations to see which jump the route selects."),
                FText::AsNumber(RoutePreview._ComponentTransferLinkCount),
                FText::AsNumber(RoutePreview._ComponentTransferCandidateCount),
                FText::AsNumber(
                    RoutePreview
                        ._ComponentTransferEdgeInteriorCandidateCount),
                FText::AsNumber(
                    RoutePreview._ComponentTransferRejectedByCellCapCount),
                FText::AsNumber(FMath::RoundToInt(
                    RoutePreview._ComponentTransferMaxDistance)),
                RoutePreview._ConfiguredNetworkGapCostMultiplier > 0.0f
                    ? FText::AsNumber(RoutePreview._ConfiguredNetworkGapCostMultiplier)
                    : LOCTEXT("RoutePreviewPendingNetworkGapCostInherited", "inherits Far/Direct Cost Multiplier"),
                FText::AsNumber(RoutePreview._EffectiveNetworkGapCostMultiplier))
            : LOCTEXT(
                "RoutePreviewPendingTransfersDisabled",
                "AVAILABLE JUMPS (PREVIEW)\nComponent transfers are disabled. Enable map route preferences and set Maximum Component Transfer Distance above zero, then refresh Preview.\nCapture or enter distinct start and goal locations to inspect a representative route.");
        return AvailableTransferText;
    }

    return LOCTEXT(
        "RoutePreviewPending",
        "Capture or enter distinct start and goal locations after generating a detector preview.");
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    Get_RoutePreviewColor() const
    -> FSlateColor
{
    const auto* Session = Get_Session();
    if (Session == nullptr)
    { return FSlateColor{CkStyle::TextDim()}; }

    const auto& RoutePreview = Session->Get_RoutePreview();
    if (NOT RoutePreview._Succeeded)
    {
        return RoutePreview._FailureReason.IsEmpty()
            ? FSlateColor{CkStyle::TextDim()}
            : FSlateColor{CkStyle::Err()};
    }

    const auto IsNetworkDecision =
        RoutePreview._Decision
            == ck::pathnetwork_editor::designer::
                ERoutePreviewDecision::NetworkSelected
        || RoutePreview._Decision
            == ck::pathnetwork_editor::designer::
                ERoutePreviewDecision::
                    NetworkPreferredByMinimumSavings;
    return IsNetworkDecision
        ? FSlateColor{CkStyle::Info()}
        : FSlateColor{CkStyle::Warn()};
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    Get_StatusText() const
    -> FText
{
    const auto* Session = Get_Session();
    return Session != nullptr
        ? FText::FromString(Session->Get_StatusMessage())
        : LOCTEXT("NoSessionStatus", "Designer session is unavailable.");
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    Get_StatusColor() const
    -> FSlateColor
{
    const auto* Session = Get_Session();
    if (Session == nullptr)
    { return FSlateColor{CkStyle::TextDim()}; }

    switch (Session->Get_Status())
    {
        case ECk_PathNetworkDesigner_Status::PreviewReady: return FSlateColor{CkStyle::Info()};
        case ECk_PathNetworkDesigner_Status::Applied:      return FSlateColor{CkStyle::Ok()};
        case ECk_PathNetworkDesigner_Status::Error:        return FSlateColor{CkStyle::Err()};
        default:                                           return FSlateColor{CkStyle::TextDim()};
    }
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    Get_StatusBackgroundColor() const
    -> FSlateColor
{
    const auto* Session = Get_Session();
    if (Session == nullptr)
    { return FSlateColor{CkStyle::NeutralDim()}; }

    switch (Session->Get_Status())
    {
        case ECk_PathNetworkDesigner_Status::PreviewReady: return FSlateColor{CkStyle::InfoDim()};
        case ECk_PathNetworkDesigner_Status::Applied:      return FSlateColor{CkStyle::OkDim()};
        case ECk_PathNetworkDesigner_Status::Error:        return FSlateColor{CkStyle::ErrDim()};
        default:                                           return FSlateColor{CkStyle::NeutralDim()};
    }
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    Get_ActivePresetText() const
    -> FText
{
    const auto* Session = Get_Session();
    if (Session != nullptr)
    {
        const auto Owner = Session->Get_ActivePresetOwner();
        const auto Id = Session->Get_ActivePresetId();
        for (const auto& Preset : _PresetItems)
        {
            if (Preset.IsValid()
                && Preset->_Owner == Owner
                && Preset->_Id == Id)
            { return Preset->_DisplayName; }
        }
    }
    return LOCTEXT("CustomWorkflow", "Custom detector configuration");
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    Get_ActiveRouteWatchText() const
    -> FText
{
    const auto* Session = Get_Session();
    if (Session != nullptr)
    {
        const auto Names = Session->Get_RouteWatchNames();
        const auto ActiveIndex = Session->Get_ActiveRouteWatchIndex();
        if (Names.IsValidIndex(ActiveIndex))
        { return FText::FromName(Names[ActiveIndex]); }
    }
    return LOCTEXT("NoRouteWatchSelected", "No saved route watch");
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    Get_RouteWatchSummaryText() const
    -> FText
{
    const auto* Session = Get_Session();
    if (Session == nullptr || Session->Get_RouteWatchCount() <= 0)
    {
        return LOCTEXT(
            "NoRouteWatches",
            "No saved route watches. Capture endpoints, enter a name, and click Add.");
    }

    const auto& Previews = Session->Get_RouteWatchPreviews();
    if (Previews.IsEmpty())
    {
        return FText::Format(
            LOCTEXT(
                "RouteWatchesPending",
                "{0} saved route watches. Click Refresh All to evaluate and draw them together."),
            FText::AsNumber(Session->Get_RouteWatchCount()));
    }

    auto Summary = FString{};
    for (const auto& Watch : Previews)
    {
        const auto* Decision = NOT Watch._Preview._Succeeded
            ? TEXT("UNAVAILABLE")
            : Watch._Preview._UsesNetwork
                ? TEXT("SIDEWALK")
                : TEXT("DIRECT");
        Summary += FString::Printf(
            TEXT("%s  [%s]  cost %.0f"),
            *Watch._Name.ToString(),
            Decision,
            Watch._Preview._EstimatedCost);
        if (NOT Watch._Preview._FailureReason.IsEmpty())
        {
            Summary += FString::Printf(
                TEXT("  %s"),
                *Watch._Preview._FailureReason);
        }
        Summary += TEXT("\n");
    }
    Summary.RemoveFromEnd(TEXT("\n"));
    return FText::FromString(Summary);
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    Get_CanPreview() const
    -> bool
{
    const auto* Session = Get_Session();
    return Session != nullptr
        && Session->Get_TargetLevel() != nullptr
        && Session->Get_DetectorTemplate() != nullptr;
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    Get_CanPreviewRoute() const
    -> bool
{
    const auto* Session = Get_Session();
    return Session != nullptr
        && Session->Get_Preview()._Succeeded
        && NOT Session->Get_RoutePreviewStart().ContainsNaN()
        && NOT Session->Get_RoutePreviewGoal().ContainsNaN()
        && FVector::Dist(
            Session->Get_RoutePreviewStart(),
            Session->Get_RoutePreviewGoal()) > 1.0;
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    Get_CanAddRouteWatch() const
    -> bool
{
    const auto* Session = Get_Session();
    const auto NameText = _RouteWatchNameTextBox.IsValid()
        ? _RouteWatchNameTextBox->GetText().ToString().TrimStartAndEnd()
        : FString{};
    return Session != nullptr
        && Session->Get_CanPersistRouteWatches()
        && NOT NameText.IsEmpty()
        && NOT Session->Get_RoutePreviewStart().ContainsNaN()
        && NOT Session->Get_RoutePreviewGoal().ContainsNaN()
        && FVector::Dist(
            Session->Get_RoutePreviewStart(),
            Session->Get_RoutePreviewGoal()) > 1.0;
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    Get_CanSaveRouteWatch() const
    -> bool
{
    const auto* Session = Get_Session();
    return Get_CanAddRouteWatch()
        && Session != nullptr
        && Session->Get_RouteWatchNames().IsValidIndex(
            Session->Get_ActiveRouteWatchIndex());
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    Get_CanRefreshAllRouteWatches() const
    -> bool
{
    const auto* Session = Get_Session();
    return Session != nullptr
        && Session->Get_Preview()._Succeeded
        && Session->Get_RouteWatchCount() > 0;
}

auto
    FCk_PathNetworkDesigner_Toolkit::
    Get_CanApply() const
    -> bool
{
    const auto* Session = Get_Session();
    return Session != nullptr
        && Session->Get_Preview()._Succeeded;
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
