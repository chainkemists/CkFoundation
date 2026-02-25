#include "CkEntityScriptSpawnParamsToolbox.h"

#include "CkCore/IO/CkIO_Utils.h"
#include "CkCore/Reflection/CkReflection_Utils.h"
#include "CkEcs/Subsystem/CkEntityScript_Subsystem.h"

#include <AssetRegistry/AssetRegistryModule.h>
#include <ContentBrowserModule.h>
#include <IContentBrowserSingleton.h>
#include <Kismet2/StructureEditorUtils.h>
#include <Subsystems/AssetEditorSubsystem.h>
#include <Widgets/Layout/SBox.h>
#include <Widgets/Layout/SSeparator.h>
#include <Styling/AppStyle.h>

// --------------------------------------------------------------------------------------------------------------------

namespace SpawnParamsToolbox_Constants
{
	constexpr float PanelPadding = 8.0f;
	constexpr float SectionSpacing = 8.0f;
	constexpr float ButtonWidth = 140.0f;
	constexpr float StatusChipSpacing = 12.0f;

	const FLinearColor Color_Success{0.2f, 0.8f, 0.2f};
	const FLinearColor Color_Warning{1.0f, 0.6f, 0.0f};
	const FLinearColor Color_Danger{0.9f, 0.2f, 0.2f};
	const FLinearColor Color_Info{0.3f, 0.6f, 1.0f};
	const FLinearColor Color_Muted{0.5f, 0.5f, 0.5f};
	const FLinearColor Color_Blueprint{0.4f, 0.7f, 1.0f};
	const FLinearColor Color_Cpp{0.7f, 0.7f, 0.7f};
}

// --------------------------------------------------------------------------------------------------------------------
// Discovery
// --------------------------------------------------------------------------------------------------------------------

auto FCkEntityScriptSpawnParamsDiscovery::DoFindBlueprintAssetData(UClass* InClass) -> FAssetData
{
	if (ck::Is_NOT_Valid(InClass))
	{ return {}; }

	auto* AssetRegistry = IAssetRegistry::Get();

	if (ck::Is_NOT_Valid(AssetRegistry, ck::IsValid_Policy_NullptrOnly{}))
	{ return {}; }

	const auto PackageName = InClass->GetPackage()->GetName();

	auto Filter = FARFilter{};
	Filter.PackageNames.Add(*PackageName);
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());

	auto Assets = TArray<FAssetData>{};
	AssetRegistry->GetAssets(Filter, Assets);

	if (Assets.Num() > 0)
	{ return Assets[0]; }

	return {};
}

auto FCkEntityScriptSpawnParamsDiscovery::DoFindStructAssetData(const FString& InStructPath) -> FAssetData
{
	auto* AssetRegistry = IAssetRegistry::Get();

	if (ck::Is_NOT_Valid(AssetRegistry, ck::IsValid_Policy_NullptrOnly{}))
	{ return {}; }

	auto Filter = FARFilter{};
	Filter.PackageNames.Add(*InStructPath);
	Filter.ClassPaths.Add(UUserDefinedStruct::StaticClass()->GetClassPathName());

	auto Assets = TArray<FAssetData>{};
	AssetRegistry->GetAssets(Filter, Assets);

	if (Assets.Num() > 0)
	{ return Assets[0]; }

	return {};
}

auto FCkEntityScriptSpawnParamsDiscovery::DoPopulatePropertyInfo(
	const TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>& InEntry)
	-> void
{
	InEntry->ScriptProperties.Empty();
	InEntry->StructProperties.Empty();

	auto* EntityScriptClass = InEntry->EntityScriptClass.Get();

	if (ck::Is_NOT_Valid(EntityScriptClass))
	{ return; }

	const auto& ExposedProperties = UCk_Utils_Reflection_UE::Get_ExposedPropertiesOfClass(EntityScriptClass);

	auto ScriptIndex = 0;
	for (const auto* Prop : ExposedProperties)
	{
		InEntry->ScriptProperties.Emplace(
			Prop->GetName(),
			Prop->GetCPPType(),
			ScriptIndex++);
	}

	if (NOT InEntry->HasStruct)
	{ return; }

	auto* SpawnParamsStruct = LoadObject<UUserDefinedStruct>(nullptr, *InEntry->StructPath);

	if (ck::Is_NOT_Valid(SpawnParamsStruct))
	{ return; }

	const auto& VarDescs = FStructureEditorUtils::GetVarDesc(SpawnParamsStruct);

	auto StructIndex = 0;
	for (const auto& VarDesc : VarDescs)
	{
		const auto* Prop = FStructureEditorUtils::GetPropertyByGuid(SpawnParamsStruct, VarDesc.VarGuid);
		const auto TypeName = ck::IsValid(Prop, ck::IsValid_Policy_NullptrOnly{}) ? Prop->GetCPPType() : TEXT("Unknown");

		InEntry->StructProperties.Emplace(
			VarDesc.FriendlyName,
			TypeName,
			StructIndex++);
	}
}

