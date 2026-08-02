#include "CkUserDefinedStructExporter_AssetAction.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/UserDefinedStructExporter/CkUserDefinedStructExporter.h"

#include <StructUtils/UserDefinedStruct.h>
#include <ContentBrowserModule.h>
#include <IContentBrowserSingleton.h>
#include <ToolMenus.h>
#include <ToolMenuContext.h>

#define LOCTEXT_NAMESPACE "CkAssetExporter"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::asset_exporter
{

static auto
    DoGetSelectedUserDefinedStructs()
    -> TArray<UUserDefinedStruct*>
{
    auto Structs = TArray<UUserDefinedStruct*>{};

    auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    auto SelectedAssets = TArray<FAssetData>{};
    ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

    for (const auto& AssetData : SelectedAssets)
    {
        if (auto* Struct = Cast<UUserDefinedStruct>(AssetData.GetAsset()))
        {
            Structs.Add(Struct);
        }
    }

    return Structs;
}

static auto
    DoShowExportNotification(
        const TArray<FCk_UserDefinedStructExportResult>& InResults)
    -> void
{
    auto SuccessCount = int32{0};
    auto FailCount = int32{0};

    for (const auto& Result : InResults)
    {
        if (Result.Succeeded)
        {
            ++SuccessCount;
            UE_LOG(CkAssetExporter, Log, TEXT("Exported UserDefinedStruct '%s': %s, %s"),
                *Result.AssetName, *Result.JsonFilePath, *Result.TextFilePath);
        }
        else
        {
            ++FailCount;
            UE_LOG(CkAssetExporter, Error, TEXT("Failed to export UserDefinedStruct '%s': %s"),
                *Result.AssetName, *Result.ErrorMessage);
        }
    }

    if (FailCount == 0)
    {
        UE_LOG(CkAssetExporter, Log, TEXT("Successfully exported %d UserDefinedStruct(s)"), SuccessCount);
    }
    else
    {
        UE_LOG(CkAssetExporter, Warning, TEXT("Exported %d UserDefinedStruct(s), %d failed"), SuccessCount, FailCount);
    }
}

static auto
    DoExecuteExportAction_UserDefinedStruct()
    -> void
{
    const auto Structs = DoGetSelectedUserDefinedStructs();

    if (Structs.Num() == 0)
    {
        UE_LOG(CkAssetExporter, Warning, TEXT("No UserDefinedStruct assets selected"));
        return;
    }

    const auto Results = FCk_UserDefinedStructExporter::ExportUserDefinedStructs(Structs);
    DoShowExportNotification(Results);
}

auto
    RegisterUserDefinedStructContextMenu()
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

    Section.AddDynamicEntry("ExportUserDefinedStruct",
        FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
        {
            const auto Structs = DoGetSelectedUserDefinedStructs();

            if (Structs.Num() == 0)
            { return; }

            InSection.AddMenuEntry(
                "ExportUDS_JsonAndText",
                LOCTEXT("ExportUDS_Label", "Export Struct to JSON"),
                LOCTEXT("ExportUDS_Tooltip",
                    "Export selected Blueprint structs to a .ckexport file next to the asset"),
                FSlateIcon(),
                FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& /*InContext*/)
                {
                    DoExecuteExportAction_UserDefinedStruct();
                })
            );
        })
    );
}

} // namespace ck::asset_exporter

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
