#pragma once

#include "CkAssetExporter/BehaviorTreeExporter/CkBehaviorTreeExporter.h"
#include "CkAssetExporter/BlueprintExporter/CkBlueprintExporter.h"
#include "CkAssetExporter/DataAssetExporter/CkDataAssetExporter.h"
#include "CkAssetExporter/EQSExporter/CkEQSExporter.h"

#include <Widgets/SCompoundWidget.h>
#include <Widgets/Views/SListView.h>
#include <Widgets/Text/STextBlock.h>
#include <Widgets/Input/SButton.h>

// --------------------------------------------------------------------------------------------------------------------

struct FCk_BehaviorTreeExporterTab_ResultEntry
{
    FString AssetName;
    FString JsonPath;
    FString TextPath;
    bool bSuccess = false;
    FString ErrorMessage;
};

// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API SCkBehaviorTreeExporterTab : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCkBehaviorTreeExporterTab) {}
    SLATE_END_ARGS()

    auto Construct(const FArguments& InArgs) -> void;

private:
    auto DoCreateButtonBar() -> TSharedRef<SWidget>;
    auto DoCreateResultsPanel() -> TSharedRef<SWidget>;

    auto DoOnExportSelectedClicked() -> FReply;
    auto DoOnExportSelectedBlueprintsClicked() -> FReply;
    auto DoOnExportSelectedDataAssetsClicked() -> FReply;
    auto DoOnExportSelectedEQSClicked() -> FReply;
    auto DoRefreshResultsList(const TArray<FCk_BehaviorTreeExportResult>& InResults) -> void;
    auto DoRefreshResultsListFromBlueprintResults(const TArray<FCk_BlueprintExportResult>& InResults) -> void;
    auto DoRefreshResultsListFromDataAssetResults(const TArray<FCk_DataAssetExportResult>& InResults) -> void;
    auto DoRefreshResultsListFromEQSResults(const TArray<FCk_EQSExportResult>& InResults) -> void;

    auto DoGenerateResultRow(
        TSharedPtr<FCk_BehaviorTreeExporterTab_ResultEntry> InEntry,
        const TSharedRef<STableViewBase>& InOwnerTable) -> TSharedRef<ITableRow>;

private:
    TSharedPtr<STextBlock> _StatusText;
    TSharedPtr<SListView<TSharedPtr<FCk_BehaviorTreeExporterTab_ResultEntry>>> _ResultsListView;
    TArray<TSharedPtr<FCk_BehaviorTreeExporterTab_ResultEntry>> _ResultEntries;
};

// --------------------------------------------------------------------------------------------------------------------
