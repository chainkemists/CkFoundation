#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/UserSettings/CkUserSettings.h"

#include "CkEntitySpawner_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Entity Spawner"))
class CKENTITYSPAWNER_API UCk_EntitySpawner_UserSettings_UE : public UCk_Plugin_UserSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntitySpawner_UserSettings_UE);

private:
    // Multiplier applied to the editor billboard's ScreenSize. 1.0 = default UE billboard size.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly,
              Category = "Editor Billboard",
              meta = (AllowPrivateAccess = true, ClampMin = "0.05", ClampMax = "4.0", UIMin = "0.05", UIMax = "4.0"))
    float _IconScale = 0.1f;

    // World-space vertical (Z) offset applied to the editor billboard so it lifts off the ground.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly,
              Category = "Editor Billboard",
              meta = (AllowPrivateAccess = true))
    float _IconVerticalOffset = 32.0f;

public:
    CK_PROPERTY_GET(_IconScale);
    CK_PROPERTY_GET(_IconVerticalOffset);
};

// --------------------------------------------------------------------------------------------------------------------
