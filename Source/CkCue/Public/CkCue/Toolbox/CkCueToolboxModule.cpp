#include "CkCueToolboxModule.h"
#include "CkCueToolbox.h"
#include "CkCueToolboxStyle.h"
#include "CkCueToolboxCommands.h"

#include <Framework/Docking/LayoutExtender.h>
#include <WorkspaceMenuStructure.h>
#include <WorkspaceMenuStructureModule.h>
#include <ToolMenus.h>
#include <LevelEditor.h>

// --------------------------------------------------------------------------------------------------------------------

auto FCkCueToolboxModule::StartupModule() -> void
{
    FCkCueToolboxStyle::Initialize();
    FCkCueToolboxCommands::Register();
    DoRegisterTabSpawner();

    auto& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
    auto ToolbarExtender = MakeShareable(new FExtender);

    ToolbarExtender.Object->AddToolBarExtension(
        "Settings",
        EExtensionHook::After,
        nullptr,
        FToolBarExtensionDelegate::CreateLambda([](FToolBarBuilder& Builder)
        {
            Builder.BeginSection("CkCue");
            {
                Builder.AddToolBarButton(
                    FCkCueToolboxCommands::Get().OpenCueToolbox,
                    NAME_None,
                    FText::FromString(TEXT("Cue Toolbox")),
                    FText::FromString(TEXT("Open the Cue Toolbox")),
                    FSlateIcon("CkCueToolboxStyle", "CkCueToolbox.Icon")
                );
            }
            Builder.EndSection();
        })
    );

    LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
}

auto FCkCueToolboxModule::ShutdownModule() -> void
{
    DoUnregisterTabSpawner();
    FCkCueToolboxCommands::Unregister();
    FCkCueToolboxStyle::Shutdown();
}

auto FCkCueToolboxModule::DoRegisterTabSpawner() -> void
{
    auto& GlobalTabManager = FGlobalTabmanager::Get();

    GlobalTabManager->RegisterNomadTabSpawner(
        TabName,
        FOnSpawnTab::CreateRaw(this, &FCkCueToolboxModule::DoOnSpawnTab)
    )
    .SetDisplayName(FText::FromString(TabDisplayName))
    .SetTooltipText(FText::FromString(TEXT("Open the Cue Toolbox for discovering and managing gameplay cues")))
    .SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsDebugCategory())
    .SetIcon(FSlateIcon("CkCueToolboxStyle", "CkCueToolbox.Icon"));
}

auto FCkCueToolboxModule::DoUnregisterTabSpawner() -> void
{
    if (FSlateApplication::IsInitialized())
    {
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);
    }
}

auto FCkCueToolboxModule::DoOnSpawnTab(const FSpawnTabArgs& Args) -> TSharedRef<SDockTab>
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        .Label(FText::FromString(TabDisplayName))
        .ToolTipText(FText::FromString(TEXT("Cue Toolbox - Discover, validate, and execute gameplay cues")))
        [
            SNew(SCkCueToolbox)
        ];
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_MODULE(FCkCueToolboxModule, CkCueToolbox)