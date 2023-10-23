#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Engine/DeveloperSettingsBackedByCVars.h>

#include "CkProjectSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Defaultconfig, Config = Engine)
class CKSETTINGS_API UCk_Engine_ProjectSettings_UE : public UDeveloperSettingsBackedByCVars
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Engine_ProjectSettings_UE);

public:
    explicit UCk_Engine_ProjectSettings_UE(const FObjectInitializer& ObjectInitializer);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Defaultconfig, Config = Game)
class CKSETTINGS_API UCk_Game_ProjectSettings_UE : public UDeveloperSettingsBackedByCVars
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Game_ProjectSettings_UE);

public:
    explicit UCk_Game_ProjectSettings_UE(const FObjectInitializer& ObjectInitializer);
};

// --------------------------------------------------------------------------------------------------------------------
