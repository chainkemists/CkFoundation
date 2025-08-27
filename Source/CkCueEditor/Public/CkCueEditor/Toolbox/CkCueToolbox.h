#pragma once

#include "CkCue/CkCueSubsystem_Base.h"
#include "CkCueToolboxStyle.h"

#include <Widgets/SCompoundWidget.h>
#include <Widgets/Views/SListView.h>
#include <Widgets/Input/SComboBox.h>
#include <Widgets/Input/SSearchBox.h>
#include <Widgets/Text/STextBlock.h>
#include <Widgets/Input/SButton.h>
#include <Widgets/Layout/SBorder.h>

// --------------------------------------------------------------------------------------------------------------------

struct CKCUE_API FCk_CueToolbox_CueInfo
{
    FGameplayTag CueName;
    TSubclassOf<UCk_CueBase_EntityScript> CueClass;
    FString SubsystemName;
    bool IsValid = true;
    FString ValidationMessage;

    FCk_CueToolbox_CueInfo() = default;
    FCk_CueToolbox_CueInfo(const FGameplayTag& InCueName, TSubclassOf<UCk_CueBase_EntityScript> InCueClass, const FString& InSubsystemName)
        : CueName(InCueName), CueClass(InCueClass), SubsystemName(InSubsystemName) {}
};

// --------------------------------------------------------------------------------------------------------------------

class CKCUEEDITOR_API FCkCueDiscovery
{
public:
    auto Request_DiscoverSubsystems() -> void;
    auto Request_RefreshCuesForSubsystem(UCk_CueSubsystem_Base_UE* InSubsystem) -> void;
    auto Request_ValidateCues() -> void;
    auto Request_ApplySearchFilter(const FString& InSearchText) -> void;

    auto Get_DiscoveredSubsystems() const -> const TMap<FString, TWeakObjectPtr<UCk_CueSubsystem_Base_UE>>& { return _DiscoveredSubsystems; }
    auto Get_AllCueInfos() const -> const TArray<TSharedPtr<FCk_CueToolbox_CueInfo>>& { return _AllCueInfos; }
    auto Get_FilteredCueInfos() const -> const TArray<TSharedPtr<FCk_CueToolbox_CueInfo>>& { return _FilteredCueInfos; }
    auto Get_ValidationStats() const -> TTuple<int32, int32, int32> { return {_TotalCuesCount, _DuplicateCuesCount, _InvalidCuesCount}; }

private:
    TMap<FString, TWeakObjectPtr<UCk_CueSubsystem_Base_UE>> _DiscoveredSubsystems;
    TArray<TSharedPtr<FCk_CueToolbox_CueInfo>> _AllCueInfos;
    TArray<TSharedPtr<FCk_CueToolbox_CueInfo>> _FilteredCueInfos;
    int32 _TotalCuesCount = 0;
    int32 _DuplicateCuesCount = 0;
    int32 _InvalidCuesCount = 0;
};

// --------------------------------------------------------------------------------------------------------------------

class CKCUEEDITOR_API SCkCueListRow : public SMultiColumnTableRow<TSharedPtr<FCk_CueToolbox_CueInfo>>
{
public:
    SLATE_BEGIN_ARGS(SCkCueListRow) {}
        SLATE_ARGUMENT(TSharedPtr<FCk_CueToolbox_CueInfo>, CueInfo)
    SLATE_END_ARGS()

    auto Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView) -> void;
    auto GenerateWidgetForColumn(const FName& ColumnName) -> TSharedRef<SWidget> override;

private:
    auto DoOnEditClicked() -> void;
    auto DoOnLocateClicked() -> void;

private:
    TSharedPtr<FCk_CueToolbox_CueInfo> _CueInfo;
};

// --------------------------------------------------------------------------------------------------------------------

class CKCUEEDITOR_API SCkCueToolbox : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCkCueToolbox) {}
    SLATE_END_ARGS()

    auto Construct(const FArguments& InArgs) -> void;

private:
    auto DoCreateHeaderPanel() -> TSharedRef<SWidget>;
    auto DoCreateSubsystemPanel() -> TSharedRef<SWidget>;
    auto DoCreateCueListPanel() -> TSharedRef<SWidget>;
    auto DoCreateValidationPanel() -> TSharedRef<SWidget>;

    auto DoOnSubsystemChanged(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectionType) -> void;
    auto DoOnSearchTextChanged(const FText& InText) -> void;
    auto DoOnRefreshClicked() -> FReply;
    auto DoOnRefreshAllClicked() -> FReply;

    auto DoUpdateValidationDisplay() -> void;
    auto DoRefreshCueList() -> void;

private:
    TSharedPtr<FCkCueDiscovery> _CueDiscovery;

    TSharedPtr<SComboBox<TSharedPtr<FString>>> _SubsystemComboBox;
    TArray<TSharedPtr<FString>> _SubsystemOptions;

    TSharedPtr<SSearchBox> _SearchBox;
    TSharedPtr<SListView<TSharedPtr<FCk_CueToolbox_CueInfo>>> _CueListView;

    TSharedPtr<STextBlock> _ValidationText;
};

// --------------------------------------------------------------------------------------------------------------------
