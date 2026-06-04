#pragma once

#include "CkAssetExporter/BehaviorTreeExporter/CkBehaviorTreeExporter.h"
#include "CkAssetExporter/BlueprintExporter/CkBlueprintExporter.h"
#include "CkAssetExporter/DataAssetExporter/CkDataAssetExporter.h"
#include "CkAssetExporter/EQSExporter/CkEQSExporter.h"
#include "CkAssetExporter/StateTreeExporter/CkStateTreeExporter.h"
#include "CkAssetExporter/UserDefinedStructExporter/CkUserDefinedStructExporter.h"

#include <Widgets/SCompoundWidget.h>
#include <Widgets/Views/SListView.h>
#include <Widgets/Text/STextBlock.h>
#include <Widgets/Input/SButton.h>

// --------------------------------------------------------------------------------------------------------------------

struct FCk_AssetExporterTab_ResultEntry
{
    FString AssetName;
    FString JsonPath;
    FString TextPath;
    bool Succeeded = false;
    FString ErrorMessage;
};

// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API SCkAssetExporterTab : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCkAssetExporterTab) {}
    SLATE_END_ARGS()

    auto Construct(const FArguments& InArgs) -> void;

private:
    auto DoCreateButtonBar() -> TSharedRef<SWidget>;
    auto DoCreateResultsPanel() -> TSharedRef<SWidget>;

    auto DoOnExportSelectedClicked() -> FReply;
    auto DoOnExportSelectedBlueprintsClicked() -> FReply;
    auto DoOnExportSelectedDataAssetsClicked() -> FReply;
    auto DoOnExportSelectedEQSClicked() -> FReply;
    auto DoOnExportSelectedStateTreesClicked() -> FReply;
    auto DoOnExportSelectedUserDefinedStructsClicked() -> FReply;
    auto DoRefreshResultsList(const TArray<FCk_BehaviorTreeExportResult>& InResults) -> void;
    auto DoRefreshResultsListFromBlueprintResults(const TArray<FCk_BlueprintExportResult>& InResults) -> void;
    auto DoRefreshResultsListFromDataAssetResults(const TArray<FCk_DataAssetExportResult>& InResults) -> void;
    auto DoRefreshResultsListFromEQSResults(const TArray<FCk_EQSExportResult>& InResults) -> void;
    auto DoRefreshResultsListFromStateTreeResults(const TArray<FCk_StateTreeExportResult>& InResults) -> void;
    auto DoRefreshResultsListFromUserDefinedStructResults(const TArray<FCk_UserDefinedStructExportResult>& InResults) -> void;

    auto DoGenerateResultRow(
        TSharedPtr<FCk_AssetExporterTab_ResultEntry> InEntry,
        const TSharedRef<STableViewBase>& InOwnerTable) -> TSharedRef<ITableRow>;

private:
    TSharedPtr<STextBlock> _StatusText;
    TSharedPtr<SListView<TSharedPtr<FCk_AssetExporterTab_ResultEntry>>> _ResultsListView;
    TArray<TSharedPtr<FCk_AssetExporterTab_ResultEntry>> _ResultEntries;
};

// --------------------------------------------------------------------------------------------------------------------
