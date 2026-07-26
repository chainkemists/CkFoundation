#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"

// --------------------------------------------------------------------------------------------------------------------

// World/GameInstance/LocalPlayer only have live instances while a world is active (typically PIE).
enum class ECk_SubsystemCategory : uint8
{
    Engine,
    Editor,
    GameInstance,
    World,
    LocalPlayer
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::subsystem_browser
{
    // Categories in display order.
    CKSUBSYSTEMBROWSER_API auto
    Get_AllCategories() -> TArray<ECk_SubsystemCategory>;

    CKSUBSYSTEMBROWSER_API auto
    Get_CategoryDisplayName(
        ECk_SubsystemCategory InCategory) -> FString;

    // Weak because World/GameInstance/LocalPlayer subsystems die on PIE teardown. Empty when no
    // owning object exists (e.g. World outside PIE) or GEditor/GEngine is null.
    CKSUBSYSTEMBROWSER_API auto
    Collect_Subsystems(
        ECk_SubsystemCategory InCategory) -> TArray<TWeakObjectPtr<UObject>>;
}

// --------------------------------------------------------------------------------------------------------------------
