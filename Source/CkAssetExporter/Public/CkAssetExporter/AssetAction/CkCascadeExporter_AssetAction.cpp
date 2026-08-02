#include "CkCascadeExporter_AssetAction.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/CascadeExporter/CkCascadeExporter.h"

#include "Particles/ParticleSystem.h"

#include <ContentBrowserModule.h>
#include <IContentBrowserSingleton.h>
#include <ToolMenus.h>
#include <ToolMenuContext.h>

#define LOCTEXT_NAMESPACE "CkAssetExporter"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::asset_exporter
{
    static auto
        DoGetSelectedParticleSystems()
        -> TArray<UParticleSystem*>
    {
        auto Systems = TArray<UParticleSystem*>{};

        auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
        auto SelectedAssets = TArray<FAssetData>{};
        ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

        for (const auto& AssetData : SelectedAssets)
        {
            if (auto* Sys = Cast<UParticleSystem>(AssetData.GetAsset()))
            { Systems.Add(Sys); }
        }
        return Systems;
    }

    static auto
        DoExecuteCascadeExportAction()
        -> void
    {
        const auto Systems = DoGetSelectedParticleSystems();
        if (Systems.Num() == 0)
        {
            UE_LOG(CkAssetExporter, Warning, TEXT("No Cascade Particle System assets selected"));
            return;
        }

        const auto Results = FCk_CascadeExporter::ExportParticleSystems(Systems);
        auto SuccessCount = int32{0};
        auto FailCount = int32{0};
        for (const auto& Result : Results)
        {
            if (Result.Succeeded)
            {
                ++SuccessCount;
                UE_LOG(CkAssetExporter, Log, TEXT("Exported Cascade System '%s': %s, %s"),
                    *Result.AssetName, *Result.JsonFilePath, *Result.TextFilePath);
            }
            else
            {
                ++FailCount;
                UE_LOG(CkAssetExporter, Error, TEXT("Failed to export Cascade System '%s': %s"),
                    *Result.AssetName, *Result.ErrorMessage);
            }
        }

        if (FailCount == 0)
        { UE_LOG(CkAssetExporter, Log, TEXT("Successfully exported %d Cascade System(s)"), SuccessCount); }
        else
        { UE_LOG(CkAssetExporter, Warning, TEXT("Exported %d Cascade System(s), %d failed"), SuccessCount, FailCount); }
    }

    auto
        RegisterCascadeContextMenu()
        -> void
    {
        auto* ToolMenus = UToolMenus::Get();
        if (ToolMenus == nullptr)
        { return; }

        auto* Menu = ToolMenus->ExtendMenu("ContentBrowser.AssetContextMenu");
        if (Menu == nullptr)
        { return; }

        auto& Section = Menu->FindOrAddSection("CkAssetExporterActions");

        Section.AddDynamicEntry("ExportCascade",
            FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
            {
                const auto Systems = DoGetSelectedParticleSystems();
                if (Systems.Num() == 0)
                { return; }

                InSection.AddMenuEntry(
                    "ExportCascade_JsonAndText",
                    LOCTEXT("ExportCascade_Label", "Export Cascade System to JSON"),
                    LOCTEXT("ExportCascade_Tooltip",
                        "Export selected Cascade Particle Systems (emitters, modules, distributions) to a .ckexport file next to the asset"),
                    FSlateIcon(),
                    FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& /*InContext*/)
                    {
                        DoExecuteCascadeExportAction();
                    })
                );
            })
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
