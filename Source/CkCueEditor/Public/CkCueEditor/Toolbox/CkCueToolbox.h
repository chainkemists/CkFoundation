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

struct CKCUEEDITOR_API FCk_CueToolbox_CueInfo
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

struct CKCUEEDITOR_API FCk_CueToolbox_BlueprintCacheInfo
{
    FString BlueprintName;
    FString BlueprintPath;
    int32 CueNodeCount = 0;
    int32 CacheFixedCount = 0;
    TWeakObjectPtr<UBlueprint> Blueprint;

    FCk_CueToolbox_BlueprintCacheInfo() = default;
};

// --------------------------------------------------------------------------------------------------------------------

class CKCUEEDITOR_API FCkCueDiscovery
{
public:
    auto Request_DiscoverSubsystems() -> void;
    auto Request_RefreshCuesForSubsystem(UCk_CueSubsystem_Base_UE* InSubsystem) -> void;
    auto Request_ValidateCues() -> void;
    auto Request_ApplySearchFilter(const FString& InSearchText) -> void;
    auto Request_ScanBlueprintCaches() -> void;

    auto Get_DiscoveredSubsystems() const -> const TMap<FString, TWeakObjectPtr<UCk_CueSubsystem_Base_UE>>& { return _DiscoveredSubsystems; }
    auto Get_AllCueInfos() const -> const TArray<TSharedPtr<FCk_CueToolbox_CueInfo>>& { return _AllCueInfos; }
    auto Get_FilteredCueInfos() const -> const TArray<TSharedPtr<FCk_CueToolbox_CueInfo>>& { return _FilteredCueInfos; }
    auto Get_ValidationStats() const -> TTuple<int32, int32, int32> { return {_TotalCuesCount, _DuplicateCuesCount, _InvalidCuesCount}; }
    auto Get_BlueprintCacheInfos() const -> const TArray<TSharedPtr<FCk_CueToolbox_BlueprintCacheInfo>>& { return _BlueprintCacheInfos; }

private:
    TMap<FString, TWeakObjectPtr<UCk_CueSubsystem_Base_UE>> _DiscoveredSubsystems;
    TArray<TSharedPtr<FCk_CueToolbox_CueInfo>> _AllCueInfos;
    TArray<TSharedPtr<FCk_CueToolbox_CueInfo>> _FilteredCueInfos;
    TArray<TSharedPtr<FCk_CueToolbox_BlueprintCacheInfo>> _BlueprintCacheInfos;
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

class SCkCueToolbox;

class CKCUEEDITOR_API SCkBlueprintCacheListRow : public SMultiColumnTableRow<TSharedPtr<FCk_CueToolbox_BlueprintCacheInfo>>
{
public:
    SLATE_BEGIN_ARGS(SCkBlueprintCacheListRow) {}
        SLATE_ARGUMENT(TSharedPtr<FCk_CueToolbox_BlueprintCacheInfo>, CacheInfo)
        SLATE_ARGUMENT(TWeakPtr<SCkCueToolbox>, OwnerToolbox)
    SLATE_END_ARGS()

    auto Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView) -> void;
    auto GenerateWidgetForColumn(const FName& ColumnName) -> TSharedRef<SWidget> override;

private:
    auto DoOnLocateClicked() -> void;
    auto DoOnResaveClicked() -> void;

private:
    TSharedPtr<FCk_CueToolbox_BlueprintCacheInfo> _CacheInfo;
    TWeakPtr<SCkCueToolbox> _OwnerToolbox;
};

// --------------------------------------------------------------------------------------------------------------------

class CKCUEEDITOR_API SCkCueToolbox : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCkCueToolbox) {}
    SLATE_END_ARGS()

    auto Construct(const FArguments& InArgs) -> void;
    auto DoOnResaveEntry(const TSharedPtr<FCk_CueToolbox_BlueprintCacheInfo>& InEntry) -> void;

private:
    auto DoCreateHeaderPanel() -> TSharedRef<SWidget>;
    auto DoCreateSubsystemPanel() -> TSharedRef<SWidget>;
    auto DoCreateCueListPanel() -> TSharedRef<SWidget>;
    auto DoCreateValidationPanel() -> TSharedRef<SWidget>;
    auto DoCreateBlueprintCachePanel() -> TSharedRef<SWidget>;

    auto DoOnSubsystemChanged(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectionType) -> void;
    auto DoOnSearchTextChanged(const FText& InText) -> void;
    auto DoOnRefreshClicked() -> FReply;
    auto DoOnRefreshAllClicked() -> FReply;
    auto DoOnScanCachesClicked() -> FReply;
    auto DoOnResaveAllClicked() -> FReply;

    auto DoUpdateValidationDisplay() -> void;
    auto DoRefreshCueList() -> void;
    auto DoRefreshBlueprintCacheList() -> void;

private:
    TSharedPtr<FCkCueDiscovery> _CueDiscovery;

    TSharedPtr<SComboBox<TSharedPtr<FString>>> _SubsystemComboBox;
    TArray<TSharedPtr<FString>> _SubsystemOptions;

    TSharedPtr<SSearchBox> _SearchBox;
    TSharedPtr<SListView<TSharedPtr<FCk_CueToolbox_CueInfo>>> _CueListView;

    TSharedPtr<STextBlock> _ValidationText;

    TSharedPtr<SListView<TSharedPtr<FCk_CueToolbox_BlueprintCacheInfo>>> _BlueprintCacheListView;
    TSharedPtr<STextBlock> _BlueprintCacheStatusText;
};

// --------------------------------------------------------------------------------------------------------------------
