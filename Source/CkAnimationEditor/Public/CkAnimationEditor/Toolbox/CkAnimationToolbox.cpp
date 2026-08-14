#include "CkAnimationToolbox.h"

#include "CkAnimationEditor_Log.h"

#include <Animation/AnimBlueprint.h>
#include <Animation/AnimSequence.h>
#include <Misc/ScopedSlowTask.h>
#include <AssetRegistry/AssetData.h>
#include <Subsystems/AssetEditorSubsystem.h>
#include <Editor.h>
#include <PropertyCustomizationHelpers.h>
#include <Widgets/Layout/SBorder.h>
#include <Widgets/Layout/SSeparator.h>
#include <Widgets/Layout/SSplitter.h>
#include <Widgets/Layout/SUniformGridPanel.h>
#include <Widgets/Layout/SScrollBox.h>
#include <Widgets/Input/SButton.h>
#include <Widgets/Input/SCheckBox.h>
#include <Widgets/Input/SEditableTextBox.h>
#include <Widgets/Input/SSegmentedControl.h>
#include <Widgets/Text/STextBlock.h>
#include <Widgets/Text/SRichTextBlock.h>
#include <Styling/AppStyle.h>
#include <EditorFontGlyphs.h>

#define LOCTEXT_NAMESPACE "CkAnimationToolbox"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::animation_editor::ColumnNames
{
    static const FName Select   = TEXT("Select");
    static const FName Status   = TEXT("Status");
    static const FName Asset    = TEXT("Asset");
    static const FName Ratio    = TEXT("Ratio");
    static const FName Frames   = TEXT("Frames");
    static const FName Notifies = TEXT("Notifies");
    static const FName Message  = TEXT("Message");
}

namespace ck::animation_editor::detail
{
    static FText StatusToText(ECk_AnimationToolbox_Status S)
    {
        switch (S)
        {
            case ECk_AnimationToolbox_Status::Clean:    return LOCTEXT("StatusClean",    "OK");
            case ECk_AnimationToolbox_Status::Affected: return LOCTEXT("StatusAffected", "Bad");
            case ECk_AnimationToolbox_Status::Skipped:  return LOCTEXT("StatusSkipped",  "Skip");
            case ECk_AnimationToolbox_Status::Fixed:    return LOCTEXT("StatusFixed",    "Fixed");
            case ECk_AnimationToolbox_Status::Failed:   return LOCTEXT("StatusFailed",   "Failed");
            default:                                     return LOCTEXT("StatusUnknown",  "—");
        }
    }

    static FSlateColor StatusToColor(ECk_AnimationToolbox_Status S)
    {
        switch (S)
        {
            case ECk_AnimationToolbox_Status::Affected: return FSlateColor(FLinearColor(1.0f, 0.45f, 0.1f));
            case ECk_AnimationToolbox_Status::Clean:    return FSlateColor(FLinearColor(0.3f, 0.9f, 0.35f));
            case ECk_AnimationToolbox_Status::Fixed:    return FSlateColor(FLinearColor(0.2f, 0.75f, 1.0f));
            case ECk_AnimationToolbox_Status::Failed:   return FSlateColor(FLinearColor(1.0f, 0.25f, 0.25f));
            case ECk_AnimationToolbox_Status::Skipped:  return FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f));
            default:                                     return FSlateColor::UseForeground();
        }
    }

    static FString StrategyToString(ECk_AnimationToolbox_RewriteStrategy S)
    {
        switch (S)
        {
            case ECk_AnimationToolbox_RewriteStrategy::TrimFirstFrame:   return TEXT("A: Trim frame 0 (length shrinks)");
            case ECk_AnimationToolbox_RewriteStrategy::CopyFromFrameOne: return TEXT("B: Copy frame 1 onto frame 0");
            case ECk_AnimationToolbox_RewriteStrategy::CopyFromLastFrame:return TEXT("C: Copy last frame onto frame 0 (recommended)");
        }
        return TEXT("");
    }
}

// --------------------------------------------------------------------------------------------------------------------
// SCkAnimListRow
// --------------------------------------------------------------------------------------------------------------------

auto
    SCkAnimListRow::
    Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
    -> void
{
    _Info  = InArgs._Info;
    _Owner = InArgs._Owner;

    SMultiColumnTableRow<TSharedPtr<FCk_AnimationToolbox_AnimInfo>>::Construct(
        FSuperRowType::FArguments(),
        InOwnerTableView);
}

