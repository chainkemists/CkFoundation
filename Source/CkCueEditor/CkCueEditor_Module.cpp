#include "CkCueEditor_Module.h"

#include "CkCueEditor/Toolbox/CkCueToolbox.h"
#include "CkCueEditor/Toolbox/CkCueToolboxCommands.h"
#include "CkCueEditor/Toolbox/CkCueToolboxStyle.h"

#include <LevelEditor.h>
#include <WorkspaceMenuStructure.h>
#include <WorkspaceMenuStructureModule.h>

#define LOCTEXT_NAMESPACE "FCkCueEditorModule"

auto
    FCk_CueEditorModule::
    StartupModule()
    -> void
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

auto
    FCk_CueEditorModule::
    ShutdownModule()
    -> void
{
    DoUnregisterTabSpawner();
    FCkCueToolboxCommands::Unregister();
    FCkCueToolboxStyle::Shutdown();
}

auto
    FCk_CueEditorModule::
    DoRegisterTabSpawner()
    -> void
{
    auto& GlobalTabManager = FGlobalTabmanager::Get();

    GlobalTabManager->RegisterNomadTabSpawner(
        TabName,
        FOnSpawnTab::CreateRaw(this, &FCk_CueEditorModule::DoOnSpawnTab)
    )
    .SetDisplayName(FText::FromString(TabDisplayName))
    .SetTooltipText(FText::FromString(TEXT("Open the Cue Toolbox for discovering and managing gameplay cues")))
    .SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsDebugCategory())
    .SetIcon(FSlateIcon("CkCueToolboxStyle", "CkCueToolbox.Icon"));
}

auto
    FCk_CueEditorModule::
    DoUnregisterTabSpawner()
    -> void
{
    if (FSlateApplication::IsInitialized())
    {
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);
    }
}

auto
    FCk_CueEditorModule::
    DoOnSpawnTab(const FSpawnTabArgs& Args)
    -> TSharedRef<SDockTab>
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

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCk_CueEditorModule, CkCueEditor)
