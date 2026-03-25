#include "SCkBehaviorTreeExporterTab.h"

#include "CkAssetExporter_Log.h"

#include <BehaviorTree/BehaviorTree.h>
#include <Engine/Blueprint.h>
#include <Engine/DataAsset.h>
#include <EnvironmentQuery/EnvQuery.h>
#include <ContentBrowserModule.h>
#include <IContentBrowserSingleton.h>
#include <Styling/AppStyle.h>
#include <Widgets/Layout/SBorder.h>
#include <Widgets/Layout/SBox.h>
#include <Widgets/Layout/SSeparator.h>
#include <Widgets/Layout/SScrollBox.h>

// --------------------------------------------------------------------------------------------------------------------

namespace BehaviorTreeExporterTab_Constants
{
    constexpr float PanelPadding = 8.0f;
    constexpr float ButtonWidth = 180.0f;
    constexpr float SectionSpacing = 8.0f;

    const FLinearColor Color_Success{0.2f, 0.8f, 0.2f};
    const FLinearColor Color_Error{0.9f, 0.2f, 0.2f};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkBehaviorTreeExporterTab::
    Construct(const FArguments& InArgs)
    -> void
{
    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(BehaviorTreeExporterTab_Constants::PanelPadding)
        [
            SNew(SVerticalBox)

            // Button bar
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, BehaviorTreeExporterTab_Constants::SectionSpacing)
            [
                DoCreateButtonBar()
            ]

            // Separator
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, BehaviorTreeExporterTab_Constants::SectionSpacing)
            [
                SNew(SSeparator)
            ]

            // Status text
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, BehaviorTreeExporterTab_Constants::SectionSpacing)
            [
                SAssignNew(_StatusText, STextBlock)
                .Text(FText::FromString(TEXT("Select assets in the Content Browser, then click Export.")))
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
    SCkBehaviorTreeExporterTab::
    DoCreateButtonBar()
    -> TSharedRef<SWidget>
{
    return SNew(SHorizontalBox)

        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0, 0, BehaviorTreeExporterTab_Constants::SectionSpacing, 0)
        [
            SNew(SBox)
            .WidthOverride(BehaviorTreeExporterTab_Constants::ButtonWidth)
            [
                SNew(SButton)
                .Text(FText::FromString(TEXT("Export Selected BTs")))
                .ToolTipText(FText::FromString(TEXT("Export Behavior Trees currently selected in the Content Browser to JSON and Text files")))
                .OnClicked(this, &SCkBehaviorTreeExporterTab::DoOnExportSelectedClicked)
                .HAlign(HAlign_Center)
            ]
        ]

        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0, 0, BehaviorTreeExporterTab_Constants::SectionSpacing, 0)
        [
            SNew(SBox)
            .WidthOverride(200.0f)
            [
                SNew(SButton)
                .Text(FText::FromString(TEXT("Export Selected Blueprints")))
                .ToolTipText(FText::FromString(TEXT("Export Blueprints currently selected in the Content Browser to JSON and Text files")))
                .OnClicked(this, &SCkBehaviorTreeExporterTab::DoOnExportSelectedBlueprintsClicked)
                .HAlign(HAlign_Center)
            ]
        ]

        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0, 0, BehaviorTreeExporterTab_Constants::SectionSpacing, 0)
        [
            SNew(SBox)
            .WidthOverride(200.0f)
            [
                SNew(SButton)
                .Text(FText::FromString(TEXT("Export Selected DataAssets")))
                .ToolTipText(FText::FromString(TEXT("Export DataAssets currently selected in the Content Browser to JSON and Text files")))
                .OnClicked(this, &SCkBehaviorTreeExporterTab::DoOnExportSelectedDataAssetsClicked)
                .HAlign(HAlign_Center)
            ]
        ]

        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0, 0, BehaviorTreeExporterTab_Constants::SectionSpacing, 0)
        [
            SNew(SBox)
            .WidthOverride(200.0f)
            [
                SNew(SButton)
                .Text(FText::FromString(TEXT("Export Selected EQS")))
                .ToolTipText(FText::FromString(TEXT("Export EQS Queries currently selected in the Content Browser to JSON and Text files")))
                .OnClicked(this, &SCkBehaviorTreeExporterTab::DoOnExportSelectedEQSClicked)
                .HAlign(HAlign_Center)
            ]
        ];
}

