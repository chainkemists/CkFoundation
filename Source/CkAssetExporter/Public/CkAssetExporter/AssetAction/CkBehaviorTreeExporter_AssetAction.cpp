#include "CkBehaviorTreeExporter_AssetAction.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/BehaviorTreeExporter/CkBehaviorTreeExporter.h"

#include <BehaviorTree/BehaviorTree.h>
#include <ContentBrowserModule.h>
#include <IContentBrowserSingleton.h>
#include <ToolMenus.h>
#include <ToolMenuContext.h>

#define LOCTEXT_NAMESPACE "CkAssetExporter"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::asset_exporter
{

static auto
    DoGetSelectedBehaviorTrees()
    -> TArray<UBehaviorTree*>
{
    auto BehaviorTrees = TArray<UBehaviorTree*>{};

    auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    auto SelectedAssets = TArray<FAssetData>{};
    ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

    for (const auto& AssetData : SelectedAssets)
    {
        if (auto* BT = Cast<UBehaviorTree>(AssetData.GetAsset()))
        {
            BehaviorTrees.Add(BT);
        }
    }

    return BehaviorTrees;
}

static auto
    DoShowExportNotification(
        const TArray<FCk_BehaviorTreeExportResult>& InResults)
    -> void
{
    auto SuccessCount = int32{0};
    auto FailCount = int32{0};

    for (const auto& Result : InResults)
    {
        if (Result.bSuccess)
        {
            ++SuccessCount;
            UE_LOG(CkAssetExporter, Log, TEXT("Exported BT '%s': %s, %s"),
                *Result.AssetName, *Result.JsonFilePath, *Result.TextFilePath);
        }
        else
        {
            ++FailCount;
            UE_LOG(CkAssetExporter, Error, TEXT("Failed to export BT '%s': %s"),
                *Result.AssetName, *Result.ErrorMessage);
        }
    }

    if (FailCount == 0)
    {
        UE_LOG(CkAssetExporter, Log, TEXT("Successfully exported %d Behavior Tree(s)"), SuccessCount);
    }
    else
    {
        UE_LOG(CkAssetExporter, Warning, TEXT("Exported %d Behavior Tree(s), %d failed"), SuccessCount, FailCount);
    }
}

static auto
    DoExecuteExportAction_BehaviorTree()
    -> void
{
    const auto BehaviorTrees = DoGetSelectedBehaviorTrees();

    if (BehaviorTrees.Num() == 0)
    {
        UE_LOG(CkAssetExporter, Warning, TEXT("No Behavior Tree assets selected"));
        return;
    }

    const auto Results = FCk_BehaviorTreeExporter::ExportBehaviorTrees(BehaviorTrees);
    DoShowExportNotification(Results);
}

auto
    RegisterBehaviorTreeContextMenu()
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

    Section.AddDynamicEntry("ExportBehaviorTree",
        FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
        {
            // Check if any selected asset is a BehaviorTree
            const auto BehaviorTrees = DoGetSelectedBehaviorTrees();

            if (BehaviorTrees.Num() == 0)
            { return; }

            InSection.AddMenuEntry(
                "ExportBT_JsonAndText",
                LOCTEXT("ExportBT_Label", "Export to JSON && Text"),
                LOCTEXT("ExportBT_Tooltip",
                    "Export selected Behavior Trees to .json and .txt files next to the asset"),
                FSlateIcon(),
                FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& /*InContext*/)
                {
                    DoExecuteExportAction_BehaviorTree();
                })
            );
        })
    );
}

} // namespace ck::asset_exporter

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
