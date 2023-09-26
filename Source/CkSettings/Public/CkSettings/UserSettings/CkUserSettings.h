#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Engine/DeveloperSettingsBackedByCVars.h>

#include "CkUserSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(config = EditorPerProjectUserSettings)
class CKSETTINGS_API UCk_EditorPerProject_UserSettings_UE : public UDeveloperSettingsBackedByCVars
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EditorPerProject_UserSettings_UE);

public:
    explicit UCk_EditorPerProject_UserSettings_UE(const FObjectInitializer& ObjectInitializer);
};

// --------------------------------------------------------------------------------------------------------------------
