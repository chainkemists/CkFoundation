#include "CkBlueprintExporter_AssetAction.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/BlueprintExporter/CkBlueprintExporter.h"

#include <Engine/Blueprint.h>
#include <ContentBrowserModule.h>
#include <IContentBrowserSingleton.h>
#include <ToolMenus.h>
#include <ToolMenuContext.h>

#define LOCTEXT_NAMESPACE "CkAssetExporter"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::asset_exporter
{

static auto
    DoGetSelectedBlueprints()
    -> TArray<UBlueprint*>
{
    auto Blueprints = TArray<UBlueprint*>{};

    auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    auto SelectedAssets = TArray<FAssetData>{};
    ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

    for (const auto& AssetData : SelectedAssets)
    {
        if (auto* BP = Cast<UBlueprint>(AssetData.GetAsset()))
        {
            Blueprints.Add(BP);
        }
    }

    return Blueprints;
}

static auto
    DoShowBlueprintExportNotification(
        const TArray<FCk_BlueprintExportResult>& InResults)
    -> void
{
    auto SuccessCount = int32{0};
    auto FailCount = int32{0};

    for (const auto& Result : InResults)
    {
        if (Result.bSuccess)
        {
            ++SuccessCount;
            UE_LOG(CkAssetExporter, Log, TEXT("Exported Blueprint '%s': %s, %s"),
                *Result.AssetName, *Result.JsonFilePath, *Result.TextFilePath);
        }
        else
        {
            ++FailCount;
            UE_LOG(CkAssetExporter, Error, TEXT("Failed to export Blueprint '%s': %s"),
                *Result.AssetName, *Result.ErrorMessage);
        }
    }

    if (FailCount == 0)
    {
        UE_LOG(CkAssetExporter, Log, TEXT("Successfully exported %d Blueprint(s)"), SuccessCount);
    }
    else
    {
        UE_LOG(CkAssetExporter, Warning, TEXT("Exported %d Blueprint(s), %d failed"), SuccessCount, FailCount);
    }
}

static auto
    DoExecuteBlueprintExportAction()
    -> void
{
    const auto Blueprints = DoGetSelectedBlueprints();

    if (Blueprints.Num() == 0)
    {
        UE_LOG(CkAssetExporter, Warning, TEXT("No Blueprint assets selected"));
        return;
    }

    const auto Results = FCk_BlueprintExporter::ExportBlueprints(Blueprints);
    DoShowBlueprintExportNotification(Results);
}

auto
    RegisterBlueprintContextMenu()
    -> void
{
    auto* ToolMenus = UToolMenus::Get();
    if (ToolMenus == nullptr)
    { return; }

    auto* Menu = ToolMenus->ExtendMenu("ContentBrowser.AssetContextMenu");
    if (Menu == nullptr)
    { return; }

    auto& Section = Menu->FindOrAddSection("CkAssetExporterActions");

    Section.AddDynamicEntry("ExportBlueprint",
        FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
        {
            const auto Blueprints = DoGetSelectedBlueprints();

            if (Blueprints.Num() == 0)
            { return; }

            InSection.AddMenuEntry(
                "ExportBP_JsonAndText",
                LOCTEXT("ExportBP_Label", "Export Blueprint to JSON && Text"),
                LOCTEXT("ExportBP_Tooltip",
                    "Export selected Blueprints to .json and .txt files next to the asset"),
                FSlateIcon(),
                FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& /*InContext*/)
                {
                    DoExecuteBlueprintExportAction();
                })
            );
        })
    );
}

} // namespace ck::asset_exporter

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
