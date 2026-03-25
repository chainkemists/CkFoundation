#include "CkInsightsAnalyzer_Module.h"

#include "CkInsightsAnalyzer/Tab/SCkInsightsAnalyzerTab.h"

#include <WorkspaceMenuStructure.h>
#include <WorkspaceMenuStructureModule.h>

#define LOCTEXT_NAMESPACE "FCkInsightsAnalyzerModule"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkInsightsAnalyzerModule::
    StartupModule()
    -> void
{
    DoRegisterTabSpawner();
}

auto
    FCkInsightsAnalyzerModule::
    ShutdownModule()
    -> void
{
    DoUnregisterTabSpawner();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkInsightsAnalyzerModule::
    DoRegisterTabSpawner()
    -> void
{
    auto& GlobalTabManager = FGlobalTabmanager::Get();

    GlobalTabManager->RegisterNomadTabSpawner(
        AnalyzerTab_TabName,
        FOnSpawnTab::CreateRaw(this, &FCkInsightsAnalyzerModule::DoOnSpawnTab)
    )
    .SetDisplayName(FText::FromString(AnalyzerTab_TabDisplayName))
    .SetTooltipText(FText::FromString(TEXT("Open .utrace files and analyze frame performance")))
    .SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsDebugCategory());
}

auto
    FCkInsightsAnalyzerModule::
    DoUnregisterTabSpawner()
    -> void
{
    if (FSlateApplication::IsInitialized())
    {
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AnalyzerTab_TabName);
    }
}

auto
    FCkInsightsAnalyzerModule::
    DoOnSpawnTab(const FSpawnTabArgs& Args)
    -> TSharedRef<SDockTab>
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        .Label(FText::FromString(AnalyzerTab_TabDisplayName))
        .ToolTipText(FText::FromString(TEXT("Analyze Unreal Insights .utrace files")))
        [
            SNew(SCkInsightsAnalyzerTab)
        ];
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkInsightsAnalyzerModule, CkInsightsAnalyzer)
