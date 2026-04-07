#include "CkEQSExporter_AssetAction.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/EQSExporter/CkEQSExporter.h"

#include <EnvironmentQuery/EnvQuery.h>
#include <ContentBrowserModule.h>
#include <IContentBrowserSingleton.h>
#include <ToolMenus.h>
#include <ToolMenuContext.h>

#define LOCTEXT_NAMESPACE "CkAssetExporter"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::asset_exporter
{

static auto
    DoGetSelectedEQSQueries()
    -> TArray<UEnvQuery*>
{
    auto Queries = TArray<UEnvQuery*>{};

    auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    auto SelectedAssets = TArray<FAssetData>{};
    ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

    for (const auto& AssetData : SelectedAssets)
    {
        if (auto* Query = Cast<UEnvQuery>(AssetData.GetAsset()))
        {
            Queries.Add(Query);
        }
    }

    return Queries;
}

static auto
    DoShowExportNotification(
        const TArray<FCk_EQSExportResult>& InResults)
    -> void
{
    auto SuccessCount = int32{0};
    auto FailCount = int32{0};

    for (const auto& Result : InResults)
    {
        if (Result.bSuccess)
        {
            ++SuccessCount;
            UE_LOG(CkAssetExporter, Log, TEXT("Exported EQS '%s': %s, %s"),
                *Result.AssetName, *Result.JsonFilePath, *Result.TextFilePath);
        }
        else
        {
            ++FailCount;
            UE_LOG(CkAssetExporter, Error, TEXT("Failed to export EQS '%s': %s"),
                *Result.AssetName, *Result.ErrorMessage);
        }
    }

    if (FailCount == 0)
    {
        UE_LOG(CkAssetExporter, Log, TEXT("Successfully exported %d EQS Query(s)"), SuccessCount);
    }
    else
    {
        UE_LOG(CkAssetExporter, Warning, TEXT("Exported %d EQS Query(s), %d failed"), SuccessCount, FailCount);
    }
}

static auto
    DoExecuteExportAction_EQS()
    -> void
{
    const auto Queries = DoGetSelectedEQSQueries();

    if (Queries.Num() == 0)
    {
        UE_LOG(CkAssetExporter, Warning, TEXT("No EQS Query assets selected"));
        return;
    }

    const auto Results = FCk_EQSExporter::ExportEQSQueries(Queries);
    DoShowExportNotification(Results);
}

auto
    RegisterEQSContextMenu()
    -> void
{
    auto* ToolMenus = UToolMenus::Get();
    if (ToolMenus == nullptr)
    { return; }

    auto* Menu = ToolMenus->ExtendMenu("ContentBrowser.AssetContextMenu");
    if (Menu == nullptr)
    { return; }

    auto& Section = Menu->FindOrAddSection("CkAssetExporterActions");
    Section.Label = LOCTEXT("CkAssetExporterSection", "CK Asset Exporter");

    Section.AddDynamicEntry("ExportEQSQuery",
        FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
        {
            // Check if any selected asset is an EQS Query
            const auto Queries = DoGetSelectedEQSQueries();

            if (Queries.Num() == 0)
            { return; }

            InSection.AddMenuEntry(
                "ExportEQS_JsonAndText",
                LOCTEXT("ExportEQS_Label", "Export EQS to JSON && Text"),
                LOCTEXT("ExportEQS_Tooltip",
                    "Export selected EQS Queries to .json and .txt files next to the asset"),
                FSlateIcon(),
                FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& /*InContext*/)
                {
                    DoExecuteExportAction_EQS();
                })
            );
        })
    );
}

} // namespace ck::asset_exporter

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
