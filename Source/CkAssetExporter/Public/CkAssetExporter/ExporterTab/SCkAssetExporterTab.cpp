#include "SCkAssetExporterTab.h"

#include "CkAssetExporter_Log.h"

#include <BehaviorTree/BehaviorTree.h>
#include <Engine/Blueprint.h>
#include <Engine/DataAsset.h>
#include <EnvironmentQuery/EnvQuery.h>
#include <Materials/MaterialInterface.h>
#include <StateTree.h>
#include <Engine/UserDefinedEnum.h>
#include <StructUtils/UserDefinedStruct.h>
#include <ContentBrowserModule.h>
#include <IContentBrowserSingleton.h>
#include <Styling/AppStyle.h>
#include <Widgets/Layout/SBorder.h>
#include <Widgets/Layout/SBox.h>
#include <Widgets/Layout/SSeparator.h>
#include <Widgets/Layout/SScrollBox.h>

// --------------------------------------------------------------------------------------------------------------------

namespace AssetExporterTab_Constants
{
    constexpr float PanelPadding = 8.0f;
    constexpr float ButtonWidth = 180.0f;
    constexpr float SectionSpacing = 8.0f;

    const FLinearColor Color_Success{0.2f, 0.8f, 0.2f};
    const FLinearColor Color_Error{0.9f, 0.2f, 0.2f};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkAssetExporterTab::
    Construct(const FArguments& InArgs)
    -> void
{
    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(AssetExporterTab_Constants::PanelPadding)
        [
            SNew(SVerticalBox)

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, AssetExporterTab_Constants::SectionSpacing)
            [
                DoCreateButtonBar()
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, AssetExporterTab_Constants::SectionSpacing)
            [
                SNew(SSeparator)
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, AssetExporterTab_Constants::SectionSpacing)
            [
                SAssignNew(_StatusText, STextBlock)
                .Text(FText::FromString(TEXT("Select assets in the Content Browser, then click Export.")))
            ]

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
    SCkAssetExporterTab::
    DoCreateButtonBar()
    -> TSharedRef<SWidget>
{
    return SNew(SHorizontalBox)

        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0, 0, AssetExporterTab_Constants::SectionSpacing, 0)
        [
            SNew(SBox)
            .WidthOverride(AssetExporterTab_Constants::ButtonWidth)
            [
                SNew(SButton)
                .Text(FText::FromString(TEXT("Export Selected BTs")))
                .ToolTipText(FText::FromString(TEXT("Export Behavior Trees currently selected in the Content Browser to JSON and Text files")))
                .OnClicked(this, &SCkAssetExporterTab::DoOnExportSelectedClicked)
                .HAlign(HAlign_Center)
            ]
        ]

        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0, 0, AssetExporterTab_Constants::SectionSpacing, 0)
        [
            SNew(SBox)
            .WidthOverride(200.0f)
            [
                SNew(SButton)
                .Text(FText::FromString(TEXT("Export Selected Blueprints")))
                .ToolTipText(FText::FromString(TEXT("Export Blueprints currently selected in the Content Browser to JSON and Text files")))
                .OnClicked(this, &SCkAssetExporterTab::DoOnExportSelectedBlueprintsClicked)
                .HAlign(HAlign_Center)
            ]
        ]

        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0, 0, AssetExporterTab_Constants::SectionSpacing, 0)
        [
            SNew(SBox)
            .WidthOverride(200.0f)
            [
                SNew(SButton)
                .Text(FText::FromString(TEXT("Export Selected DataAssets")))
                .ToolTipText(FText::FromString(TEXT("Export DataAssets currently selected in the Content Browser to JSON and Text files")))
                .OnClicked(this, &SCkAssetExporterTab::DoOnExportSelectedDataAssetsClicked)
                .HAlign(HAlign_Center)
            ]
        ]

        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0, 0, AssetExporterTab_Constants::SectionSpacing, 0)
        [
            SNew(SBox)
            .WidthOverride(200.0f)
            [
                SNew(SButton)
                .Text(FText::FromString(TEXT("Export Selected EQS")))
                .ToolTipText(FText::FromString(TEXT("Export EQS Queries currently selected in the Content Browser to JSON and Text files")))
                .OnClicked(this, &SCkAssetExporterTab::DoOnExportSelectedEQSClicked)
                .HAlign(HAlign_Center)
            ]
        ]

        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0, 0, AssetExporterTab_Constants::SectionSpacing, 0)
        [
            SNew(SBox)
            .WidthOverride(200.0f)
            [
                SNew(SButton)
                .Text(FText::FromString(TEXT("Export Selected Materials")))
                .ToolTipText(FText::FromString(TEXT("Export Materials / Material Instances currently selected in the Content Browser to JSON files")))
                .OnClicked(this, &SCkAssetExporterTab::DoOnExportSelectedMaterialsClicked)
                .HAlign(HAlign_Center)
            ]
        ]

        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0, 0, AssetExporterTab_Constants::SectionSpacing, 0)
        [
            SNew(SBox)
            .WidthOverride(200.0f)
            [
                SNew(SButton)
                .Text(FText::FromString(TEXT("Export Selected StateTrees")))
                .ToolTipText(FText::FromString(TEXT("Export State Trees currently selected in the Content Browser to JSON and Text files")))
                .OnClicked(this, &SCkAssetExporterTab::DoOnExportSelectedStateTreesClicked)
                .HAlign(HAlign_Center)
            ]
        ]

        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0, 0, AssetExporterTab_Constants::SectionSpacing, 0)
        [
            SNew(SBox)
            .WidthOverride(200.0f)
            [
                SNew(SButton)
                .Text(FText::FromString(TEXT("Export Selected Structs")))
                .ToolTipText(FText::FromString(TEXT("Export Blueprint structs currently selected in the Content Browser to JSON and Text files")))
                .OnClicked(this, &SCkAssetExporterTab::DoOnExportSelectedUserDefinedStructsClicked)
                .HAlign(HAlign_Center)
            ]
        ]

        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0, 0, AssetExporterTab_Constants::SectionSpacing, 0)
        [
            SNew(SBox)
            .WidthOverride(200.0f)
            [
                SNew(SButton)
                .Text(FText::FromString(TEXT("Export Selected Enums")))
                .ToolTipText(FText::FromString(TEXT("Export Blueprint enums currently selected in the Content Browser to JSON and Text files")))
                .OnClicked(this, &SCkAssetExporterTab::DoOnExportSelectedUserDefinedEnumsClicked)
                .HAlign(HAlign_Center)
            ]
        ];
}

