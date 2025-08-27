#include "CkCueToolboxCommands.h"

#include <Framework/Application/SlateApplication.h>

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FCkCueToolboxCommands"

// --------------------------------------------------------------------------------------------------------------------

FCkCueToolboxCommands::FCkCueToolboxCommands()
    : TCommands<FCkCueToolboxCommands>(
        TEXT("CkCueToolbox"),
        NSLOCTEXT("Contexts", "CkCueToolbox", "Cue Toolbox"),
        NAME_None,
        "CkCueToolboxStyle"
    )
{
}

auto FCkCueToolboxCommands::RegisterCommands() -> void
{
    UI_COMMAND(OpenCueToolbox, "Cue Toolbox", "Open the Cue Toolbox", EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(RefreshCues, "Refresh Cues", "Refresh the discovered cues", EUserInterfaceActionType::Button, FInputChord(EKeys::F5));
    UI_COMMAND(CreateNewCue, "Create Cue", "Create a new cue class", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::N));
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE