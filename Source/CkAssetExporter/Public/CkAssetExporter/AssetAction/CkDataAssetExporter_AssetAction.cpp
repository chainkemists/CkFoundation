#include "CkDataAssetExporter_AssetAction.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/DataAssetExporter/CkDataAssetExporter.h"

#include <Engine/DataAsset.h>
#include <StateTree.h>
#include <EnvironmentQuery/EnvQuery.h>
#include <ContentBrowserModule.h>
#include <IContentBrowserSingleton.h>
#include <ToolMenus.h>
#include <ToolMenuContext.h>

#define LOCTEXT_NAMESPACE "CkAssetExporter"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::asset_exporter
{

// Some UDataAsset subclasses (e.g. UStateTree, UEnvQuery) have their own dedicated,
// structure-aware exporter. Exclude them from the generic DataAsset exporter so the
// content-browser menu only offers the most specific option per asset type.
static auto
    DoHasDedicatedExporter(
        const UClass* InClass)
    -> bool
{
    if (InClass == nullptr)
    { return false; }

    static const TArray<const UClass*> ClassesWithDedicatedExporter = {
        UStateTree::StaticClass(),
        UEnvQuery::StaticClass(),
    };

    for (const auto* DedicatedClass : ClassesWithDedicatedExporter)
    {
        if (InClass->IsChildOf(DedicatedClass))
        { return true; }
    }

    return false;
}

static auto
    DoGetSelectedDataAssets()
    -> TArray<UDataAsset*>
{
    auto DataAssets = TArray<UDataAsset*>{};

    auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    auto SelectedAssets = TArray<FAssetData>{};
    ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

    for (const auto& AssetData : SelectedAssets)
    {
        if (auto* DA = Cast<UDataAsset>(AssetData.GetAsset()))
        {
            if (DoHasDedicatedExporter(DA->GetClass()))
            { continue; }

            DataAssets.Add(DA);
        }
    }

    return DataAssets;
}

static auto
    DoShowDataAssetExportNotification(
        const TArray<FCk_DataAssetExportResult>& InResults)
    -> void
{
    auto SuccessCount = int32{0};
    auto FailCount = int32{0};

    for (const auto& Result : InResults)
    {
        if (Result.Succeeded)
        {
            ++SuccessCount;
            UE_LOG(CkAssetExporter, Log, TEXT("Exported DataAsset '%s': %s, %s"),
                *Result.AssetName, *Result.JsonFilePath, *Result.TextFilePath);
        }
        else
        {
            ++FailCount;
            UE_LOG(CkAssetExporter, Error, TEXT("Failed to export DataAsset '%s': %s"),
                *Result.AssetName, *Result.ErrorMessage);
        }
    }

    if (FailCount == 0)
    {
        UE_LOG(CkAssetExporter, Log, TEXT("Successfully exported %d DataAsset(s)"), SuccessCount);
    }
    else
    {
        UE_LOG(CkAssetExporter, Warning, TEXT("Exported %d DataAsset(s), %d failed"), SuccessCount, FailCount);
    }
}

static auto
    DoExecuteDataAssetExportAction()
    -> void
{
    const auto DataAssets = DoGetSelectedDataAssets();

    if (DataAssets.Num() == 0)
    {
        UE_LOG(CkAssetExporter, Warning, TEXT("No DataAsset assets selected"));
        return;
    }

    const auto Results = FCk_DataAssetExporter::ExportDataAssets(DataAssets);
    DoShowDataAssetExportNotification(Results);
}

auto
    RegisterDataAssetContextMenu()
    -> void
{
    auto* ToolMenus = UToolMenus::Get();
    if (ToolMenus == nullptr)
    { return; }

    auto* Menu = ToolMenus->ExtendMenu("ContentBrowser.AssetContextMenu");
    if (Menu == nullptr)
    { return; }

    auto& Section = Menu->FindOrAddSection("CkAssetExporterActions");

    Section.AddDynamicEntry("ExportDataAsset",
        FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
        {
            const auto DataAssets = DoGetSelectedDataAssets();

            if (DataAssets.Num() == 0)
            { return; }

            InSection.AddMenuEntry(
                "ExportDA_JsonAndText",
                LOCTEXT("ExportDA_Label", "Export DataAsset to JSON"),
                LOCTEXT("ExportDA_Tooltip",
                    "Export selected DataAssets to a .ckexport file next to the asset"),
                FSlateIcon(),
                FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& /*InContext*/)
                {
                    DoExecuteDataAssetExportAction();
                })
            );
        })
    );
}

} // namespace ck::asset_exporter

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
