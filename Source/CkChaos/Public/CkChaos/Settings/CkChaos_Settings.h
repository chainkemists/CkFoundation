#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"
#include "CkSettings/UserSettings/CkUserSettings.h"

#include "CkChaos_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Chaos"))
class CKCHAOS_API UCk_Chaos_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Chaos_ProjectSettings_UE);

private:
    // each frame, the radial damage radius is increased by this amount until we reach the radial damage radius
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Recycling",
              meta = (AllowPrivateAccess = true, ValidEnumValues="Recycle"))
    float _MaximumRadialDamageDeltaRadiusPerFrame = 100.0f;

public:
    CK_PROPERTY_GET(_MaximumRadialDamageDeltaRadiusPerFrame);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Chaos"))
class CKCHAOS_API UCk_Chaos_UserSettings_UE : public UCk_Plugin_UserSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Chaos_UserSettings_UE);

private:

public:
};

// --------------------------------------------------------------------------------------------------------------------

class CKCHAOS_API UCk_Utils_Chaos_Settings_UE
{
public:
    static auto
    Get_MaximumRadialDamageDeltaRadiusPerFrame() -> float;
};

// --------------------------------------------------------------------------------------------------------------------
