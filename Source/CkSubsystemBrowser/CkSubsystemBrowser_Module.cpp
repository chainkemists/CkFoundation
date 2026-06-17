#include "CkSubsystemBrowser_Module.h"

#include "CkSubsystemBrowser/Tab/SCkSubsystemBrowserTab.h"

#include <Framework/Application/SlateApplication.h>
#include <Framework/Docking/TabManager.h>
#include <Widgets/Docking/SDockTab.h>
#include <WorkspaceMenuStructure.h>
#include <WorkspaceMenuStructureModule.h>

#define LOCTEXT_NAMESPACE "FCkSubsystemBrowserModule"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkSubsystemBrowserModule::
    StartupModule()
    -> void
{
    DoRegisterTabSpawner();
}

auto
    FCkSubsystemBrowserModule::
    ShutdownModule()
    -> void
{
    DoUnregisterTabSpawner();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkSubsystemBrowserModule::
    DoRegisterTabSpawner()
    -> void
{
    auto& GlobalTabManager = FGlobalTabmanager::Get();

    GlobalTabManager->RegisterNomadTabSpawner(
        SubsystemBrowser_TabName,
        FOnSpawnTab::CreateRaw(this, &FCkSubsystemBrowserModule::DoOnSpawnTab)
    )
    .SetDisplayName(FText::FromString(SubsystemBrowser_TabDisplayName))
    .SetTooltipText(FText::FromString(TEXT("Browse live Engine/Editor/GameInstance/World/LocalPlayer subsystems and invoke their CallInEditor functions")))
    .SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsDebugCategory());
}

auto
    FCkSubsystemBrowserModule::
    DoUnregisterTabSpawner()
    -> void
{
    if (FSlateApplication::IsInitialized())
    {
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SubsystemBrowser_TabName);
    }
}

auto
    FCkSubsystemBrowserModule::
    DoOnSpawnTab(
        const FSpawnTabArgs& Args)
    -> TSharedRef<SDockTab>
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        .Label(FText::FromString(SubsystemBrowser_TabDisplayName))
        .ToolTipText(FText::FromString(TEXT("Ck Subsystem Browser - inspect live subsystems and run their CallInEditor buttons")))
        [
            SNew(SCkSubsystemBrowserTab)
        ];
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkSubsystemBrowserModule, CkSubsystemBrowser)
