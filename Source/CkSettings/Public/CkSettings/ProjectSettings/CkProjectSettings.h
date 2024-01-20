#pragma once

#include <Engine/DeveloperSettingsBackedByCVars.h>

#include "CkProjectSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, DefaultConfig, Config = CkFoundation)
class CKSETTINGS_API UCk_Plugin_ProjectSettings_UE : public UDeveloperSettingsBackedByCVars
{
    GENERATED_BODY()

public:
    explicit UCk_Plugin_ProjectSettings_UE(
        const FObjectInitializer& InObjectInitializer);
};

// --------------------------------------------------------------------------------------------------------------------
