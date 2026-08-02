#include "CkUserDefinedEnumExporter_AssetAction.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/UserDefinedEnumExporter/CkUserDefinedEnumExporter.h"

#include <Engine/UserDefinedEnum.h>
#include <ContentBrowserModule.h>
#include <IContentBrowserSingleton.h>
#include <ToolMenus.h>
#include <ToolMenuContext.h>

#define LOCTEXT_NAMESPACE "CkAssetExporter"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::asset_exporter
{

static auto
    DoGetSelectedUserDefinedEnums()
    -> TArray<UUserDefinedEnum*>
{
    auto Enums = TArray<UUserDefinedEnum*>{};

    auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    auto SelectedAssets = TArray<FAssetData>{};
    ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

    for (const auto& AssetData : SelectedAssets)
    {
        if (auto* Enum = Cast<UUserDefinedEnum>(AssetData.GetAsset()))
        {
            Enums.Add(Enum);
        }
    }

    return Enums;
}

static auto
    DoShowExportNotification(
        const TArray<FCk_UserDefinedEnumExportResult>& InResults)
    -> void
{
    auto SuccessCount = int32{0};
    auto FailCount = int32{0};

    for (const auto& Result : InResults)
    {
        if (Result.Succeeded)
        {
            ++SuccessCount;
            UE_LOG(CkAssetExporter, Log, TEXT("Exported UserDefinedEnum '%s': %s, %s"),
                *Result.AssetName, *Result.JsonFilePath, *Result.TextFilePath);
        }
        else
        {
            ++FailCount;
            UE_LOG(CkAssetExporter, Error, TEXT("Failed to export UserDefinedEnum '%s': %s"),
                *Result.AssetName, *Result.ErrorMessage);
        }
    }

    if (FailCount == 0)
    {
        UE_LOG(CkAssetExporter, Log, TEXT("Successfully exported %d UserDefinedEnum(s)"), SuccessCount);
    }
    else
    {
        UE_LOG(CkAssetExporter, Warning, TEXT("Exported %d UserDefinedEnum(s), %d failed"), SuccessCount, FailCount);
    }
}

static auto
    DoExecuteExportAction_UserDefinedEnum()
    -> void
{
    const auto Enums = DoGetSelectedUserDefinedEnums();

    if (Enums.Num() == 0)
    {
        UE_LOG(CkAssetExporter, Warning, TEXT("No UserDefinedEnum assets selected"));
        return;
    }

    const auto Results = FCk_UserDefinedEnumExporter::ExportUserDefinedEnums(Enums);
    DoShowExportNotification(Results);
}

auto
    RegisterUserDefinedEnumContextMenu()
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

    Section.AddDynamicEntry("ExportUserDefinedEnum",
        FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
        {
            const auto Enums = DoGetSelectedUserDefinedEnums();

            if (Enums.Num() == 0)
            { return; }

            InSection.AddMenuEntry(
                "ExportUDE_JsonAndText",
                LOCTEXT("ExportUDE_Label", "Export Enum to JSON"),
                LOCTEXT("ExportUDE_Tooltip",
                    "Export selected Blueprint enums to a .ckexport file next to the asset"),
                FSlateIcon(),
                FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& /*InContext*/)
                {
                    DoExecuteExportAction_UserDefinedEnum();
                })
            );
        })
    );
}

} // namespace ck::asset_exporter

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