auto FCkEntityScriptSpawnParamsDiscovery::Request_DiscoverAll() -> void
{
	_AllEntries.Empty();
	_TotalCount = 0;
	_MismatchCount = 0;
	_DuplicateCount = 0;
	_MissingStructCount = 0;

	auto* Subsystem = GEngine->GetEngineSubsystem<UCk_EntityScript_Subsystem_UE>();

	for (auto It = TObjectIterator<UClass>{}; It; ++It)
	{
		auto* Class = *It;

		if (NOT Class->IsChildOf(UCk_EntityScript_UE::StaticClass()))
		{ continue; }

		if (Class == UCk_EntityScript_UE::StaticClass())
		{ continue; }

		if (Class->HasAnyClassFlags(CLASS_Abstract))
		{ continue; }

		if (UCk_Utils_IO_UE::Get_IsTemporaryAsset(Class->GetName()))
		{ continue; }

		const auto StructName = UCk_EntityScript_Subsystem_UE::GenerateEntitySpawnParamsStructName(Class);
		const auto StructPackagePath = Subsystem->Get_StructPathForEntityScriptPath(Class->GetPackage()->GetName());
		const auto StructFullPath = StructPackagePath / StructName.ToString();

		auto Entry = MakeShared<FCk_SpawnParamsToolbox_EntryInfo>();
		Entry->EntityScriptName = Class->GetName();
		Entry->EntityScriptPath = Class->GetPackage()->GetName();
		Entry->EntityScriptClass = Class;
		Entry->IsBlueprint = Class->HasAnyClassFlags(CLASS_CompiledFromBlueprint);
		Entry->ScriptAssetData = DoFindBlueprintAssetData(Class);

		if (auto* ExistingStruct = LoadObject<UUserDefinedStruct>(nullptr, *StructFullPath);
			ck::IsValid(ExistingStruct))
		{
			Entry->HasStruct = true;
			Entry->StructPath = StructFullPath;
			Entry->StructAssetData = DoFindStructAssetData(StructFullPath);
		}
		else
		{
			_MissingStructCount++;
		}

		_AllEntries.Add(Entry);
	}

	_TotalCount = _AllEntries.Num();

	_AllEntries.Sort([](const TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>& A, const TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>& B)
	{
		return A->EntityScriptName < B->EntityScriptName;
	});

	_FilteredEntries = _AllEntries;
}

auto FCkEntityScriptSpawnParamsDiscovery::DoCheckEntryMismatch(
	const TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>& InEntry)
	-> bool
{
	auto* EntityScriptClass = InEntry->EntityScriptClass.Get();

	if (ck::Is_NOT_Valid(EntityScriptClass))
	{ return false; }

	auto* SpawnParamsStruct = LoadObject<UUserDefinedStruct>(nullptr, *InEntry->StructPath);

	if (ck::Is_NOT_Valid(SpawnParamsStruct))
	{ return false; }

	const auto& ExposedProperties = UCk_Utils_Reflection_UE::Get_ExposedPropertiesOfClass(EntityScriptClass);
	const auto& VarDescs = FStructureEditorUtils::GetVarDesc(SpawnParamsStruct);

	if (ExposedProperties.IsEmpty() && VarDescs.Num() == 1)
	{ return false; }

	if (ExposedProperties.Num() != VarDescs.Num())
	{ return true; }

	auto ExistingByName = TMap<FName, FGuid>{};
	for (const auto& VarDesc : VarDescs)
	{
		ExistingByName.Add(*VarDesc.FriendlyName, VarDesc.VarGuid);
	}

	for (const auto* ExpProp : ExposedProperties)
	{
		const auto& PropName = ExpProp->GetFName();
		const auto* FoundGuid = ExistingByName.Find(PropName);

		if (ck::Is_NOT_Valid(FoundGuid, ck::IsValid_Policy_NullptrOnly{}))
		{ return true; }

		const auto* ExistingProperty = FStructureEditorUtils::GetPropertyByGuid(SpawnParamsStruct, *FoundGuid);

		if (ck::Is_NOT_Valid(ExistingProperty, ck::IsValid_Policy_NullptrOnly{}))
		{ return true; }

		if (NOT UCk_Utils_Reflection_UE::Get_ArePropertiesCompatible(ExistingProperty, ExpProp))
		{ return true; }
	}

	return false;
}

auto FCkEntityScriptSpawnParamsDiscovery::Request_CheckMismatches() -> void
{
	_MismatchCount = 0;

	for (const auto& Entry : _AllEntries)
	{
		if (NOT Entry.IsValid() || NOT Entry->HasStruct)
		{ continue; }

		Entry->HasMismatch = false;
		Entry->DiagnosticMessage.Empty();

		if (DoCheckEntryMismatch(Entry))
		{
			Entry->HasMismatch = true;
			_MismatchCount++;

			auto* EntityScriptClass = Entry->EntityScriptClass.Get();
			const auto& ExposedProperties = UCk_Utils_Reflection_UE::Get_ExposedPropertiesOfClass(EntityScriptClass);

			auto ExposedNames = TArray<FString>{};
			for (const auto* Prop : ExposedProperties)
			{
				ExposedNames.Add(Prop->GetName());
			}

			auto* SpawnParamsStruct = LoadObject<UUserDefinedStruct>(nullptr, *Entry->StructPath);
			const auto& VarDescs = FStructureEditorUtils::GetVarDesc(SpawnParamsStruct);

			auto StructNames = TArray<FString>{};
			for (const auto& VarDesc : VarDescs)
			{
				StructNames.Add(VarDesc.FriendlyName);
			}

			Entry->DiagnosticMessage = FString::Printf(
				TEXT("Mismatch: Script has [%s], Struct has [%s]"),
				*FString::Join(ExposedNames, TEXT(", ")),
				*FString::Join(StructNames, TEXT(", ")));
		}
	}
}

