#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

#include <Widgets/SCompoundWidget.h>
#include <Widgets/Views/SListView.h>
#include <Widgets/Input/SSearchBox.h>
#include <Widgets/Text/STextBlock.h>
#include <Widgets/Input/SButton.h>
#include <Widgets/Layout/SBorder.h>
#include <Widgets/Layout/SBox.h>
#include <Widgets/Layout/SExpandableArea.h>

// --------------------------------------------------------------------------------------------------------------------

struct CKECSEDITOR_API FCk_SpawnParamsToolbox_PropertyInfo
{
	FString Name;
	FString TypeName;
	int32 Index = -1;

	FCk_SpawnParamsToolbox_PropertyInfo() = default;
	FCk_SpawnParamsToolbox_PropertyInfo(const FString& InName, const FString& InTypeName, int32 InIndex)
		: Name(InName), TypeName(InTypeName), Index(InIndex) {}
};

// --------------------------------------------------------------------------------------------------------------------

struct CKECSEDITOR_API FCk_SpawnParamsToolbox_EntryInfo
{
	FString EntityScriptName;
	FString EntityScriptPath;
	FString StructPath;
	TArray<FString> DuplicatePaths;
	bool HasMismatch = false;
	bool HasDuplicate = false;
	bool HasStruct = false;
	bool IsInspected = false;
	bool IsBlueprint = false;
	FString DiagnosticMessage;
	TWeakObjectPtr<UClass> EntityScriptClass;

	TArray<FCk_SpawnParamsToolbox_PropertyInfo> ScriptProperties;
	TArray<FCk_SpawnParamsToolbox_PropertyInfo> StructProperties;

	FAssetData ScriptAssetData;
	FAssetData StructAssetData;

	FCk_SpawnParamsToolbox_EntryInfo() = default;

	auto Get_HasIssue() const -> bool { return HasMismatch || HasDuplicate || NOT HasStruct; }
};

// --------------------------------------------------------------------------------------------------------------------

class CKECSEDITOR_API FCkEntityScriptSpawnParamsDiscovery
{
public:
	auto Request_DiscoverAll() -> void;
	auto Request_CheckMismatches() -> void;
	auto Request_CheckDuplicates() -> void;
	auto Request_RecreateAll() -> void;
	auto Request_InspectEntry(const TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>& InEntry) -> void;
	auto Request_InspectAll() -> void;
	auto Request_CreateOrRecreateEntry(const TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>& InEntry) -> void;
	auto Request_ApplySearchFilter(const FString& InSearchText) -> void;

	auto Get_AllEntries() const -> const TArray<TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>>& { return _AllEntries; }
	auto Get_FilteredEntries() const -> const TArray<TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>>& { return _FilteredEntries; }
	auto Get_Stats() const -> TTuple<int32, int32, int32, int32> { return {_TotalCount, _MismatchCount, _DuplicateCount, _MissingStructCount}; }

private:
	auto DoPopulatePropertyInfo(const TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>& InEntry) -> void;
	auto DoCheckEntryMismatch(const TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>& InEntry) -> bool;
	auto DoFindBlueprintAssetData(UClass* InClass) -> FAssetData;
	auto DoFindStructAssetData(const FString& InStructPath) -> FAssetData;
	auto DoRefreshSingleEntry(const TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>& InEntry) -> void;

private:
	TArray<TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>> _AllEntries;
	TArray<TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>> _FilteredEntries;
	int32 _TotalCount = 0;
	int32 _MismatchCount = 0;
	int32 _DuplicateCount = 0;
	int32 _MissingStructCount = 0;
};

// --------------------------------------------------------------------------------------------------------------------

class SCkEntityScriptSpawnParamsToolbox;

class CKECSEDITOR_API SCkEntityScriptSpawnParamsListRow
	: public SMultiColumnTableRow<TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>>
{
public:
	SLATE_BEGIN_ARGS(SCkEntityScriptSpawnParamsListRow) {}
		SLATE_ARGUMENT(TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>, EntryInfo)
		SLATE_ARGUMENT(TSharedPtr<FCkEntityScriptSpawnParamsDiscovery>, Discovery)
		SLATE_ARGUMENT(TWeakPtr<SCkEntityScriptSpawnParamsToolbox>, OwnerToolbox)
	SLATE_END_ARGS()

	auto Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView) -> void;
	auto GenerateWidgetForColumn(const FName& ColumnName) -> TSharedRef<SWidget> override;

private:
	static auto DoLocateAsset(const FAssetData& InAssetData) -> void;
	static auto DoEditAsset(const FAssetData& InAssetData) -> void;

	auto DoCreatePropertyComparisonWidget() -> TSharedRef<SWidget>;

private:
	TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo> _EntryInfo;
	TSharedPtr<FCkEntityScriptSpawnParamsDiscovery> _Discovery;
	TWeakPtr<SCkEntityScriptSpawnParamsToolbox> _OwnerToolbox;
	TSharedPtr<SExpandableArea> _ExpandableArea;
	TSharedPtr<SBox> _BodyBox;
};

// --------------------------------------------------------------------------------------------------------------------

class CKECSEDITOR_API SCkEntityScriptSpawnParamsToolbox : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCkEntityScriptSpawnParamsToolbox) {}
	SLATE_END_ARGS()

	auto Construct(const FArguments& InArgs) -> void;
	auto DoOnCreateOrRecreateEntry(const TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>& InEntry) -> void;

private:
	auto DoCreateButtonBar() -> TSharedRef<SWidget>;
	auto DoCreateStatusBar() -> TSharedRef<SWidget>;
	auto DoCreateListPanel() -> TSharedRef<SWidget>;

	auto DoOnDiscoverClicked() -> FReply;
	auto DoOnCheckMismatchesClicked() -> FReply;
	auto DoOnCheckDuplicatesClicked() -> FReply;
	auto DoOnRecreateAllClicked() -> FReply;
	auto DoOnInspectAllClicked() -> FReply;
	auto DoOnSearchTextChanged(const FText& InText) -> void;

	auto DoRefreshList() -> void;
	auto DoUpdateStatusBar() -> void;

private:
	TSharedPtr<FCkEntityScriptSpawnParamsDiscovery> _Discovery;

	TSharedPtr<SSearchBox> _SearchBox;
	TSharedPtr<SListView<TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>>> _ListView;

	TSharedPtr<STextBlock> _StatusText_Total;
	TSharedPtr<STextBlock> _StatusText_Mismatches;
	TSharedPtr<STextBlock> _StatusText_Duplicates;
	TSharedPtr<STextBlock> _StatusText_MissingStructs;
};

// --------------------------------------------------------------------------------------------------------------------
