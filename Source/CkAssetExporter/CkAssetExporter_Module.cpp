#include "CkAssetExporter_Module.h"

#include "CkAssetExporter/AssetAction/CkBehaviorTreeExporter_AssetAction.h"
#include "CkAssetExporter/AutoSidecar/CkAssetExporter_OnSaveHook.h"
#include "CkAssetExporter/AssetAction/CkBlueprintExporter_AssetAction.h"
#include "CkAssetExporter/AssetAction/CkCascadeExporter_AssetAction.h"
#include "CkAssetExporter/AssetAction/CkDataAssetExporter_AssetAction.h"
#include "CkAssetExporter/AssetAction/CkEQSExporter_AssetAction.h"
#include "CkAssetExporter/AssetAction/CkMaterialExporter_AssetAction.h"
#include "CkAssetExporter/AssetAction/CkNiagaraExporter_AssetAction.h"
#include "CkAssetExporter/AssetAction/CkStateTreeExporter_AssetAction.h"
#include "CkAssetExporter/AssetAction/CkUserDefinedEnumExporter_AssetAction.h"
#include "CkAssetExporter/AssetAction/CkUserDefinedStructExporter_AssetAction.h"
#include "CkAssetExporter/ExporterTab/SCkAssetExporterTab.h"
#include "CkAssetExporter/VfxCorpusExporter/CkVfxCorpusExporter.h"

#include <ToolMenus.h>
#include <WorkspaceMenuStructure.h>
#include <WorkspaceMenuStructureModule.h>

#define LOCTEXT_NAMESPACE "FCkAssetExporterModule"

// --------------------------------------------------------------------------------------------------------------------
// Editor-console entry point for the corpus batch export (the automation test Ck.AssetExporter.ExportVfxCorpus is
// the headless equivalent).

namespace ck::asset_exporter
{
    static FAutoConsoleCommand ExportVfxCorpus_ConsoleCommand{
        TEXT("Ck.AssetExporter.ExportVfxCorpus"),
        TEXT("Exports every Niagara + Cascade particle system under /Game (recipes, materials, textures) into Saved/CkVfxCorpus"),
        FConsoleCommandDelegate::CreateLambda([]() -> void
        {
            FCk_VfxCorpusExporter::ExportCorpus({ TEXT("/Game") }, FCk_VfxCorpusExporter::Get_DefaultCorpusRoot());
        })};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAssetExporterModule::
    StartupModule()
    -> void
{
    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FCkAssetExporterModule::DoRegisterContentBrowserExtension));

    DoRegisterTabSpawner();

    FCk_AssetExporter_OnSaveHook::Register();
}

auto
    FCkAssetExporterModule::
    ShutdownModule()
    -> void
{
    FCk_AssetExporter_OnSaveHook::Unregister();

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
    ck::asset_exporter::RegisterCascadeContextMenu();
    ck::asset_exporter::RegisterDataAssetContextMenu();
    ck::asset_exporter::RegisterEQSContextMenu();
    ck::asset_exporter::RegisterMaterialContextMenu();
    ck::asset_exporter::RegisterNiagaraContextMenu();
    ck::asset_exporter::RegisterStateTreeContextMenu();
    ck::asset_exporter::RegisterUserDefinedEnumContextMenu();
    ck::asset_exporter::RegisterUserDefinedStructContextMenu();
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
    .SetTooltipText(FText::FromString(TEXT("Export assets (Behavior Trees, Blueprints, DataAssets, EQS, State Trees, Structs, Enums, Materials) to JSON and plain-text formats")))
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
            SNew(SCkAssetExporterTab)
        ];
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkAssetExporterModule, CkAssetExporter)