auto FCkEntityScriptSpawnParamsDiscovery::Request_CheckDuplicates() -> void
{
	_DuplicateCount = 0;

	auto* AssetRegistry = IAssetRegistry::Get();

	if (ck::Is_NOT_Valid(AssetRegistry, ck::IsValid_Policy_NullptrOnly{}))
	{ return; }

	auto StructAssets = TArray<FAssetData>{};
	auto Filter = FARFilter{};
	Filter.ClassPaths.Add(UUserDefinedStruct::StaticClass()->GetClassPathName());
	AssetRegistry->GetAssets(Filter, StructAssets);

	const auto& Prefix = UCk_EntityScript_Subsystem_UE::_SpawnParamsStructName_Prefix;

	auto AssetsByName = TMap<FName, TArray<FString>>{};

	for (const auto& AssetData : StructAssets)
	{
		const auto AssetName = AssetData.AssetName;

		if (NOT AssetName.ToString().StartsWith(Prefix))
		{ continue; }

		AssetsByName.FindOrAdd(AssetName).Add(AssetData.GetObjectPathString());
	}

	for (const auto& Entry : _AllEntries)
	{
		if (NOT Entry.IsValid())
		{ continue; }

		Entry->HasDuplicate = false;
		Entry->DuplicatePaths.Empty();

		auto* EntityScriptClass = Entry->EntityScriptClass.Get();

		if (ck::Is_NOT_Valid(EntityScriptClass))
		{ continue; }

		const auto StructName = UCk_EntityScript_Subsystem_UE::GenerateEntitySpawnParamsStructName(EntityScriptClass);

		if (const auto* FoundPaths = AssetsByName.Find(StructName);
			ck::IsValid(FoundPaths, ck::IsValid_Policy_NullptrOnly{}) && FoundPaths->Num() > 1)
		{
			Entry->HasDuplicate = true;
			Entry->DuplicatePaths = *FoundPaths;
			_DuplicateCount++;

			const auto DuplicateMessage = FString::Printf(
				TEXT("Duplicate: found in %d locations: %s"),
				FoundPaths->Num(),
				*FString::Join(*FoundPaths, TEXT(", ")));

			if (Entry->DiagnosticMessage.IsEmpty())
			{
				Entry->DiagnosticMessage = DuplicateMessage;
			}
			else
			{
				Entry->DiagnosticMessage += TEXT("; ") + DuplicateMessage;
			}
		}
	}
}

auto FCkEntityScriptSpawnParamsDiscovery::Request_RecreateAll() -> void
{
	auto* Subsystem = GEngine->GetEngineSubsystem<UCk_EntityScript_Subsystem_UE>();

	if (ck::Is_NOT_Valid(Subsystem))
	{ return; }

	for (const auto& Entry : _AllEntries)
	{
		if (NOT Entry.IsValid())
		{ continue; }

		auto* EntityScriptClass = Entry->EntityScriptClass.Get();

		if (ck::Is_NOT_Valid(EntityScriptClass))
		{ continue; }

		constexpr auto ForceRecreate = true;
		Subsystem->GetOrCreate_SpawnParamsStructForEntity(EntityScriptClass, ForceRecreate);
	}

	Request_DiscoverAll();
	Request_CheckMismatches();
	Request_CheckDuplicates();
}

auto FCkEntityScriptSpawnParamsDiscovery::Request_InspectEntry(
	const TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>& InEntry)
	-> void
{
	if (NOT InEntry.IsValid())
	{ return; }

	DoPopulatePropertyInfo(InEntry);
	InEntry->IsInspected = true;
}

auto FCkEntityScriptSpawnParamsDiscovery::Request_InspectAll() -> void
{
	for (const auto& Entry : _AllEntries)
	{
		if (NOT Entry.IsValid())
		{ continue; }

		DoPopulatePropertyInfo(Entry);
		Entry->IsInspected = true;
	}
}

auto FCkEntityScriptSpawnParamsDiscovery::Request_CreateOrRecreateEntry(
	const TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>& InEntry)
	-> void
{
	if (NOT InEntry.IsValid())
	{ return; }

	auto* EntityScriptClass = InEntry->EntityScriptClass.Get();

	if (ck::Is_NOT_Valid(EntityScriptClass))
	{ return; }

	auto* Subsystem = GEngine->GetEngineSubsystem<UCk_EntityScript_Subsystem_UE>();

	if (ck::Is_NOT_Valid(Subsystem))
	{ return; }

	constexpr auto ForceRecreate = true;
	Subsystem->GetOrCreate_SpawnParamsStructForEntity(EntityScriptClass, ForceRecreate);

	DoRefreshSingleEntry(InEntry);
}

