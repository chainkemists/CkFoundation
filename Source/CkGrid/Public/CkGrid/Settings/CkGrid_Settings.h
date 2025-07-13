#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/Public/CkSettings/UserSettings/CkUserSettings.h"

#include <CoreMinimal.h>

#include "CkGrid_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Grid"))
class CKGRID_API UCk_Grid_UserSettings : public UCk_Plugin_UserSettings_UE
{
    GENERATED_BODY()

private:
    // Draw the debug information of all existing 2D Grid Systems
    // CVar: ck.Grid.PreviewAllGrids
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true, ConsoleVariable="ck.Grid.PreviewAllGrids"))
    bool _DebugPreviewAllGrids = false;

public:
    CK_PROPERTY_GET(_DebugPreviewAllGrids);
};

// --------------------------------------------------------------------------------------------------------------------

class CKGRID_API UCk_Utils_Grid_Settings
{
public:
    static auto
    Get_DebugPreviewAllGrids() -> bool;
};

// --------------------------------------------------------------------------------------------------------------------