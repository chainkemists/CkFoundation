#include "CkNiagaraExporter_AssetAction.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/NiagaraExporter/CkNiagaraExporter.h"

#include "NiagaraSystem.h"

#include <ContentBrowserModule.h>
#include <IContentBrowserSingleton.h>
#include <ToolMenus.h>
#include <ToolMenuContext.h>

#define LOCTEXT_NAMESPACE "CkAssetExporter"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::asset_exporter
{
    static auto
        DoGetSelectedNiagaraSystems()
        -> TArray<UNiagaraSystem*>
    {
        auto Systems = TArray<UNiagaraSystem*>{};

        auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
        auto SelectedAssets = TArray<FAssetData>{};
        ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

        for (const auto& AssetData : SelectedAssets)
        {
            if (auto* Sys = Cast<UNiagaraSystem>(AssetData.GetAsset()))
            { Systems.Add(Sys); }
        }
        return Systems;
    }

    static auto
        DoShowNiagaraExportNotification(
            const TArray<FCk_NiagaraExportResult>& InResults)
        -> void
    {
        auto SuccessCount = int32{0};
        auto FailCount = int32{0};
        for (const auto& Result : InResults)
        {
            if (Result.Succeeded)
            {
                ++SuccessCount;
                UE_LOG(CkAssetExporter, Log, TEXT("Exported Niagara System '%s': %s, %s"),
                    *Result.AssetName, *Result.JsonFilePath, *Result.TextFilePath);
            }
            else
            {
                ++FailCount;
                UE_LOG(CkAssetExporter, Error, TEXT("Failed to export Niagara System '%s': %s"),
                    *Result.AssetName, *Result.ErrorMessage);
            }
        }

        if (FailCount == 0)
        { UE_LOG(CkAssetExporter, Log, TEXT("Successfully exported %d Niagara System(s)"), SuccessCount); }
        else
        { UE_LOG(CkAssetExporter, Warning, TEXT("Exported %d Niagara System(s), %d failed"), SuccessCount, FailCount); }
    }

    static auto
        DoExecuteNiagaraExportAction()
        -> void
    {
        const auto Systems = DoGetSelectedNiagaraSystems();
        if (Systems.Num() == 0)
        {
            UE_LOG(CkAssetExporter, Warning, TEXT("No Niagara System assets selected"));
            return;
        }
        const auto Results = FCk_NiagaraExporter::ExportNiagaraSystems(Systems);
        DoShowNiagaraExportNotification(Results);
    }

    auto
        RegisterNiagaraContextMenu()
        -> void
    {
        auto* ToolMenus = UToolMenus::Get();
        if (ToolMenus == nullptr)
        { return; }

        auto* Menu = ToolMenus->ExtendMenu("ContentBrowser.AssetContextMenu");
        if (Menu == nullptr)
        { return; }

        auto& Section = Menu->FindOrAddSection("CkAssetExporterActions");

        Section.AddDynamicEntry("ExportNiagara",
            FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
            {
                const auto Systems = DoGetSelectedNiagaraSystems();
                if (Systems.Num() == 0)
                { return; }

                InSection.AddMenuEntry(
                    "ExportNiagara_JsonAndText",
                    LOCTEXT("ExportNiagara_Label", "Export Niagara System to JSON"),
                    LOCTEXT("ExportNiagara_Tooltip",
                        "Export selected Niagara Systems (emitters, module stacks, renderers) to a .ckexport file next to the asset"),
                    FSlateIcon(),
                    FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& /*InContext*/)
                    {
                        DoExecuteNiagaraExportAction();
                    })
                );
            })
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
