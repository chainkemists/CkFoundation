#include "CkMaterialExporter_AssetAction.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/MaterialExporter/CkMaterialExporter.h"

#include "Materials/MaterialInterface.h"
#include "Materials/MaterialFunctionInterface.h"

#include <ContentBrowserModule.h>
#include <IContentBrowserSingleton.h>
#include <ToolMenus.h>
#include <ToolMenuContext.h>

#define LOCTEXT_NAMESPACE "CkAssetExporter"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::asset_exporter
{
    static auto
        DoGetSelectedMaterials()
        -> TArray<UMaterialInterface*>
    {
        auto Materials = TArray<UMaterialInterface*>{};

        auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
        auto SelectedAssets = TArray<FAssetData>{};
        ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

        for (const auto& AssetData : SelectedAssets)
        {
            if (auto* Mat = Cast<UMaterialInterface>(AssetData.GetAsset()))
            { Materials.Add(Mat); }
        }
        return Materials;
    }

    static auto
        DoGetSelectedMaterialFunctions()
        -> TArray<UMaterialFunctionInterface*>
    {
        auto Functions = TArray<UMaterialFunctionInterface*>{};

        auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
        auto SelectedAssets = TArray<FAssetData>{};
        ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

        for (const auto& AssetData : SelectedAssets)
        {
            if (auto* Function = Cast<UMaterialFunctionInterface>(AssetData.GetAsset()))
            { Functions.Add(Function); }
        }
        return Functions;
    }

    static auto
        DoShowMaterialExportNotification(
            const TArray<FCk_MaterialExportResult>& InResults)
        -> void
    {
        auto SuccessCount = int32{0};
        auto FailCount = int32{0};
        for (const auto& Result : InResults)
        {
            if (Result.Succeeded)
            {
                ++SuccessCount;
                UE_LOG(CkAssetExporter, Log, TEXT("Exported Material '%s': %s"),
                    *Result.AssetName, *Result.JsonFilePath);
            }
            else
            {
                ++FailCount;
                UE_LOG(CkAssetExporter, Error, TEXT("Failed to export Material '%s': %s"),
                    *Result.AssetName, *Result.ErrorMessage);
            }
        }

        if (FailCount == 0)
        { UE_LOG(CkAssetExporter, Log, TEXT("Successfully exported %d Material(s)"), SuccessCount); }
        else
        { UE_LOG(CkAssetExporter, Warning, TEXT("Exported %d Material(s), %d failed"), SuccessCount, FailCount); }
    }

    static auto
        DoExecuteMaterialExportAction()
        -> void
    {
        const auto Materials = DoGetSelectedMaterials();
        if (Materials.Num() == 0)
        {
            UE_LOG(CkAssetExporter, Warning, TEXT("No Material assets selected"));
            return;
        }
        const auto Results = FCk_MaterialExporter::ExportMaterials(Materials);
        DoShowMaterialExportNotification(Results);
    }

    auto
        RegisterMaterialContextMenu()
        -> void
    {
        auto* ToolMenus = UToolMenus::Get();
        if (ToolMenus == nullptr)
        { return; }

        auto* Menu = ToolMenus->ExtendMenu("ContentBrowser.AssetContextMenu");
        if (Menu == nullptr)
        { return; }

        auto& Section = Menu->FindOrAddSection("CkAssetExporterActions");

        Section.AddDynamicEntry("ExportMaterial",
            FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
            {
                const auto Materials = DoGetSelectedMaterials();
                if (Materials.Num() == 0)
                { return; }

                InSection.AddMenuEntry(
                    "ExportMaterial_Json",
                    LOCTEXT("ExportMaterial_Label", "Export Material to JSON"),
                    LOCTEXT("ExportMaterial_Tooltip",
                        "Export selected Materials / Material Instances (params, instance chain, expression histogram, textures) to .json next to the asset"),
                    FSlateIcon(),
                    FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& /*InContext*/)
                    {
                        DoExecuteMaterialExportAction();
                    })
                );
            })
        );
    }

    auto
        RegisterMaterialFunctionContextMenu()
        -> void
    {
        auto* ToolMenus = UToolMenus::Get();
        if (ToolMenus == nullptr)
        { return; }

        auto* Menu = ToolMenus->ExtendMenu("ContentBrowser.AssetContextMenu");
        if (Menu == nullptr)
        { return; }

        auto& Section = Menu->FindOrAddSection("CkAssetExporterActions");

        Section.AddDynamicEntry("ExportMaterialFunction",
            FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
            {
                const auto Functions = DoGetSelectedMaterialFunctions();
                if (Functions.Num() == 0)
                { return; }

                InSection.AddMenuEntry(
                    "ExportMaterialFunction_Json",
                    LOCTEXT("ExportMaterialFunction_Label", "Export Material Function to JSON"),
                    LOCTEXT("ExportMaterialFunction_Tooltip",
                        "Export selected Material Functions (node graph, connectivity, non-default node properties) to .ckexport next to the asset"),
                    FSlateIcon(),
                    FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& /*InContext*/)
                    {
                        const auto Functions = DoGetSelectedMaterialFunctions();
                        if (Functions.Num() == 0)
                        {
                            UE_LOG(CkAssetExporter, Warning, TEXT("No Material Function assets selected"));
                            return;
                        }
                        DoShowMaterialExportNotification(FCk_MaterialExporter::ExportMaterialFunctions(Functions));
                    })
                );
            })
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
