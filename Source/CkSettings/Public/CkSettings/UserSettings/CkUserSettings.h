#pragma once

#include <Engine/DeveloperSettingsBackedByCVars.h>

#include "CkUserSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Config = EditorPerProjectUserSettings)
class CKSETTINGS_API UCk_EditorPerProject_UserSettings_UE : public UDeveloperSettingsBackedByCVars
{
    GENERATED_BODY()

public:
    explicit UCk_EditorPerProject_UserSettings_UE(const FObjectInitializer& ObjectInitializer);
};

// --------------------------------------------------------------------------------------------------------------------
