#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/Public/CkSettings/UserSettings/CkUserSettings.h"

#include <CoreMinimal.h>

#include "CkSpatialQuery_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "SpatialQuery"))
class CKSPATIALQUERY_API UCk_SpatialQuery_UserSettings : public UCk_Plugin_UserSettings_UE
{
    GENERATED_BODY()

private:
    // Draw the debug information of all existing Probe Fragments
    // CVar: ck.SpatialQuery.PreviewAllProbes
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true, ConsoleVariable="ck.SpatialQuery.PreviewAllProbes"))
    bool _DebugPreviewAllProbes = false;

public:
    CK_PROPERTY_GET(_DebugPreviewAllProbes);
};

// --------------------------------------------------------------------------------------------------------------------

class CKSPATIALQUERY_API UCk_Utils_SpatialQuery_Settings
{
public:
    static auto Get_DebugPreviewAllProbes() -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