auto FCkEntityScriptSpawnParamsDiscovery::DoRefreshSingleEntry(
	const TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>& InEntry)
	-> void
{
	if (NOT InEntry.IsValid())
	{ return; }

	auto* EntityScriptClass = InEntry->EntityScriptClass.Get();

	if (ck::Is_NOT_Valid(EntityScriptClass))
	{ return; }

	auto* Subsystem = GEngine->GetEngineSubsystem<UCk_EntityScript_Subsystem_UE>();

	const auto StructName = UCk_EntityScript_Subsystem_UE::GenerateEntitySpawnParamsStructName(EntityScriptClass);
	const auto StructPackagePath = Subsystem->Get_StructPathForEntityScriptPath(EntityScriptClass->GetPackage()->GetName());
	const auto StructFullPath = StructPackagePath / StructName.ToString();

	const auto WasMissing = NOT InEntry->HasStruct;

	InEntry->HasStruct = false;
	InEntry->StructPath.Empty();
	InEntry->StructAssetData = {};
	InEntry->HasMismatch = false;
	InEntry->DiagnosticMessage.Empty();

	if (auto* ExistingStruct = LoadObject<UUserDefinedStruct>(nullptr, *StructFullPath);
		ck::IsValid(ExistingStruct))
	{
		InEntry->HasStruct = true;
		InEntry->StructPath = StructFullPath;
		InEntry->StructAssetData = DoFindStructAssetData(StructFullPath);

		if (DoCheckEntryMismatch(InEntry))
		{
			InEntry->HasMismatch = true;
		}
	}

	if (WasMissing && InEntry->HasStruct)
	{
		_MissingStructCount = FMath::Max(0, _MissingStructCount - 1);
	}

	if (InEntry->IsInspected)
	{
		DoPopulatePropertyInfo(InEntry);
	}

	_MismatchCount = 0;
	for (const auto& Entry : _AllEntries)
	{
		if (Entry.IsValid() && Entry->HasMismatch)
		{
			_MismatchCount++;
		}
	}
}

auto FCkEntityScriptSpawnParamsDiscovery::Request_ApplySearchFilter(const FString& InSearchText) -> void
{
	_FilteredEntries.Empty();

	if (InSearchText.IsEmpty())
	{
		_FilteredEntries = _AllEntries;
		return;
	}

	const auto SearchTextLower = InSearchText.ToLower();

	for (const auto& Entry : _AllEntries)
	{
		if (NOT Entry.IsValid())
		{ continue; }

		if (Entry->EntityScriptName.ToLower().Contains(SearchTextLower) ||
			Entry->StructPath.ToLower().Contains(SearchTextLower) ||
			Entry->DiagnosticMessage.ToLower().Contains(SearchTextLower))
		{
			_FilteredEntries.Add(Entry);
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------
// List Row
// --------------------------------------------------------------------------------------------------------------------

auto SCkEntityScriptSpawnParamsListRow::Construct(
	const FArguments& InArgs,
	const TSharedRef<STableViewBase>& InOwnerTableView)
	-> void
{
	_EntryInfo = InArgs._EntryInfo;
	_Discovery = InArgs._Discovery;
	_OwnerToolbox = InArgs._OwnerToolbox;

	SMultiColumnTableRow<TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>>::Construct(
		FSuperRowType::FArguments()
			.Padding(1.0f),
		InOwnerTableView);
}

auto SCkEntityScriptSpawnParamsListRow::GenerateWidgetForColumn(const FName& ColumnName) -> TSharedRef<SWidget>
{
	if (NOT _EntryInfo.IsValid())
	{
		return SNew(STextBlock).Text(FText::FromString(TEXT("Invalid")));
	}

	// ---- Status Column ----
	if (ColumnName == TEXT("Status"))
	{
		auto StatusText = FString{};
		auto StatusColor = FLinearColor{};

		if (_EntryInfo->HasMismatch)
		{
			StatusText = TEXT("\u274C Mismatch");
			StatusColor = SpawnParamsToolbox_Constants::Color_Danger;
		}
		else if (_EntryInfo->HasDuplicate)
		{
			StatusText = TEXT("\u26A0 Duplicate");
			StatusColor = SpawnParamsToolbox_Constants::Color_Warning;
		}
		else if (NOT _EntryInfo->HasStruct)
		{
			StatusText = TEXT("\U0001F4ED No Struct");
			StatusColor = FLinearColor::Yellow;
		}
		else
		{
			StatusText = TEXT("\u2705 OK");
			StatusColor = SpawnParamsToolbox_Constants::Color_Success;
		}

		return SNew(SBox)
			.Padding(FMargin{4.0f, 2.0f})
			[
				SNew(STextBlock)
				.Text(FText::FromString(StatusText))
				.ColorAndOpacity(StatusColor)
				.ToolTipText(FText::FromString(_EntryInfo->DiagnosticMessage))
			];
	}

	// ---- EntityScript Column ----
	if (ColumnName == TEXT("EntityScript"))
	{
		const auto TypeIcon = _EntryInfo->IsBlueprint ? TEXT("\U0001F4E6 ") : TEXT("\u2699 ");
		const auto TypeColor = _EntryInfo->IsBlueprint
			? SpawnParamsToolbox_Constants::Color_Blueprint
			: SpawnParamsToolbox_Constants::Color_Cpp;
		const auto TypeTooltip = _EntryInfo->IsBlueprint ? TEXT("Blueprint EntityScript") : TEXT("C++ EntityScript");

		return SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4, 2)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TypeIcon))
					.ColorAndOpacity(TypeColor)
					.ToolTipText(FText::FromString(TypeTooltip))
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(_EntryInfo->EntityScriptName))
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4, 0)
			[
				SAssignNew(_ExpandableArea, SExpandableArea)
				.InitiallyCollapsed(NOT _EntryInfo->IsInspected)
				.HeaderContent()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Properties")))
					.ColorAndOpacity(FSlateColor{FLinearColor::Gray})
				]
				.BodyContent()
				[
					SAssignNew(_BodyBox, SBox)
					[
						DoCreatePropertyComparisonWidget()
					]
				]
			];
	}

	// ---- StructPath Column ----
	if (ColumnName == TEXT("StructPath"))
	{
		const auto DisplayPath = _EntryInfo->HasStruct ? _EntryInfo->StructPath : TEXT("(none)");

		return SNew(SBox)
			.Padding(FMargin{4.0f, 2.0f})
			[
				SNew(STextBlock)
				.Text(FText::FromString(DisplayPath))
				.ColorAndOpacity(_EntryInfo->HasStruct ? FSlateColor{FLinearColor::White} : FSlateColor{FLinearColor::Gray})
			];
	}

	// ---- Actions Column ----
	if (ColumnName == TEXT("Actions"))
	{
		const auto HasScriptAsset = _EntryInfo->ScriptAssetData.IsValid();
		const auto HasStructAsset = _EntryInfo->StructAssetData.IsValid();

		const auto CreateRecreateLabel = _EntryInfo->HasStruct ? TEXT("\U0001F504 Recreate") : TEXT("\u2795 Create");
		const auto CreateRecreateTooltip = _EntryInfo->HasStruct
			? TEXT("Force-recreate the SpawnParams struct from the EntityScript")
			: TEXT("Create a new SpawnParams struct for this EntityScript");

		return SNew(SVerticalBox)

			// Row 1: Create/Recreate + Inspect
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 1)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(1, 0)
				[
					SNew(SButton)
					.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
					.Text(FText::FromString(CreateRecreateLabel))
					.ToolTipText(FText::FromString(CreateRecreateTooltip))
					.OnClicked_Lambda([this]()
					{
						if (const auto Toolbox = _OwnerToolbox.Pin();
							Toolbox.IsValid())
						{
							Toolbox->DoOnCreateOrRecreateEntry(_EntryInfo);
						}
						return FReply::Handled();
					})
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(1, 0)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("\U0001F50E Inspect")))
					.ToolTipText(FText::FromString(TEXT("Show Script vs Struct property comparison")))
					.OnClicked_Lambda([this]()
					{
						if (_Discovery.IsValid())
						{
							_Discovery->Request_InspectEntry(_EntryInfo);

							if (_BodyBox.IsValid())
							{
								_BodyBox->SetContent(DoCreatePropertyComparisonWidget());
							}

							if (_ExpandableArea.IsValid())
							{
								_ExpandableArea->SetExpanded(true);
							}
						}
						return FReply::Handled();
					})
				]
			]

			// Row 2: Script actions
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 1)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(1, 0)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Script:")))
					.ColorAndOpacity(SpawnParamsToolbox_Constants::Color_Muted)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4, 0, 1, 0)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Edit")))
					.ToolTipText(FText::FromString(TEXT("Open the EntityScript Blueprint in the editor")))
					.IsEnabled(HasScriptAsset)
					.OnClicked_Lambda([this]()
					{
						DoEditAsset(_EntryInfo->ScriptAssetData);
						return FReply::Handled();
					})
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(1, 0)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Locate")))
					.ToolTipText(FText::FromString(TEXT("Navigate to the EntityScript in the Content Browser")))
					.IsEnabled(HasScriptAsset)
					.OnClicked_Lambda([this]()
					{
						DoLocateAsset(_EntryInfo->ScriptAssetData);
						return FReply::Handled();
					})
				]
			]

			// Row 3: Struct actions
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 1)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(1, 0)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Struct:")))
					.ColorAndOpacity(SpawnParamsToolbox_Constants::Color_Muted)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4, 0, 1, 0)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Edit")))
					.ToolTipText(FText::FromString(TEXT("Open the SpawnParams struct in the editor")))
					.IsEnabled(HasStructAsset)
					.OnClicked_Lambda([this]()
					{
						DoEditAsset(_EntryInfo->StructAssetData);
						return FReply::Handled();
					})
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(1, 0)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Locate")))
					.ToolTipText(FText::FromString(TEXT("Navigate to the SpawnParams struct in the Content Browser")))
					.IsEnabled(HasStructAsset)
					.OnClicked_Lambda([this]()
					{
						DoLocateAsset(_EntryInfo->StructAssetData);
						return FReply::Handled();
					})
				]
			];
	}

	return SNew(STextBlock).Text(FText::FromString(TEXT("Unknown Column")));
}

