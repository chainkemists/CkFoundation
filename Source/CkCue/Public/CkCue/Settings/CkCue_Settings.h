#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkCue_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Cue"))
class CKCUE_API UCk_Cue_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Cue_ProjectSettings_UE);

private:
    UPROPERTY(Config, EditDefaultsOnly, Category = "Execution",
        meta = (AllowPrivateAccess = true,
            ToolTip = "How long to wait (in seconds) before warning about pending cues that couldn't execute due to missing executors"))
    float _PendingCueTimeoutSeconds = 10.0f;

public:
    CK_PROPERTY_GET(_PendingCueTimeoutSeconds);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCUE_API UCk_Utils_Cue_Settings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Cue_Settings_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Cue|Settings")
    static float
    Get_PendingCueTimeoutSeconds();
};

// --------------------------------------------------------------------------------------------------------------------
