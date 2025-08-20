#pragma once

#include <Framework/Commands/Commands.h>
#include <EditorStyleSet.h>

// --------------------------------------------------------------------------------------------------------------------

class CKCUE_API FCkCueToolboxCommands : public TCommands<FCkCueToolboxCommands>
{
public:
    FCkCueToolboxCommands();

    auto RegisterCommands() -> void override;

public:
    TSharedPtr<FUICommandInfo> OpenCueToolbox;
    TSharedPtr<FUICommandInfo> RefreshCues;
    TSharedPtr<FUICommandInfo> CreateNewCue;
};

// --------------------------------------------------------------------------------------------------------------------