auto SCkEntityScriptSpawnParamsListRow::DoLocateAsset(const FAssetData& InAssetData) -> void
{
	if (NOT InAssetData.IsValid())
	{ return; }

	const auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToAssets(TArray<FAssetData>{InAssetData});
}

auto SCkEntityScriptSpawnParamsListRow::DoEditAsset(const FAssetData& InAssetData) -> void
{
	if (NOT InAssetData.IsValid())
	{ return; }

	auto* Asset = InAssetData.GetAsset();

	if (ck::Is_NOT_Valid(Asset))
	{ return; }

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(TArray<UObject*>{Asset});
}

auto SCkEntityScriptSpawnParamsListRow::DoCreatePropertyComparisonWidget() -> TSharedRef<SWidget>
{
	if (NOT _EntryInfo.IsValid() || NOT _EntryInfo->IsInspected)
	{
		return SNew(STextBlock)
			.Text(FText::FromString(TEXT("Click 'Inspect' to load properties")))
			.ColorAndOpacity(FSlateColor{FLinearColor::Gray});
	}

	const auto MaxRows = FMath::Max(_EntryInfo->ScriptProperties.Num(), _EntryInfo->StructProperties.Num());

	if (MaxRows == 0)
	{
		return SNew(STextBlock)
			.Text(FText::FromString(TEXT("No exposed properties")))
			.ColorAndOpacity(FSlateColor{FLinearColor::Gray});
	}

	auto ComparisonBox = SNew(SVerticalBox);

	ComparisonBox->AddSlot()
		.AutoHeight()
		.Padding(0, 2)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(0.05f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("#")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(0.45f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Script Property")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(0.05f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("#")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(0.45f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Struct Property")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
			]
		];

	ComparisonBox->AddSlot()
		.AutoHeight()
		[
			SNew(SSeparator)
		];

	for (auto RowIndex = 0; RowIndex < MaxRows; ++RowIndex)
	{
		const auto HasScript = RowIndex < _EntryInfo->ScriptProperties.Num();
		const auto HasStruct = RowIndex < _EntryInfo->StructProperties.Num();

		auto ScriptText = FString{TEXT("-")};
		auto StructText = FString{TEXT("-")};
		auto ScriptIndex = FString{TEXT("-")};
		auto StructIndex = FString{TEXT("-")};
		auto RowColor = FLinearColor::White;

		if (HasScript)
		{
			const auto& ScriptProp = _EntryInfo->ScriptProperties[RowIndex];
			ScriptText = FString::Printf(TEXT("%s (%s)"), *ScriptProp.Name, *ScriptProp.TypeName);
			ScriptIndex = FString::Printf(TEXT("%d"), ScriptProp.Index);
		}

		if (HasStruct)
		{
			const auto& StructProp = _EntryInfo->StructProperties[RowIndex];
			StructText = FString::Printf(TEXT("%s (%s)"), *StructProp.Name, *StructProp.TypeName);
			StructIndex = FString::Printf(TEXT("%d"), StructProp.Index);
		}

		if (HasScript && HasStruct)
		{
			const auto& ScriptProp = _EntryInfo->ScriptProperties[RowIndex];
			const auto& StructProp = _EntryInfo->StructProperties[RowIndex];

			if (ScriptProp.Name != StructProp.Name || ScriptProp.TypeName != StructProp.TypeName)
			{
				RowColor = FLinearColor{1.0f, 0.3f, 0.3f};
			}
		}
		else
		{
			RowColor = FLinearColor::Yellow;
		}

		ComparisonBox->AddSlot()
			.AutoHeight()
			.Padding(0, 1)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.FillWidth(0.05f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(ScriptIndex))
					.ColorAndOpacity(RowColor)
				]

				+ SHorizontalBox::Slot()
				.FillWidth(0.45f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(ScriptText))
					.ColorAndOpacity(RowColor)
				]

				+ SHorizontalBox::Slot()
				.FillWidth(0.05f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(StructIndex))
					.ColorAndOpacity(RowColor)
				]

				+ SHorizontalBox::Slot()
				.FillWidth(0.45f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(StructText))
					.ColorAndOpacity(RowColor)
				]
			];
	}

	return ComparisonBox;
}

// --------------------------------------------------------------------------------------------------------------------
// Main Widget
// --------------------------------------------------------------------------------------------------------------------

