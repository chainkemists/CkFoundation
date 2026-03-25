#include "CkAssetExporter_Module.h"

#include "CkAssetExporter/AssetAction/CkBehaviorTreeExporter_AssetAction.h"
#include "CkAssetExporter/AssetAction/CkBlueprintExporter_AssetAction.h"
#include "CkAssetExporter/AssetAction/CkDataAssetExporter_AssetAction.h"
#include "CkAssetExporter/AssetAction/CkEQSExporter_AssetAction.h"
#include "CkAssetExporter/ExporterTab/SCkBehaviorTreeExporterTab.h"

#include <ToolMenus.h>
#include <WorkspaceMenuStructure.h>
#include <WorkspaceMenuStructureModule.h>

#define LOCTEXT_NAMESPACE "FCkAssetExporterModule"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAssetExporterModule::
    StartupModule()
    -> void
{
    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FCkAssetExporterModule::DoRegisterContentBrowserExtension));

    DoRegisterTabSpawner();
}

auto
    FCkAssetExporterModule::
    ShutdownModule()
    -> void
{
    DoUnregisterTabSpawner();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAssetExporterModule::
    DoRegisterContentBrowserExtension()
    -> void
{
    ck::asset_exporter::RegisterBehaviorTreeContextMenu();
    ck::asset_exporter::RegisterBlueprintContextMenu();
    ck::asset_exporter::RegisterDataAssetContextMenu();
    ck::asset_exporter::RegisterEQSContextMenu();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAssetExporterModule::
    DoRegisterTabSpawner()
    -> void
{
    auto& GlobalTabManager = FGlobalTabmanager::Get();

    GlobalTabManager->RegisterNomadTabSpawner(
        ExporterTab_TabName,
        FOnSpawnTab::CreateRaw(this, &FCkAssetExporterModule::DoOnSpawnTab)
    )
    .SetDisplayName(FText::FromString(ExporterTab_TabDisplayName))
    .SetTooltipText(FText::FromString(TEXT("Export assets (Behavior Trees, Blueprints, DataAssets) to JSON and plain-text formats")))
    .SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsDebugCategory());
}

auto
    FCkAssetExporterModule::
    DoUnregisterTabSpawner()
    -> void
{
    if (FSlateApplication::IsInitialized())
    {
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ExporterTab_TabName);
    }
}

auto
    FCkAssetExporterModule::
    DoOnSpawnTab(const FSpawnTabArgs& Args)
    -> TSharedRef<SDockTab>
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        .Label(FText::FromString(ExporterTab_TabDisplayName))
        .ToolTipText(FText::FromString(TEXT("Export assets to JSON and plain-text formats")))
        [
            SNew(SCkBehaviorTreeExporterTab)
        ];
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkAssetExporterModule, CkAssetExporter)
