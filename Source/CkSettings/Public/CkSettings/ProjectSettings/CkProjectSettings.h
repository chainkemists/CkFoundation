#pragma once

#include <Engine/DeveloperSettingsBackedByCVars.h>

#include "CkProjectSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, DefaultConfig, Config = Engine)
class CKSETTINGS_API UCk_Engine_ProjectSettings_UE : public UDeveloperSettingsBackedByCVars
{
    GENERATED_BODY()

public:
    explicit UCk_Engine_ProjectSettings_UE(const FObjectInitializer& ObjectInitializer);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, DefaultConfig, Config = Editor)
class CKSETTINGS_API UCk_Editor_ProjectSettings_UE : public UDeveloperSettingsBackedByCVars
{
    GENERATED_BODY()

public:
    explicit UCk_Editor_ProjectSettings_UE(const FObjectInitializer& ObjectInitializer);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, DefaultConfig, Config = Game)
class CKSETTINGS_API UCk_Game_ProjectSettings_UE : public UDeveloperSettingsBackedByCVars
{
    GENERATED_BODY()

public:
    explicit UCk_Game_ProjectSettings_UE(const FObjectInitializer& ObjectInitializer);
};

// --------------------------------------------------------------------------------------------------------------------
