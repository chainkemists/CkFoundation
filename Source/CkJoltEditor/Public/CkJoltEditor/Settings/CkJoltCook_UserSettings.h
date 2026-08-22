#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkSettings/UserSettings/CkUserSettings.h"

#include "CkJoltCook_UserSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/// Keeps cooked Jolt data from going stale behind a designer's back. Per-user because the cost
/// lands on whoever is editing; which MAPS are eligible is shared and lives in the project settings
/// (Jolt -> Static World -> Cook Excluded Map Path Prefixes).
UCLASS(meta = (DisplayName = "Jolt Cook"))
class CKJOLTEDITOR_API UCk_JoltCook_UserSettings_UE : public UCk_Plugin_UserSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_JoltCook_UserSettings_UE);

private:
    // Re-cook a static mesh's pre-baked Jolt shape when the mesh asset is saved. Scoped to meshes
    // under the project's BakedMeshShapeRoots, and costs one shape build for the one mesh saved.
    UPROPERTY(EditAnywhere, Config, Category = "Auto Cook",
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _AutoCookMeshShapeOnAssetSave = ECk_EnableDisable::Enable;

    // Re-cook the open map's static world when a level is saved. Incremental — only the bake-grid
    // cells whose actors actually changed are rewritten. Saving a STREAMING SUBLEVEL cooks the
    // persistent map it belongs to, because that is the map the runtime resolves cooked data by.
    UPROPERTY(EditAnywhere, Config, Category = "Auto Cook",
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _AutoCookStaticWorldOnLevelSave = ECk_EnableDisable::Enable;

    // How long an auto-cook waits after the triggering save, so a burst (Save All, a multi-sublevel
    // save) coalesces into one cook.
    UPROPERTY(EditAnywhere, Config, Category = "Auto Cook",
              meta = (AllowPrivateAccess = true))
    FCk_Time _AutoCookDebounce = FCk_Time{2.0};

public:
    CK_PROPERTY_GET(_AutoCookMeshShapeOnAssetSave);
    CK_PROPERTY_GET(_AutoCookStaticWorldOnLevelSave);
    CK_PROPERTY_GET(_AutoCookDebounce);
};

// --------------------------------------------------------------------------------------------------------------------

class CKJOLTEDITOR_API UCk_Utils_JoltCook_UserSettings
{
public:
    static auto Get_AutoCookMeshShapeOnAssetSave() -> ECk_EnableDisable;
    static auto Get_AutoCookStaticWorldOnLevelSave() -> ECk_EnableDisable;
    static auto Get_AutoCookDebounce() -> FCk_Time;
};

// --------------------------------------------------------------------------------------------------------------------