auto
    SCkAnimListRow::
    GenerateWidgetForColumn(const FName& ColumnName)
    -> TSharedRef<SWidget>
{
    using namespace ck::animation_editor::ColumnNames;
    using namespace ck::animation_editor::detail;

    if (!_Info.IsValid())
    { return SNullWidget::NullWidget; }

    if (ColumnName == Select)
    {
        const bool bEligible = (_Info->Status == ECk_AnimationToolbox_Status::Affected)
                            || (_Info->Status == ECk_AnimationToolbox_Status::Fixed);
        return SNew(SHorizontalBox)
            + SHorizontalBox::Slot().Padding(6, 2).VAlign(VAlign_Center)
            [
                SNew(SCheckBox)
                .IsEnabled(bEligible)
                .IsChecked(this, &SCkAnimListRow::DoGetCheckState)
                .OnCheckStateChanged(this, &SCkAnimListRow::DoOnCheckStateChanged)
            ];
    }
    if (ColumnName == Status)
    {
        return SNew(STextBlock)
            .Text(StatusToText(_Info->Status))
            .ColorAndOpacity(StatusToColor(_Info->Status))
            .Margin(FMargin(6, 2));
    }
    if (ColumnName == Asset)
    {
        return SNew(STextBlock)
            .Text(FText::FromString(_Info->AssetName))
            .ToolTipText(FText::FromString(_Info->PackagePath))
            .Margin(FMargin(6, 2));
    }
    if (ColumnName == Ratio)
    {
        const FText Txt = (_Info->Status == ECk_AnimationToolbox_Status::Skipped)
            ? LOCTEXT("NotApplicable", "—")
            : FText::FromString(FString::Printf(TEXT("%.1fx"), _Info->Ratio));
        return SNew(STextBlock)
            .Text(Txt)
            .ToolTipText(FText::FromString(FString::Printf(
                TEXT("Δ(frame 0 → 1)      = %.2f (%.1fx median)\n")
                TEXT("Δ(last → frame 0)   = %.2f (%.1fx median)\n")
                TEXT("median Δ(f → f+1)   = %.2f\n(units: cm + deg summed per bone, component space)"),
                _Info->Frame0To1Delta, _Info->Ratio,
                _Info->LoopBoundaryDelta, _Info->LoopBoundaryRatio,
                _Info->MedianDelta)))
            .Margin(FMargin(6, 2));
    }
    if (ColumnName == Frames)
    {
        return SNew(STextBlock).Text(FText::AsNumber(_Info->NumFrames)).Margin(FMargin(6, 2));
    }
    if (ColumnName == Notifies)
    {
        return SNew(STextBlock).Text(FText::AsNumber(_Info->NumNotifies)).Margin(FMargin(6, 2));
    }
    if (ColumnName == Message)
    {
        // Build a tooltip listing the first few unmapped bones (if any).
        FString Tooltip = _Info->StatusMessage;
        if (_Info->UnmappedBoneCount > 0 && _Info->UnmappedBoneSample.Num() > 0)
        {
            Tooltip += TEXT("\n\nUnmapped bones (sample):\n");
            for (const FName& BN : _Info->UnmappedBoneSample)
            { Tooltip += FString::Printf(TEXT("  · %s\n"), *BN.ToString()); }
            if (_Info->UnmappedBoneCount > _Info->UnmappedBoneSample.Num())
            {
                Tooltip += FString::Printf(TEXT("  … and %d more"),
                    _Info->UnmappedBoneCount - _Info->UnmappedBoneSample.Num());
            }
        }
        return SNew(STextBlock)
            .Text(FText::FromString(_Info->StatusMessage))
            .ToolTipText(FText::FromString(Tooltip))
            .Margin(FMargin(6, 2));
    }
    return SNullWidget::NullWidget;
}

auto
    SCkAnimListRow::
    DoOnCheckStateChanged(ECheckBoxState NewState)
    -> void
{
    if (_Info.IsValid())
    { _Info->bSelectedForFix = (NewState == ECheckBoxState::Checked); }
}