auto
    SCkBehaviorTreeExporterTab::
    DoCreateResultsPanel()
    -> TSharedRef<SWidget>
{
    return SAssignNew(_ResultsListView,
        SListView<TSharedPtr<FCk_BehaviorTreeExporterTab_ResultEntry>>)
        .ListItemsSource(&_ResultEntries)
        .OnGenerateRow(this, &SCkBehaviorTreeExporterTab::DoGenerateResultRow)
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
    SCkBehaviorTreeExporterTab::
    DoOnExportSelectedClicked()
    -> FReply
{
    // Get selected BTs from Content Browser
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
        if (R.bSuccess) { ++SuccessCount; }
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
    SCkBehaviorTreeExporterTab::
    DoRefreshResultsList(
        const TArray<FCk_BehaviorTreeExportResult>& InResults)
    -> void
{
    _ResultEntries.Empty();

    for (const auto& Result : InResults)
    {
        auto Entry = MakeShared<FCk_BehaviorTreeExporterTab_ResultEntry>();
        Entry->AssetName = Result.AssetName;
        Entry->bSuccess = Result.bSuccess;
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
    SCkBehaviorTreeExporterTab::
    DoOnExportSelectedBlueprintsClicked()
    -> FReply
{
    // Get selected Blueprints from Content Browser
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
        if (R.bSuccess) { ++SuccessCount; }
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
    SCkBehaviorTreeExporterTab::
    DoRefreshResultsListFromBlueprintResults(
        const TArray<FCk_BlueprintExportResult>& InResults)
    -> void
{
    _ResultEntries.Empty();

    for (const auto& Result : InResults)
    {
        auto Entry = MakeShared<FCk_BehaviorTreeExporterTab_ResultEntry>();
        Entry->AssetName = Result.AssetName;
        Entry->bSuccess = Result.bSuccess;
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
    SCkBehaviorTreeExporterTab::
    DoOnExportSelectedDataAssetsClicked()
    -> FReply
{
    // Get selected DataAssets from Content Browser
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
        if (R.bSuccess) { ++SuccessCount; }
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
    SCkBehaviorTreeExporterTab::
    DoRefreshResultsListFromDataAssetResults(
        const TArray<FCk_DataAssetExportResult>& InResults)
    -> void
{
    _ResultEntries.Empty();

    for (const auto& Result : InResults)
    {
        auto Entry = MakeShared<FCk_BehaviorTreeExporterTab_ResultEntry>();
        Entry->AssetName = Result.AssetName;
        Entry->bSuccess = Result.bSuccess;
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
    SCkBehaviorTreeExporterTab::
    DoOnExportSelectedEQSClicked()
    -> FReply
{
    // Get selected EQS Queries from Content Browser
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
        if (R.bSuccess) { ++SuccessCount; }
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
    SCkBehaviorTreeExporterTab::
    DoRefreshResultsListFromEQSResults(
        const TArray<FCk_EQSExportResult>& InResults)
    -> void
{
    _ResultEntries.Empty();

    for (const auto& Result : InResults)
    {
        auto Entry = MakeShared<FCk_BehaviorTreeExporterTab_ResultEntry>();
        Entry->AssetName = Result.AssetName;
        Entry->bSuccess = Result.bSuccess;
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
    SCkBehaviorTreeExporterTab::
    DoGenerateResultRow(
        TSharedPtr<FCk_BehaviorTreeExporterTab_ResultEntry> InEntry,
        const TSharedRef<STableViewBase>& InOwnerTable)
    -> TSharedRef<ITableRow>
{
    const auto StatusColor = InEntry->bSuccess
        ? BehaviorTreeExporterTab_Constants::Color_Success
        : BehaviorTreeExporterTab_Constants::Color_Error;

    const auto StatusLabel = InEntry->bSuccess ? TEXT("OK") : TEXT("FAIL");

    const auto OutputText = InEntry->bSuccess
        ? FString::Printf(TEXT("%s\n%s"), *InEntry->JsonPath, *InEntry->TextPath)
        : InEntry->ErrorMessage;

    return SNew(STableRow<TSharedPtr<FCk_BehaviorTreeExporterTab_ResultEntry>>, InOwnerTable)
        [
            SNew(SHorizontalBox)

            // Status
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(4.0f, 2.0f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(StatusLabel))
                .ColorAndOpacity(FSlateColor(StatusColor))
            ]

            // Asset name
            + SHorizontalBox::Slot()
            .FillWidth(0.3f)
            .VAlign(VAlign_Center)
            .Padding(4.0f, 2.0f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(InEntry->AssetName))
            ]

            // Output path
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