auto SCkEntityScriptSpawnParamsToolbox::Construct(const FArguments& InArgs) -> void
{
	_Discovery = MakeShared<FCkEntityScriptSpawnParamsDiscovery>();

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(SpawnParamsToolbox_Constants::PanelPadding)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, SpawnParamsToolbox_Constants::SectionSpacing)
			[
				DoCreateButtonBar()
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, SpawnParamsToolbox_Constants::SectionSpacing)
			[
				DoCreateStatusBar()
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				DoCreateListPanel()
			]
		]
	];
}

auto SCkEntityScriptSpawnParamsToolbox::DoCreateButtonBar() -> TSharedRef<SWidget>
{
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0, 0, 4, 0)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
			.Text(FText::FromString(TEXT("\U0001F50D Discover All")))
			.ToolTipText(FText::FromString(TEXT("Find all EntityScripts and their SpawnParams structs")))
			.OnClicked(this, &SCkEntityScriptSpawnParamsToolbox::DoOnDiscoverClicked)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4, 0)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("\u26A0 Check Mismatches")))
			.ToolTipText(FText::FromString(TEXT("Compare EntityScript properties against their SpawnParams struct")))
			.OnClicked(this, &SCkEntityScriptSpawnParamsToolbox::DoOnCheckMismatchesClicked)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4, 0)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("\U0001F4CB Check Duplicates")))
			.ToolTipText(FText::FromString(TEXT("Find SpawnParams structs that exist in multiple locations")))
			.OnClicked(this, &SCkEntityScriptSpawnParamsToolbox::DoOnCheckDuplicatesClicked)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4, 0)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("\U0001F50E Inspect All")))
			.ToolTipText(FText::FromString(TEXT("Load and display property comparison for all entries")))
			.OnClicked(this, &SCkEntityScriptSpawnParamsToolbox::DoOnInspectAllClicked)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4, 0)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
			.Text(FText::FromString(TEXT("\U0001F504 Recreate All")))
			.ForegroundColor(SpawnParamsToolbox_Constants::Color_Danger)
			.ToolTipText(FText::FromString(TEXT("Force-recreate all SpawnParams structs from their EntityScripts")))
			.OnClicked(this, &SCkEntityScriptSpawnParamsToolbox::DoOnRecreateAllClicked)
		];
}