auto
    SCkAnimListRow::
    DoGetCheckState() const
    -> ECheckBoxState
{
    return (_Info.IsValid() && _Info->bSelectedForFix) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

// --------------------------------------------------------------------------------------------------------------------
// SCkAnimationToolbox
// --------------------------------------------------------------------------------------------------------------------

auto
    SCkAnimationToolbox::
    Construct(const FArguments& InArgs)
    -> void
{
    DoLoadSettings();

    _StrategyOptions = {
        MakeShared<ECk_AnimationToolbox_RewriteStrategy>(ECk_AnimationToolbox_RewriteStrategy::TrimFirstFrame),
        MakeShared<ECk_AnimationToolbox_RewriteStrategy>(ECk_AnimationToolbox_RewriteStrategy::CopyFromFrameOne),
        MakeShared<ECk_AnimationToolbox_RewriteStrategy>(ECk_AnimationToolbox_RewriteStrategy::CopyFromLastFrame),
    };

    ChildSlot
    [
        SNew(SBorder)
        .Padding(8)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)[ DoCreateOptionsPanel() ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)[ DoCreateActionsPanel() ]
            + SVerticalBox::Slot().FillHeight(0.6f)               [ DoCreateListPanel() ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)[ DoCreateStatusPanel() ]
            + SVerticalBox::Slot().FillHeight(0.4f).Padding(0, 6, 0, 0)[ DoCreateLogPanel() ]
        ]
    ];

    DoUpdateStatusText();
    DoAppendLog(TEXT("INFO"), TEXT("Anim Scrunch Fixer ready."));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkAnimationToolbox::
    DoCreateOptionsPanel()
    -> TSharedRef<SWidget>
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
        .Padding(6)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
                [
                    SNew(STextBlock).Text(LOCTEXT("ScopeLabel", "Scope:"))
                    .Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SSegmentedControl<ECk_AnimationToolbox_ScanScope>)
                    .Value(_Options.Scope)
                    .OnValueChanged_Lambda([this](ECk_AnimationToolbox_ScanScope NewScope) { DoOnScopeChanged(NewScope); })
                    + SSegmentedControl<ECk_AnimationToolbox_ScanScope>::Slot(ECk_AnimationToolbox_ScanScope::ProjectWide)
                        .Text(LOCTEXT("ScopeProject", "Project-wide"))
                    + SSegmentedControl<ECk_AnimationToolbox_ScanScope>::Slot(ECk_AnimationToolbox_ScanScope::AnimBlueprintDeps)
                        .Text(LOCTEXT("ScopeAnimBP", "AnimBlueprint deps"))
                    + SSegmentedControl<ECk_AnimationToolbox_ScanScope>::Slot(ECk_AnimationToolbox_ScanScope::Path)
                        .Text(LOCTEXT("ScopePath", "Path"))
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
                [ SNew(STextBlock).Text(LOCTEXT("AnimBPLabel", "AnimBlueprint:")).MinDesiredWidth(110) ]
                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SObjectPropertyEntryBox)
                    .AllowedClass(UAnimBlueprint::StaticClass())
                    .ObjectPath_Lambda([this](){ return _Options.AnimBlueprint.ToString(); })
                    .OnObjectChanged(this, &SCkAnimationToolbox::DoOnAnimBlueprintChanged)
                    .IsEnabled_Lambda([this]() { return _Options.Scope == ECk_AnimationToolbox_ScanScope::AnimBlueprintDeps; })
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
                [ SNew(STextBlock).Text(LOCTEXT("PathLabel", "Content Path:")).MinDesiredWidth(110) ]
                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SEditableTextBox)
                    .Text_Lambda([this](){ return FText::FromString(_Options.ContentPath); })
                    .OnTextCommitted_Lambda([this](const FText& Txt, ETextCommit::Type){ DoOnPathChanged(Txt.ToString()); })
                    .IsEnabled_Lambda([this]() { return _Options.Scope == ECk_AnimationToolbox_ScanScope::Path; })
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
                [ SNew(STextBlock).Text(LOCTEXT("ThresholdLabel", "Outlier factor:")).MinDesiredWidth(110) ]
                + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                [
                    SNew(SSpinBox<float>)
                    .MinValue(1.0f).MaxValue(20.0f).Delta(0.25f)
                    .Value_Lambda([this](){ return _Options.OutlierMultiplier; })
                    .OnValueChanged(this, &SCkAnimationToolbox::DoOnThresholdChanged)
                    .ToolTipText(LOCTEXT("ThresholdTip", "Anim is flagged when its frame 0 → frame 1 pose delta is more than this many times the median transition delta. Lower = more sensitive."))
                ]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0, 0, 0)
                [
                    SNew(STextBlock)
                    .Text_Lambda([this](){ return FText::FromString(FString::Printf(TEXT("(%.1fx)"), _Options.OutlierMultiplier)); })
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
                [ SNew(STextBlock).Text(LOCTEXT("LoopBoundaryLabel", "Loop check:")).MinDesiredWidth(110) ]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [
                    SNew(SCheckBox)
                    .IsChecked_Lambda([this]()
                    {
                        return _Options.bRequireLoopBoundaryMismatch ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                    })
                    .OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
                    {
                        _Options.bRequireLoopBoundaryMismatch = (NewState == ECheckBoxState::Checked);
                        DoSaveSettings();
                    })
                    .ToolTipText(LOCTEXT("LoopBoundaryTip",
                        "Also require last-frame → frame-0 to be an outlier. Skips non-looping anims "
                        "(stun starts, attacks, jumps) whose frame 0 legitimately differs from frame 1. "
                        "Turn off to flag abrupt openings too."))
                    [ SNew(STextBlock).Text(LOCTEXT("LoopBoundaryText", "Only flag anims with broken loop boundary")) ]
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
                [ SNew(STextBlock).Text(LOCTEXT("StrategyLabel", "Strategy:")).MinDesiredWidth(110) ]
                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SAssignNew(_StrategyCombo, SComboBox<TSharedPtr<ECk_AnimationToolbox_RewriteStrategy>>)
                    .OptionsSource(&_StrategyOptions)
                    .InitiallySelectedItem(_StrategyOptions[static_cast<int32>(_Strategy)])
                    .OnGenerateWidget_Lambda([](TSharedPtr<ECk_AnimationToolbox_RewriteStrategy> Opt)
                    {
                        return SNew(STextBlock).Text(FText::FromString(ck::animation_editor::detail::StrategyToString(*Opt)));
                    })
                    .OnSelectionChanged(this, &SCkAnimationToolbox::DoOnStrategyChanged)
                    .Content()
                    [
                        SNew(STextBlock)
                        .Text_Lambda([this]()
                        {
                            return FText::FromString(ck::animation_editor::detail::StrategyToString(_Strategy));
                        })
                    ]
                ]
            ]
        ];
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkAnimationToolbox::
    DoCreateActionsPanel()
    -> TSharedRef<SWidget>
{
    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
        [
            SNew(SButton)
            .Text(LOCTEXT("ScanBtn", "Scan"))
            .ButtonStyle(FAppStyle::Get(), "PrimaryButton")
            .OnClicked(this, &SCkAnimationToolbox::DoOnScanClicked)
            .ToolTipText(LOCTEXT("ScanTip", "Evaluate selected scope and report affected animations."))
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
        [
            SNew(SButton)
            .Text(LOCTEXT("ApplySelBtn", "Apply to Selected"))
            .OnClicked(this, &SCkAnimationToolbox::DoOnApplySelectedClicked)
            .IsEnabled_Lambda([this]()
            {
                for (const auto& I : _Infos)
                { if (I.IsValid() && I->bSelectedForFix && I->Status == ECk_AnimationToolbox_Status::Affected) { return true; } }
                return false;
            })
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
        [
            SNew(SButton)
            .Text(LOCTEXT("ApplyAffBtn", "Apply to All Affected"))
            .OnClicked(this, &SCkAnimationToolbox::DoOnApplyAllAffectedClicked)
            .IsEnabled_Lambda([this]()
            {
                for (const auto& I : _Infos)
                { if (I.IsValid() && I->Status == ECk_AnimationToolbox_Status::Affected) { return true; } }
                return false;
            })
        ];
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkAnimationToolbox::
    DoCreateListPanel()
    -> TSharedRef<SWidget>
{
    using namespace ck::animation_editor::ColumnNames;

    _ListView = SNew(SListView<TSharedPtr<FCk_AnimationToolbox_AnimInfo>>)
        .ListItemsSource(&_Infos)
        .OnGenerateRow(this, &SCkAnimationToolbox::DoOnGenerateRow)
        .OnMouseButtonDoubleClick_Lambda([](TSharedPtr<FCk_AnimationToolbox_AnimInfo> Item)
        {
            if (Item.IsValid() && !Item->AnimPtr.IsNull())
            {
                if (UObject* LoadedAsset = Item->AnimPtr.LoadSynchronous())
                {
                    GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(LoadedAsset);
                }
            }
        })
        .SelectionMode(ESelectionMode::Single)
        .HeaderRow
        (
            SNew(SHeaderRow)
            + SHeaderRow::Column(Select)  .DefaultLabel(LOCTEXT("ColSelect",  "Fix?")) .FixedWidth(52)
            + SHeaderRow::Column(Status)  .DefaultLabel(LOCTEXT("ColStatus",  "Status")).FixedWidth(66)
            + SHeaderRow::Column(Asset)   .DefaultLabel(LOCTEXT("ColAsset",   "Asset")) .FillWidth(0.35f)
            + SHeaderRow::Column(Ratio)   .DefaultLabel(LOCTEXT("ColRatio",   "Δ₀/median")).FixedWidth(88)
            + SHeaderRow::Column(Frames)  .DefaultLabel(LOCTEXT("ColFrames",  "Frames")).FixedWidth(64)
            + SHeaderRow::Column(Notifies).DefaultLabel(LOCTEXT("ColNotifies","Notifies")).FixedWidth(72)
            + SHeaderRow::Column(Message) .DefaultLabel(LOCTEXT("ColMessage", "Detail")).FillWidth(0.35f)
        );

    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(2)
        [ _ListView.ToSharedRef() ];
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkAnimationToolbox::
    DoCreateStatusPanel()
    -> TSharedRef<SWidget>
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
        .Padding(6)
        [
            SAssignNew(_StatusText, STextBlock)
            .Text(LOCTEXT("StatusIdle", "Idle"))
        ];
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkAnimationToolbox::
    DoCreateLogPanel()
    -> TSharedRef<SWidget>
{
    SAssignNew(_LogBox, SVerticalBox);

    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
        .Padding(4)
        [
            SNew(SScrollBox)
            + SScrollBox::Slot()[ _LogBox.ToSharedRef() ]
        ];
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkAnimationToolbox::
    DoOnGenerateRow(TSharedPtr<FCk_AnimationToolbox_AnimInfo> Item, const TSharedRef<STableViewBase>& Owner)
    -> TSharedRef<ITableRow>
{
    return SNew(SCkAnimListRow, Owner)
        .Info(Item)
        .Owner(SharedThis(this));
}

// --------------------------------------------------------------------------------------------------------------------

auto SCkAnimationToolbox::DoOnScopeChanged(ECk_AnimationToolbox_ScanScope NewScope) -> void
{ _Options.Scope = NewScope; DoSaveSettings(); }

auto SCkAnimationToolbox::DoOnAnimBlueprintChanged(const FAssetData& NewAsset) -> void
{ _Options.AnimBlueprint = TSoftObjectPtr<UAnimBlueprint>(NewAsset.GetSoftObjectPath()); DoSaveSettings(); }

auto SCkAnimationToolbox::DoOnPathChanged(const FString& NewPath) -> void
{ _Options.ContentPath = NewPath; DoSaveSettings(); }

auto SCkAnimationToolbox::DoOnThresholdChanged(float NewValue) -> void
{ _Options.OutlierMultiplier = NewValue; DoSaveSettings(); }

auto SCkAnimationToolbox::DoOnStrategyChanged(TSharedPtr<ECk_AnimationToolbox_RewriteStrategy> NewValue, ESelectInfo::Type) -> void
{ if (NewValue.IsValid()) { _Strategy = *NewValue; DoSaveSettings(); } }

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkAnimationToolbox::
    DoOnScanClicked()
    -> FReply
{
    _Infos.Reset();
    Request_RefreshListView();
    DoAppendLog(TEXT("INFO"), FString::Printf(TEXT("Scan started (scope = %s)"), *UEnum::GetValueAsString(_Options.Scope)));

    FScopedSlowTask SlowTask(1000.0f, LOCTEXT("Scanning", "Scanning animations…"));
    SlowTask.MakeDialog(true);

    int32 LastTotal = 0;
    auto OnProgress = [&](int32 Index, int32 Total, const FString& Name) -> bool
    {
        if (LastTotal != Total)
        {
            LastTotal = Total;
        }
        const float PerTick = (Total > 0) ? (1000.0f / static_cast<float>(Total)) : 1000.0f;
        SlowTask.EnterProgressFrame(PerTick,
            FText::Format(LOCTEXT("ScanProgress", "[{0}/{1}] {2}"),
                FText::AsNumber(Index + 1), FText::AsNumber(Total), FText::FromString(Name)));
        return !SlowTask.ShouldCancel();
    };

    _Infos = _Detector.Request_Scan(_Options, OnProgress);

    int32 Affected = 0, Clean = 0, Skipped = 0;
    int32 WithUnmapped = 0;
    for (const auto& I : _Infos)
    {
        switch (I->Status)
        {
            case ECk_AnimationToolbox_Status::Affected: ++Affected; break;
            case ECk_AnimationToolbox_Status::Clean:    ++Clean;    break;
            case ECk_AnimationToolbox_Status::Skipped:  ++Skipped;  break;
            default: break;
        }
        if (I->UnmappedBoneCount > 0)
        { ++WithUnmapped; }
    }
    DoAppendLog(TEXT("INFO"),
        FString::Printf(TEXT("Scan complete: %d affected, %d clean, %d skipped"),
            Affected, Clean, Skipped));

    if (WithUnmapped > 0)
    {
        DoAppendLog(TEXT("INFO"),
            FString::Printf(TEXT("%d asset(s) have skeleton bones without tracks (virtual bones / IK joints — sampled from ref pose). Hover the Detail cell for bone names."),
                WithUnmapped));
    }

    Request_RefreshListView();
    DoUpdateStatusText();
    return FReply::Handled();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkAnimationToolbox::
    DoOnApplySelectedClicked()
    -> FReply
{
    TArray<TSharedPtr<FCk_AnimationToolbox_AnimInfo>> ToFix;
    for (const auto& I : _Infos)
    {
        if (I.IsValid() && I->bSelectedForFix && I->Status == ECk_AnimationToolbox_Status::Affected)
        { ToFix.Add(I); }
    }
    DoApplyFixToList(ToFix);
    return FReply::Handled();
}

auto
    SCkAnimationToolbox::
    DoOnApplyAllAffectedClicked()
    -> FReply
{
    TArray<TSharedPtr<FCk_AnimationToolbox_AnimInfo>> ToFix;
    for (const auto& I : _Infos)
    {
        if (I.IsValid() && I->Status == ECk_AnimationToolbox_Status::Affected)
        { ToFix.Add(I); }
    }
    DoApplyFixToList(ToFix);
    return FReply::Handled();
}

auto
    SCkAnimationToolbox::
    DoApplyFixToList(const TArray<TSharedPtr<FCk_AnimationToolbox_AnimInfo>>& ToFix)
    -> void
{
    if (ToFix.Num() == 0)
    { return; }

    DoAppendLog(TEXT("INFO"), FString::Printf(TEXT("Applying fix to %d animations (strategy: %s)"),
        ToFix.Num(), *ck::animation_editor::detail::StrategyToString(_Strategy)));

    FScopedSlowTask SlowTask(static_cast<float>(ToFix.Num()), LOCTEXT("FixingAnims", "Fixing animations…"));
    SlowTask.MakeDialog(true);

    int32 OK = 0, Fail = 0;
    for (int32 i = 0; i < ToFix.Num(); ++i)
    {
        if (SlowTask.ShouldCancel())
        {
            DoAppendLog(TEXT("WARN"), TEXT("Fix cancelled by user."));
            break;
        }

        const auto& Info = ToFix[i];
        SlowTask.EnterProgressFrame(1.0f,
            FText::Format(LOCTEXT("FixProgress", "[{0}/{1}] {2}"),
                FText::AsNumber(i + 1), FText::AsNumber(ToFix.Num()),
                FText::FromString(Info->AssetName)));

        const bool bOk = _Detector.Request_ApplyFix(Info, _Strategy);
        if (bOk)
        {
            ++OK;
            DoAppendLog(TEXT("INFO"), FString::Printf(TEXT("Fixed %s"), *Info->AssetName));
        }
        else
        {
            ++Fail;
            DoAppendLog(TEXT("ERROR"), FString::Printf(TEXT("Failed %s: %s"),
                *Info->AssetName, *Info->StatusMessage));
        }
    }

    DoAppendLog(TEXT("INFO"), FString::Printf(TEXT("Done: %d fixed, %d failed. Save affected assets to persist (Ctrl+S)."), OK, Fail));
    Request_RefreshListView();
    DoUpdateStatusText();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkAnimationToolbox::
    Request_RefreshListView()
    -> void
{
    if (_ListView.IsValid())
    { _ListView->RequestListRefresh(); }
}

auto
    SCkAnimationToolbox::
    DoUpdateStatusText()
    -> void
{
    if (NOT _StatusText.IsValid())
    { return; }

    int32 Affected = 0, Clean = 0, Fixed = 0, Skipped = 0, Failed = 0;
    for (const auto& I : _Infos)
    {
        switch (I->Status)
        {
            case ECk_AnimationToolbox_Status::Affected: ++Affected; break;
            case ECk_AnimationToolbox_Status::Clean:    ++Clean;    break;
            case ECk_AnimationToolbox_Status::Fixed:    ++Fixed;    break;
            case ECk_AnimationToolbox_Status::Skipped:  ++Skipped;  break;
            case ECk_AnimationToolbox_Status::Failed:   ++Failed;   break;
            default: break;
        }
    }

    _StatusText->SetText(FText::FromString(FString::Printf(
        TEXT("%d total   ·   %d affected   ·   %d clean   ·   %d fixed   ·   %d skipped   ·   %d failed"),
        _Infos.Num(), Affected, Clean, Fixed, Skipped, Failed)));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkAnimationToolbox::
    DoLoadSettings()
    -> void
{
    const auto* Settings = GetDefault<UCk_AnimationToolbox_DeveloperSettings>();
    _Options.Scope           = Settings->LastScope;
    _Options.AnimBlueprint   = Settings->LastAnimBlueprint;
    _Options.ContentPath     = Settings->LastPath.Path.IsEmpty() ? TEXT("/Game/") : Settings->LastPath.Path;
    _Options.OutlierMultiplier = Settings->OutlierMultiplier;
    _Options.bRequireLoopBoundaryMismatch = Settings->bRequireLoopBoundaryMismatch;
    _Strategy                = Settings->Strategy;
}

auto
    SCkAnimationToolbox::
    DoSaveSettings() const
    -> void
{
    auto* Settings = GetMutableDefault<UCk_AnimationToolbox_DeveloperSettings>();
    Settings->LastScope           = _Options.Scope;
    Settings->LastAnimBlueprint   = _Options.AnimBlueprint;
    Settings->LastPath.Path       = _Options.ContentPath;
    Settings->OutlierMultiplier   = _Options.OutlierMultiplier;
    Settings->bRequireLoopBoundaryMismatch = _Options.bRequireLoopBoundaryMismatch;
    Settings->Strategy            = _Strategy;
    Settings->SaveConfig();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkAnimationToolbox::
    DoAppendLog(const FString& InLevel, const FString& InMessage)
    -> void
{
    const FDateTime Now = FDateTime::Now();
    _LogEntries.Add({ InLevel, InMessage, Now });
    if (_LogEntries.Num() > 200)
    { _LogEntries.RemoveAt(0, _LogEntries.Num() - 200); }

    if (!_LogBox.IsValid())
    { return; }

    FSlateColor Color = FSlateColor::UseForeground();
    if (InLevel == TEXT("WARN"))
    { Color = FLinearColor(1.0f, 0.75f, 0.2f); }
    else if (InLevel == TEXT("ERROR"))
    { Color = FLinearColor(1.0f, 0.3f, 0.3f); }

    _LogBox->AddSlot().AutoHeight().Padding(2, 1)
    [
        SNew(STextBlock)
        .Text(FText::FromString(FString::Printf(TEXT("[%s] %s  %s"),
            *InLevel, *Now.ToString(TEXT("%H:%M:%S")), *InMessage)))
        .ColorAndOpacity(Color)
        .Font(FAppStyle::GetFontStyle("MonospacedText"))
    ];

    ck::animation_editor::Verbose(TEXT("[{}] {}"), InLevel, InMessage);
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
