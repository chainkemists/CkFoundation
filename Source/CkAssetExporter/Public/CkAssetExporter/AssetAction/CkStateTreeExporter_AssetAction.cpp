#include "CkStateTreeExporter_AssetAction.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/StateTreeExporter/CkStateTreeExporter.h"

#include <StateTree.h>
#include <ContentBrowserModule.h>
#include <IContentBrowserSingleton.h>
#include <ToolMenus.h>
#include <ToolMenuContext.h>

#define LOCTEXT_NAMESPACE "CkAssetExporter"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::asset_exporter
{

static auto
    DoGetSelectedStateTrees()
    -> TArray<UStateTree*>
{
    auto StateTrees = TArray<UStateTree*>{};

    auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    auto SelectedAssets = TArray<FAssetData>{};
    ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

    for (const auto& AssetData : SelectedAssets)
    {
        if (auto* ST = Cast<UStateTree>(AssetData.GetAsset()))
        {
            StateTrees.Add(ST);
        }
    }

    return StateTrees;
}

static auto
    DoShowExportNotification(
        const TArray<FCk_StateTreeExportResult>& InResults)
    -> void
{
    auto SuccessCount = int32{0};
    auto FailCount = int32{0};

    for (const auto& Result : InResults)
    {
        if (Result.Succeeded)
        {
            ++SuccessCount;
            UE_LOG(CkAssetExporter, Log, TEXT("Exported StateTree '%s': %s, %s"),
                *Result.AssetName, *Result.JsonFilePath, *Result.TextFilePath);
        }
        else
        {
            ++FailCount;
            UE_LOG(CkAssetExporter, Error, TEXT("Failed to export StateTree '%s': %s"),
                *Result.AssetName, *Result.ErrorMessage);
        }
    }

    if (FailCount == 0)
    {
        UE_LOG(CkAssetExporter, Log, TEXT("Successfully exported %d State Tree(s)"), SuccessCount);
    }
    else
    {
        UE_LOG(CkAssetExporter, Warning, TEXT("Exported %d State Tree(s), %d failed"), SuccessCount, FailCount);
    }
}

static auto
    DoExecuteExportAction_StateTree()
    -> void
{
    const auto StateTrees = DoGetSelectedStateTrees();

    if (StateTrees.Num() == 0)
    {
        UE_LOG(CkAssetExporter, Warning, TEXT("No State Tree assets selected"));
        return;
    }

    const auto Results = FCk_StateTreeExporter::ExportStateTrees(StateTrees);
    DoShowExportNotification(Results);
}

auto
    RegisterStateTreeContextMenu()
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

    Section.AddDynamicEntry("ExportStateTree",
        FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
        {
            // Check if any selected asset is a StateTree
            const auto StateTrees = DoGetSelectedStateTrees();

            if (StateTrees.Num() == 0)
            { return; }

            InSection.AddMenuEntry(
                "ExportStateTree_JsonAndText",
                LOCTEXT("ExportStateTree_Label", "Export StateTree to JSON && Text"),
                LOCTEXT("ExportStateTree_Tooltip",
                    "Export selected State Trees to .json and .txt files next to the asset"),
                FSlateIcon(),
                FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& /*InContext*/)
                {
                    DoExecuteExportAction_StateTree();
                })
            );
        })
    );
}

} // namespace ck::asset_exporter

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