auto
    SCkAssetExporterTab::
    DoCreateResultsPanel()
    -> TSharedRef<SWidget>
{
    return SAssignNew(_ResultsListView,
        SListView<TSharedPtr<FCk_AssetExporterTab_ResultEntry>>)
        .ListItemsSource(&_ResultEntries)
        .OnGenerateRow(this, &SCkAssetExporterTab::DoGenerateResultRow)
        .HeaderRow(
            SNew(SHeaderRow)
            + SHeaderRow::Column("Status")
            .DefaultLabel(FText::FromString(TEXT("Status")))
            .FixedWidth(60.0f)
            + SHeaderRow::Column("Asset")
            .DefaultLabel(FText::FromString(TEXT("Asset")))
            .FillWidth(0.3f)
            + SHeaderRow::Column("Output")
            .DefaultLabel(FText::FromString(TEXT("Output Path")))
            .FillWidth(0.7f)
        );
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkAssetExporterTab::
    DoOnExportSelectedClicked()
    -> FReply
{
    auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    auto SelectedAssets = TArray<FAssetData>{};
    ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

    auto BehaviorTrees = TArray<UBehaviorTree*>{};
    for (const auto& AssetData : SelectedAssets)
    {
        if (auto* BT = Cast<UBehaviorTree>(AssetData.GetAsset()))
        {
            BehaviorTrees.Add(BT);
        }
    }

    if (BehaviorTrees.Num() == 0)
    {
        _StatusText->SetText(FText::FromString(TEXT("No Behavior Tree assets selected in the Content Browser.")));
        return FReply::Handled();
    }

    const auto Results = FCk_BehaviorTreeExporter::ExportBehaviorTrees(BehaviorTrees);
    DoRefreshResultsList(Results);

    auto SuccessCount = int32{0};
    auto FailCount = int32{0};
    for (const auto& R : Results)
    {
        if (R.Succeeded) { ++SuccessCount; }
        else { ++FailCount; }
    }

    if (FailCount == 0)
    {
        _StatusText->SetText(FText::FromString(
            FString::Printf(TEXT("Successfully exported %d Behavior Tree(s)."), SuccessCount)));
    }
    else
    {
        _StatusText->SetText(FText::FromString(
            FString::Printf(TEXT("Exported %d, failed %d Behavior Tree(s)."), SuccessCount, FailCount)));
    }

    return FReply::Handled();
}

auto
    SCkAssetExporterTab::
    DoRefreshResultsList(
        const TArray<FCk_BehaviorTreeExportResult>& InResults)
    -> void
{
    _ResultEntries.Empty();

    for (const auto& Result : InResults)
    {
        auto Entry = MakeShared<FCk_AssetExporterTab_ResultEntry>();
        Entry->AssetName = Result.AssetName;
        Entry->Succeeded = Result.Succeeded;
        Entry->JsonPath = Result.JsonFilePath;
        Entry->TextPath = Result.TextFilePath;
        Entry->ErrorMessage = Result.ErrorMessage;
        _ResultEntries.Add(Entry);
    }

    if (_ResultsListView.IsValid())
    {
        _ResultsListView->RequestListRefresh();
    }
}

auto
    SCkAssetExporterTab::
    DoOnExportSelectedBlueprintsClicked()
    -> FReply
{
    auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    auto SelectedAssets = TArray<FAssetData>{};
    ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

    auto Blueprints = TArray<UBlueprint*>{};
    for (const auto& AssetData : SelectedAssets)
    {
        if (auto* BP = Cast<UBlueprint>(AssetData.GetAsset()))
        {
            Blueprints.Add(BP);
        }
    }

    if (Blueprints.Num() == 0)
    {
        _StatusText->SetText(FText::FromString(TEXT("No Blueprint assets selected in the Content Browser.")));
        return FReply::Handled();
    }

    const auto Results = FCk_BlueprintExporter::ExportBlueprints(Blueprints);
    DoRefreshResultsListFromBlueprintResults(Results);

    auto SuccessCount = int32{0};
    auto FailCount = int32{0};
    for (const auto& R : Results)
    {
        if (R.Succeeded) { ++SuccessCount; }
        else { ++FailCount; }
    }

    if (FailCount == 0)
    {
        _StatusText->SetText(FText::FromString(
            FString::Printf(TEXT("Successfully exported %d Blueprint(s)."), SuccessCount)));
    }
    else
    {
        _StatusText->SetText(FText::FromString(
            FString::Printf(TEXT("Exported %d, failed %d Blueprint(s)."), SuccessCount, FailCount)));
    }

    return FReply::Handled();
}

auto
    SCkAssetExporterTab::
    DoRefreshResultsListFromBlueprintResults(
        const TArray<FCk_BlueprintExportResult>& InResults)
    -> void
{
    _ResultEntries.Empty();

    for (const auto& Result : InResults)
    {
        auto Entry = MakeShared<FCk_AssetExporterTab_ResultEntry>();
        Entry->AssetName = Result.AssetName;
        Entry->Succeeded = Result.Succeeded;
        Entry->JsonPath = Result.JsonFilePath;
        Entry->TextPath = Result.TextFilePath;
        Entry->ErrorMessage = Result.ErrorMessage;
        _ResultEntries.Add(Entry);
    }

    if (_ResultsListView.IsValid())
    {
        _ResultsListView->RequestListRefresh();
    }
}

auto
    SCkAssetExporterTab::
    DoOnExportSelectedDataAssetsClicked()
    -> FReply
{
    auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    auto SelectedAssets = TArray<FAssetData>{};
    ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

    auto DataAssets = TArray<UDataAsset*>{};
    for (const auto& AssetData : SelectedAssets)
    {
        if (auto* DA = Cast<UDataAsset>(AssetData.GetAsset()))
        {
            DataAssets.Add(DA);
        }
    }

    if (DataAssets.Num() == 0)
    {
        _StatusText->SetText(FText::FromString(TEXT("No DataAsset assets selected in the Content Browser.")));
        return FReply::Handled();
    }

    const auto Results = FCk_DataAssetExporter::ExportDataAssets(DataAssets);
    DoRefreshResultsListFromDataAssetResults(Results);

    auto SuccessCount = int32{0};
    auto FailCount = int32{0};
    for (const auto& R : Results)
    {
        if (R.Succeeded) { ++SuccessCount; }
        else { ++FailCount; }
    }

    if (FailCount == 0)
    {
        _StatusText->SetText(FText::FromString(
            FString::Printf(TEXT("Successfully exported %d DataAsset(s)."), SuccessCount)));
    }
    else
    {
        _StatusText->SetText(FText::FromString(
            FString::Printf(TEXT("Exported %d, failed %d DataAsset(s)."), SuccessCount, FailCount)));
    }

    return FReply::Handled();
}

auto
    SCkAssetExporterTab::
    DoRefreshResultsListFromDataAssetResults(
        const TArray<FCk_DataAssetExportResult>& InResults)
    -> void
{
    _ResultEntries.Empty();

    for (const auto& Result : InResults)
    {
        auto Entry = MakeShared<FCk_AssetExporterTab_ResultEntry>();
        Entry->AssetName = Result.AssetName;
        Entry->Succeeded = Result.Succeeded;
        Entry->JsonPath = Result.JsonFilePath;
        Entry->TextPath = Result.TextFilePath;
        Entry->ErrorMessage = Result.ErrorMessage;
        _ResultEntries.Add(Entry);
    }

    if (_ResultsListView.IsValid())
    {
        _ResultsListView->RequestListRefresh();
    }
}

auto
    SCkAssetExporterTab::
    DoOnExportSelectedEQSClicked()
    -> FReply
{
    auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    auto SelectedAssets = TArray<FAssetData>{};
    ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

    auto Queries = TArray<UEnvQuery*>{};
    for (const auto& AssetData : SelectedAssets)
    {
        if (auto* Query = Cast<UEnvQuery>(AssetData.GetAsset()))
        {
            Queries.Add(Query);
        }
    }

    if (Queries.Num() == 0)
    {
        _StatusText->SetText(FText::FromString(TEXT("No EQS Query assets selected in the Content Browser.")));
        return FReply::Handled();
    }

    const auto Results = FCk_EQSExporter::ExportEQSQueries(Queries);
    DoRefreshResultsListFromEQSResults(Results);

    auto SuccessCount = int32{0};
    auto FailCount = int32{0};
    for (const auto& R : Results)
    {
        if (R.Succeeded) { ++SuccessCount; }
        else { ++FailCount; }
    }

    if (FailCount == 0)
    {
        _StatusText->SetText(FText::FromString(
            FString::Printf(TEXT("Successfully exported %d EQS Query(s)."), SuccessCount)));
    }
    else
    {
        _StatusText->SetText(FText::FromString(
            FString::Printf(TEXT("Exported %d, failed %d EQS Query(s)."), SuccessCount, FailCount)));
    }

    return FReply::Handled();
}

auto
    SCkAssetExporterTab::
    DoRefreshResultsListFromEQSResults(
        const TArray<FCk_EQSExportResult>& InResults)
    -> void
{
    _ResultEntries.Empty();

    for (const auto& Result : InResults)
    {
        auto Entry = MakeShared<FCk_AssetExporterTab_ResultEntry>();
        Entry->AssetName = Result.AssetName;
        Entry->Succeeded = Result.Succeeded;
        Entry->JsonPath = Result.JsonFilePath;
        Entry->TextPath = Result.TextFilePath;
        Entry->ErrorMessage = Result.ErrorMessage;
        _ResultEntries.Add(Entry);
    }

    if (_ResultsListView.IsValid())
    {
        _ResultsListView->RequestListRefresh();
    }
}

auto
    SCkAssetExporterTab::
    DoOnExportSelectedMaterialsClicked()
    -> FReply
{
    auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    auto SelectedAssets = TArray<FAssetData>{};
    ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

    auto Materials = TArray<UMaterialInterface*>{};
    for (const auto& AssetData : SelectedAssets)
    {
        if (auto* Mat = Cast<UMaterialInterface>(AssetData.GetAsset()))
        {
            Materials.Add(Mat);
        }
    }

    if (Materials.Num() == 0)
    {
        _StatusText->SetText(FText::FromString(TEXT("No Material assets selected in the Content Browser.")));
        return FReply::Handled();
    }

    const auto Results = FCk_MaterialExporter::ExportMaterials(Materials);
    DoRefreshResultsListFromMaterialResults(Results);

    auto SuccessCount = int32{0};
    auto FailCount = int32{0};
    for (const auto& R : Results)
    {
        if (R.Succeeded) { ++SuccessCount; }
        else { ++FailCount; }
    }

    if (FailCount == 0)
    {
        _StatusText->SetText(FText::FromString(
            FString::Printf(TEXT("Successfully exported %d Material(s)."), SuccessCount)));
    }
    else
    {
        _StatusText->SetText(FText::FromString(
            FString::Printf(TEXT("Exported %d, failed %d Material(s)."), SuccessCount, FailCount)));
    }

    return FReply::Handled();
}

auto
    SCkAssetExporterTab::
    DoRefreshResultsListFromMaterialResults(
        const TArray<FCk_MaterialExportResult>& InResults)
    -> void
{
    _ResultEntries.Empty();

    for (const auto& Result : InResults)
    {
        auto Entry = MakeShared<FCk_AssetExporterTab_ResultEntry>();
        Entry->AssetName = Result.AssetName;
        Entry->Succeeded = Result.Succeeded;
        Entry->JsonPath = Result.JsonFilePath;
        Entry->TextPath = FString{};   // material export is json-only
        Entry->ErrorMessage = Result.ErrorMessage;
        _ResultEntries.Add(Entry);
    }

    if (_ResultsListView.IsValid())
    {
        _ResultsListView->RequestListRefresh();
    }
}

auto
    SCkAssetExporterTab::
    DoOnExportSelectedStateTreesClicked()
    -> FReply
{
    auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    auto SelectedAssets = TArray<FAssetData>{};
    ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

    auto StateTrees = TArray<UStateTree*>{};
    for (const auto& AssetData : SelectedAssets)
    {
        if (auto* ST = Cast<UStateTree>(AssetData.GetAsset()))
        {
            StateTrees.Add(ST);
        }
    }

    if (StateTrees.Num() == 0)
    {
        _StatusText->SetText(FText::FromString(TEXT("No State Tree assets selected in the Content Browser.")));
        return FReply::Handled();
    }

    const auto Results = FCk_StateTreeExporter::ExportStateTrees(StateTrees);
    DoRefreshResultsListFromStateTreeResults(Results);

    auto SuccessCount = int32{0};
    auto FailCount = int32{0};
    for (const auto& R : Results)
    {
        if (R.Succeeded) { ++SuccessCount; }
        else { ++FailCount; }
    }

    if (FailCount == 0)
    {
        _StatusText->SetText(FText::FromString(
            FString::Printf(TEXT("Successfully exported %d State Tree(s)."), SuccessCount)));
    }
    else
    {
        _StatusText->SetText(FText::FromString(
            FString::Printf(TEXT("Exported %d, failed %d State Tree(s)."), SuccessCount, FailCount)));
    }

    return FReply::Handled();
}

auto
    SCkAssetExporterTab::
    DoRefreshResultsListFromStateTreeResults(
        const TArray<FCk_StateTreeExportResult>& InResults)
    -> void
{
    _ResultEntries.Empty();

    for (const auto& Result : InResults)
    {
        auto Entry = MakeShared<FCk_AssetExporterTab_ResultEntry>();
        Entry->AssetName = Result.AssetName;
        Entry->Succeeded = Result.Succeeded;
        Entry->JsonPath = Result.JsonFilePath;
        Entry->TextPath = Result.TextFilePath;
        Entry->ErrorMessage = Result.ErrorMessage;
        _ResultEntries.Add(Entry);
    }

    if (_ResultsListView.IsValid())
    {
        _ResultsListView->RequestListRefresh();
    }
}

auto
    SCkAssetExporterTab::
    DoOnExportSelectedUserDefinedStructsClicked()
    -> FReply
{
    auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    auto SelectedAssets = TArray<FAssetData>{};
    ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

    auto Structs = TArray<UUserDefinedStruct*>{};
    for (const auto& AssetData : SelectedAssets)
    {
        if (auto* Struct = Cast<UUserDefinedStruct>(AssetData.GetAsset()))
        {
            Structs.Add(Struct);
        }
    }

    if (Structs.Num() == 0)
    {
        _StatusText->SetText(FText::FromString(TEXT("No Blueprint struct assets selected in the Content Browser.")));
        return FReply::Handled();
    }

    const auto Results = FCk_UserDefinedStructExporter::ExportUserDefinedStructs(Structs);
    DoRefreshResultsListFromUserDefinedStructResults(Results);

    auto SuccessCount = int32{0};
    auto FailCount = int32{0};
    for (const auto& R : Results)
    {
        if (R.Succeeded) { ++SuccessCount; }
        else { ++FailCount; }
    }

    if (FailCount == 0)
    {
        _StatusText->SetText(FText::FromString(
            FString::Printf(TEXT("Successfully exported %d Struct(s)."), SuccessCount)));
    }
    else
    {
        _StatusText->SetText(FText::FromString(
            FString::Printf(TEXT("Exported %d, failed %d Struct(s)."), SuccessCount, FailCount)));
    }

    return FReply::Handled();
}

auto
    SCkAssetExporterTab::
    DoRefreshResultsListFromUserDefinedStructResults(
        const TArray<FCk_UserDefinedStructExportResult>& InResults)
    -> void
{
    _ResultEntries.Empty();

    for (const auto& Result : InResults)
    {
        auto Entry = MakeShared<FCk_AssetExporterTab_ResultEntry>();
        Entry->AssetName = Result.AssetName;
        Entry->Succeeded = Result.Succeeded;
        Entry->JsonPath = Result.JsonFilePath;
        Entry->TextPath = Result.TextFilePath;
        Entry->ErrorMessage = Result.ErrorMessage;
        _ResultEntries.Add(Entry);
    }

    if (_ResultsListView.IsValid())
    {
        _ResultsListView->RequestListRefresh();
    }
}

auto
    SCkAssetExporterTab::
    DoOnExportSelectedUserDefinedEnumsClicked()
    -> FReply
{
    auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    auto SelectedAssets = TArray<FAssetData>{};
    ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

    auto Enums = TArray<UUserDefinedEnum*>{};
    for (const auto& AssetData : SelectedAssets)
    {
        if (auto* Enum = Cast<UUserDefinedEnum>(AssetData.GetAsset()))
        {
            Enums.Add(Enum);
        }
    }

    if (Enums.Num() == 0)
    {
        _StatusText->SetText(FText::FromString(TEXT("No Blueprint enum assets selected in the Content Browser.")));
        return FReply::Handled();
    }

    const auto Results = FCk_UserDefinedEnumExporter::ExportUserDefinedEnums(Enums);
    DoRefreshResultsListFromUserDefinedEnumResults(Results);

    auto SuccessCount = int32{0};
    auto FailCount = int32{0};
    for (const auto& R : Results)
    {
        if (R.Succeeded) { ++SuccessCount; }
        else { ++FailCount; }
    }

    if (FailCount == 0)
    {
        _StatusText->SetText(FText::FromString(
            FString::Printf(TEXT("Successfully exported %d Enum(s)."), SuccessCount)));
    }
    else
    {
        _StatusText->SetText(FText::FromString(
            FString::Printf(TEXT("Exported %d, failed %d Enum(s)."), SuccessCount, FailCount)));
    }

    return FReply::Handled();
}

auto
    SCkAssetExporterTab::
    DoRefreshResultsListFromUserDefinedEnumResults(
        const TArray<FCk_UserDefinedEnumExportResult>& InResults)
    -> void
{
    _ResultEntries.Empty();

    for (const auto& Result : InResults)
    {
        auto Entry = MakeShared<FCk_AssetExporterTab_ResultEntry>();
        Entry->AssetName = Result.AssetName;
        Entry->Succeeded = Result.Succeeded;
        Entry->JsonPath = Result.JsonFilePath;
        Entry->TextPath = Result.TextFilePath;
        Entry->ErrorMessage = Result.ErrorMessage;
        _ResultEntries.Add(Entry);
    }

    if (_ResultsListView.IsValid())
    {
        _ResultsListView->RequestListRefresh();
    }
}

auto
    SCkAssetExporterTab::
    DoGenerateResultRow(
        TSharedPtr<FCk_AssetExporterTab_ResultEntry> InEntry,
        const TSharedRef<STableViewBase>& InOwnerTable)
    -> TSharedRef<ITableRow>
{
    const auto StatusColor = InEntry->Succeeded
        ? AssetExporterTab_Constants::Color_Success
        : AssetExporterTab_Constants::Color_Error;

    const auto StatusLabel = InEntry->Succeeded ? TEXT("OK") : TEXT("FAIL");

    const auto OutputText = InEntry->Succeeded
        ? FString::Printf(TEXT("%s\n%s"), *InEntry->JsonPath, *InEntry->TextPath)
        : InEntry->ErrorMessage;

    return SNew(STableRow<TSharedPtr<FCk_AssetExporterTab_ResultEntry>>, InOwnerTable)
        [
            SNew(SHorizontalBox)

            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(4.0f, 2.0f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(StatusLabel))
                .ColorAndOpacity(FSlateColor(StatusColor))
            ]

            + SHorizontalBox::Slot()
            .FillWidth(0.3f)
            .VAlign(VAlign_Center)
            .Padding(4.0f, 2.0f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(InEntry->AssetName))
            ]

            + SHorizontalBox::Slot()
            .FillWidth(0.7f)
            .VAlign(VAlign_Center)
            .Padding(4.0f, 2.0f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(OutputText))
                .AutoWrapText(true)
            ]
        ];
}

// --------------------------------------------------------------------------------------------------------------------
