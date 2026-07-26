#pragma once

#include "CkAnimationToolbox_Common.h"
#include "CkAnimationToolbox_Detector.h"
#include "CkAnimationToolbox_Style.h"

#include <CoreMinimal.h>
#include <Widgets/SCompoundWidget.h>
#include <Widgets/Views/SListView.h>
#include <Widgets/Views/STableRow.h>
#include <Widgets/Input/SComboBox.h>
#include <Widgets/Input/SSpinBox.h>

// --------------------------------------------------------------------------------------------------------------------

class SCkAnimationToolbox;

// --------------------------------------------------------------------------------------------------------------------

class CKANIMATIONEDITOR_API SCkAnimListRow : public SMultiColumnTableRow<TSharedPtr<FCk_AnimationToolbox_AnimInfo>>
{
public:
    SLATE_BEGIN_ARGS(SCkAnimListRow) {}
        SLATE_ARGUMENT(TSharedPtr<FCk_AnimationToolbox_AnimInfo>, Info)
        SLATE_ARGUMENT(TWeakPtr<SCkAnimationToolbox>, Owner)
    SLATE_END_ARGS()

    auto Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView) -> void;
    auto GenerateWidgetForColumn(const FName& ColumnName) -> TSharedRef<SWidget> override;

private:
    auto DoOnCheckStateChanged(ECheckBoxState NewState) -> void;
    auto DoGetCheckState() const -> ECheckBoxState;

private:
    TSharedPtr<FCk_AnimationToolbox_AnimInfo> _Info;
    TWeakPtr<SCkAnimationToolbox> _Owner;
};

// --------------------------------------------------------------------------------------------------------------------

class CKANIMATIONEDITOR_API SCkAnimationToolbox : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCkAnimationToolbox) {}
    SLATE_END_ARGS()

    auto Construct(const FArguments& InArgs) -> void;

    auto Get_Infos() const -> const TArray<TSharedPtr<FCk_AnimationToolbox_AnimInfo>>& { return _Infos; }
    auto Request_RefreshListView() -> void;

private:
    auto DoCreateOptionsPanel() -> TSharedRef<SWidget>;
    auto DoCreateActionsPanel() -> TSharedRef<SWidget>;
    auto DoCreateListPanel() -> TSharedRef<SWidget>;
    auto DoCreateStatusPanel() -> TSharedRef<SWidget>;
    auto DoCreateLogPanel() -> TSharedRef<SWidget>;

    auto DoOnScopeChanged(ECk_AnimationToolbox_ScanScope NewScope) -> void;
    auto DoOnAnimBlueprintChanged(const FAssetData& NewAsset) -> void;
    auto DoOnPathChanged(const FString& NewPath) -> void;
    auto DoOnThresholdChanged(float NewValue) -> void;
    auto DoOnStrategyChanged(TSharedPtr<ECk_AnimationToolbox_RewriteStrategy> NewValue, ESelectInfo::Type Type) -> void;
    auto DoOnScanClicked() -> FReply;
    auto DoOnApplySelectedClicked() -> FReply;
    auto DoOnApplyAllAffectedClicked() -> FReply;

    auto DoOnGenerateRow(TSharedPtr<FCk_AnimationToolbox_AnimInfo> Item, const TSharedRef<STableViewBase>& Owner) -> TSharedRef<ITableRow>;

    auto DoLoadSettings() -> void;
    auto DoSaveSettings() const -> void;
    auto DoAppendLog(const FString& InLevel, const FString& InMessage) -> void;
    auto DoUpdateStatusText() -> void;

    auto DoApplyFixToList(const TArray<TSharedPtr<FCk_AnimationToolbox_AnimInfo>>& ToFix) -> void;

private:
    FCk_AnimationToolbox_ScanOptions _Options;
    ECk_AnimationToolbox_RewriteStrategy _Strategy = ECk_AnimationToolbox_RewriteStrategy::CopyFromLastFrame;

    TArray<TSharedPtr<FCk_AnimationToolbox_AnimInfo>> _Infos;

    TSharedPtr<SListView<TSharedPtr<FCk_AnimationToolbox_AnimInfo>>> _ListView;
    TSharedPtr<STextBlock> _StatusText;

    TArray<TSharedPtr<ECk_AnimationToolbox_RewriteStrategy>> _StrategyOptions;
    TSharedPtr<SComboBox<TSharedPtr<ECk_AnimationToolbox_RewriteStrategy>>> _StrategyCombo;

    struct FLogEntry
    {
        FString Level;
        FString Message;
        FDateTime When;
    };
    TArray<FLogEntry> _LogEntries;
    TSharedPtr<SVerticalBox> _LogBox;

    FCkAnimScrunchDetector _Detector;
};

// --------------------------------------------------------------------------------------------------------------------
