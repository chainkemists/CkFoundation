#pragma once

#include "CkAnimationToolbox_Style.h"

#include <CoreMinimal.h>
#include <Framework/Commands/Commands.h>

// --------------------------------------------------------------------------------------------------------------------

class CKANIMATIONEDITOR_API FCkAnimationToolboxCommands : public TCommands<FCkAnimationToolboxCommands>
{
public:
    FCkAnimationToolboxCommands()
        : TCommands<FCkAnimationToolboxCommands>(
            TEXT("CkAnimationToolbox"),
            NSLOCTEXT("Contexts", "CkAnimationToolbox", "CkAnimationToolbox Plugin"),
            NAME_None,
            FCkAnimationToolboxStyle::GetStyleSetName())
    {}

    auto RegisterCommands() -> void override;

public:
    TSharedPtr<FUICommandInfo> OpenAnimationToolbox;
};

// --------------------------------------------------------------------------------------------------------------------