auto SCkEntityScriptSpawnParamsToolbox::DoCreateStatusBar() -> TSharedRef<SWidget>
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
		.Padding(FMargin{8.0f, 4.0f})
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, SpawnParamsToolbox_Constants::StatusChipSpacing, 0)
			.VAlign(VAlign_Center)
			[
				SAssignNew(_StatusText_Total, STextBlock)
				.Text(FText::FromString(TEXT("\U0001F4CA Total: 0")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(FLinearColor::White)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, SpawnParamsToolbox_Constants::StatusChipSpacing, 0)
			.VAlign(VAlign_Center)
			[
				SAssignNew(_StatusText_Mismatches, STextBlock)
				.Text(FText::FromString(TEXT("\u274C Mismatches: 0")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.ColorAndOpacity(SpawnParamsToolbox_Constants::Color_Muted)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, SpawnParamsToolbox_Constants::StatusChipSpacing, 0)
			.VAlign(VAlign_Center)
			[
				SAssignNew(_StatusText_Duplicates, STextBlock)
				.Text(FText::FromString(TEXT("\u26A0 Duplicates: 0")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.ColorAndOpacity(SpawnParamsToolbox_Constants::Color_Muted)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SAssignNew(_StatusText_MissingStructs, STextBlock)
				.Text(FText::FromString(TEXT("\U0001F4ED Missing: 0")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.ColorAndOpacity(SpawnParamsToolbox_Constants::Color_Muted)
			]
		];
}

auto SCkEntityScriptSpawnParamsToolbox::DoCreateListPanel() -> TSharedRef<SWidget>
{
	return SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 4)
		[
			SAssignNew(_SearchBox, SSearchBox)
			.HintText(FText::FromString(TEXT("Search EntityScripts...")))
			.OnTextChanged(this, &SCkEntityScriptSpawnParamsToolbox::DoOnSearchTextChanged)
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(_ListView, SListView<TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>>)
			.ListItemsSource(&_Discovery->Get_FilteredEntries())
			.OnGenerateRow_Lambda([this](TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo> InItem, const TSharedRef<STableViewBase>& OwnerTable) -> TSharedRef<ITableRow>
			{
				return SNew(SCkEntityScriptSpawnParamsListRow, OwnerTable)
					.EntryInfo(InItem)
					.Discovery(_Discovery)
					.OwnerToolbox(SharedThis(this));
			})
			.HeaderRow
			(
				SNew(SHeaderRow)

				+ SHeaderRow::Column(TEXT("Status"))
				.DefaultLabel(FText::FromString(TEXT("Status")))
				.FixedWidth(120.0f)

				+ SHeaderRow::Column(TEXT("EntityScript"))
				.DefaultLabel(FText::FromString(TEXT("EntityScript")))
				.FillWidth(1.5f)

				+ SHeaderRow::Column(TEXT("StructPath"))
				.DefaultLabel(FText::FromString(TEXT("Struct Path")))
				.FillWidth(1.0f)

				+ SHeaderRow::Column(TEXT("Actions"))
				.DefaultLabel(FText::FromString(TEXT("Actions")))
				.FixedWidth(280.0f)
			)
		];
}

auto SCkEntityScriptSpawnParamsToolbox::DoOnDiscoverClicked() -> FReply
{
	_Discovery->Request_DiscoverAll();
	DoRefreshList();
	return FReply::Handled();
}

auto SCkEntityScriptSpawnParamsToolbox::DoOnCheckMismatchesClicked() -> FReply
{
	if (_Discovery->Get_AllEntries().IsEmpty())
	{
		_Discovery->Request_DiscoverAll();
	}

	_Discovery->Request_CheckMismatches();
	DoRefreshList();
	return FReply::Handled();
}

auto SCkEntityScriptSpawnParamsToolbox::DoOnCheckDuplicatesClicked() -> FReply
{
	if (_Discovery->Get_AllEntries().IsEmpty())
	{
		_Discovery->Request_DiscoverAll();
	}

	_Discovery->Request_CheckDuplicates();
	DoRefreshList();
	return FReply::Handled();
}

auto SCkEntityScriptSpawnParamsToolbox::DoOnRecreateAllClicked() -> FReply
{
	if (_Discovery->Get_AllEntries().IsEmpty())
	{
		_Discovery->Request_DiscoverAll();
	}

	_Discovery->Request_RecreateAll();
	DoRefreshList();
	return FReply::Handled();
}

auto SCkEntityScriptSpawnParamsToolbox::DoOnInspectAllClicked() -> FReply
{
	if (_Discovery->Get_AllEntries().IsEmpty())
	{
		_Discovery->Request_DiscoverAll();
	}

	_Discovery->Request_InspectAll();
	DoRefreshList();
	return FReply::Handled();
}

auto SCkEntityScriptSpawnParamsToolbox::DoOnSearchTextChanged(const FText& InText) -> void
{
	_Discovery->Request_ApplySearchFilter(InText.ToString());
	DoRefreshList();
}

auto SCkEntityScriptSpawnParamsToolbox::DoOnCreateOrRecreateEntry(
	const TSharedPtr<FCk_SpawnParamsToolbox_EntryInfo>& InEntry)
	-> void
{
	if (NOT InEntry.IsValid())
	{ return; }

	_Discovery->Request_CreateOrRecreateEntry(InEntry);
	DoRefreshList();
}

auto SCkEntityScriptSpawnParamsToolbox::DoRefreshList() -> void
{
	if (_ListView.IsValid())
	{
		_ListView->RequestListRefresh();
	}

	DoUpdateStatusBar();
}

auto SCkEntityScriptSpawnParamsToolbox::DoUpdateStatusBar() -> void
{
	const auto [TotalCount, MismatchCount, DuplicateCount, MissingCount] = _Discovery->Get_Stats();

	if (_StatusText_Total.IsValid())
	{
		_StatusText_Total->SetText(FText::FromString(
			FString::Printf(TEXT("\U0001F4CA Total: %d"), TotalCount)));
	}

	if (_StatusText_Mismatches.IsValid())
	{
		_StatusText_Mismatches->SetText(FText::FromString(
			FString::Printf(TEXT("\u274C Mismatches: %d"), MismatchCount)));

		const auto MismatchColor = MismatchCount > 0
			? SpawnParamsToolbox_Constants::Color_Danger
			: SpawnParamsToolbox_Constants::Color_Muted;
		_StatusText_Mismatches->SetColorAndOpacity(MismatchColor);
		_StatusText_Mismatches->SetFont(FCoreStyle::GetDefaultFontStyle(
			MismatchCount > 0 ? "Bold" : "Regular", 9));
	}

	if (_StatusText_Duplicates.IsValid())
	{
		_StatusText_Duplicates->SetText(FText::FromString(
			FString::Printf(TEXT("\u26A0 Duplicates: %d"), DuplicateCount)));

		const auto DuplicateColor = DuplicateCount > 0
			? SpawnParamsToolbox_Constants::Color_Warning
			: SpawnParamsToolbox_Constants::Color_Muted;
		_StatusText_Duplicates->SetColorAndOpacity(DuplicateColor);
		_StatusText_Duplicates->SetFont(FCoreStyle::GetDefaultFontStyle(
			DuplicateCount > 0 ? "Bold" : "Regular", 9));
	}

	if (_StatusText_MissingStructs.IsValid())
	{
		_StatusText_MissingStructs->SetText(FText::FromString(
			FString::Printf(TEXT("\U0001F4ED Missing: %d"), MissingCount)));

		const auto MissingColor = MissingCount > 0
			? FLinearColor::Yellow
			: SpawnParamsToolbox_Constants::Color_Muted;
		_StatusText_MissingStructs->SetColorAndOpacity(MissingColor);
		_StatusText_MissingStructs->SetFont(FCoreStyle::GetDefaultFontStyle(
			MissingCount > 0 ? "Bold" : "Regular", 9));
	}
}

// --------------------------------------------------------------------------------------------------------------------
