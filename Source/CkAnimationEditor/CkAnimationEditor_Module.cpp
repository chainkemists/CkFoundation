#include "CkAnimationEditor_Module.h"

#include "CkAnimationEditor/Toolbox/CkAnimationToolbox.h"
#include "CkAnimationEditor/Toolbox/CkAnimationToolbox_Commands.h"
#include "CkAnimationEditor/Toolbox/CkAnimationToolbox_Style.h"

#include <LevelEditor.h>
#include <WorkspaceMenuStructure.h>
#include <WorkspaceMenuStructureModule.h>

#define LOCTEXT_NAMESPACE "FCk_AnimationEditorModule"

auto
    FCk_AnimationEditorModule::
    StartupModule()
    -> void
{
    // The editor binary also boots as a game process (process-level tests, and any -game run
    // under a real RHI). There is no toolbar to extend there, and initializing the Slate style
    // creates RHI textures that the game-process teardown destroys without releasing them —
    // a fatal "FRenderResource was deleted without being released first". Under -nullrhi the
    // same textures never acquire GPU resources, which is why this only bites with rendering on.
    if (IsRunningGame())
    { return; }

    FCkAnimationToolboxStyle::Initialize();
    FCkAnimationToolboxCommands::Register();
    DoRegisterTabSpawner();

    auto& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
    auto ToolbarExtender = MakeShareable(new FExtender);

    ToolbarExtender.Object->AddToolBarExtension(
        "Settings",
        EExtensionHook::After,
        nullptr,
        FToolBarExtensionDelegate::CreateLambda([](FToolBarBuilder& Builder)
        {
            Builder.BeginSection("CkAnimation");
            {
                Builder.AddToolBarButton(
                    FCkAnimationToolboxCommands::Get().OpenAnimationToolbox,
                    NAME_None,
                    FText::FromString(TEXT("Anim Scrunch Fixer")),
                    FText::FromString(TEXT("Scan and fix the leading-frame scrunch in AnimSequences")),
                    FSlateIcon("CkAnimationToolboxStyle", "CkAnimationToolbox.Icon")
                );
            }
            Builder.EndSection();
        })
    );

    LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
}

auto
    FCk_AnimationEditorModule::
    ShutdownModule()
    -> void
{
    if (IsRunningGame())
    { return; }

    DoUnregisterTabSpawner();
    FCkAnimationToolboxCommands::Unregister();
    FCkAnimationToolboxStyle::Shutdown();
}

auto
    FCk_AnimationEditorModule::
    DoRegisterTabSpawner()
    -> void
{
    auto& GlobalTabManager = FGlobalTabmanager::Get();

    GlobalTabManager->RegisterNomadTabSpawner(
        TabName,
        FOnSpawnTab::CreateRaw(this, &FCk_AnimationEditorModule::DoOnSpawnTab)
    )
    .SetDisplayName(FText::FromString(TabDisplayName))
    .SetTooltipText(FText::FromString(TEXT("Find and repair AnimSequences whose leading frame collapses to identity local transforms")))
    .SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsDebugCategory())
    .SetIcon(FSlateIcon("CkAnimationToolboxStyle", "CkAnimationToolbox.Icon"));
}

auto
    FCk_AnimationEditorModule::
    DoUnregisterTabSpawner()
    -> void
{
    if (FSlateApplication::IsInitialized())
    {
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);
    }
}

auto
    FCk_AnimationEditorModule::
    DoOnSpawnTab(const FSpawnTabArgs& Args)
    -> TSharedRef<SDockTab>
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        .Label(FText::FromString(TabDisplayName))
        .ToolTipText(FText::FromString(TEXT("Anim Scrunch Fixer - detects and repairs leading-frame collapse in AnimSequences")))
        [
            SNew(SCkAnimationToolbox)
        ];
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCk_AnimationEditorModule, CkAnimationEditor)
