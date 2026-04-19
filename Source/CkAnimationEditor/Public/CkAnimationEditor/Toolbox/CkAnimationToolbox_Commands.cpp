#include "CkAnimationToolbox_Commands.h"

#include "CkAnimationEditor_Module.h"

#include <Framework/Docking/TabManager.h>

#define LOCTEXT_NAMESPACE "FCkAnimationToolboxCommands"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAnimationToolboxCommands::
    RegisterCommands()
    -> void
{
    UI_COMMAND(OpenAnimationToolbox,
        "Anim Scrunch Fixer",
        "Open the Animation Scrunch Fixer tool",
        EUserInterfaceActionType::Button,
        FInputChord());

    FGlobalTabmanager::Get()->RegisterDefaultTabWindowSize(FCk_AnimationEditorModule::TabName, FVector2D(1100.0f, 720.0f));
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
