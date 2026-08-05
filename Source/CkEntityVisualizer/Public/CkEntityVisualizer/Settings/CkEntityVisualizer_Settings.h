#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/UserSettings/CkUserSettings.h"

#include "CkEntityVisualizer_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM()
enum class ECk_EntityVisualizer_VisibilityMode : uint8
{
    Disabled UMETA(DisplayName = "Disabled"),
    SelectedOnly UMETA(DisplayName = "Selected Only"),
    All UMETA(DisplayName = "All"),
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::entity_visualizer
{
    inline auto
        ShouldVisualize(
            ECk_EntityVisualizer_VisibilityMode InMode,
            bool InHasSelectionOwner,
            bool InSelectionOwnerIsSelected)
        -> bool
    {
        switch (InMode)
        {
            case ECk_EntityVisualizer_VisibilityMode::SelectedOnly:
                return InHasSelectionOwner && InSelectionOwnerIsSelected;
            case ECk_EntityVisualizer_VisibilityMode::All:
                return true;
            case ECk_EntityVisualizer_VisibilityMode::Disabled:
            default:
                return false;
        }
    }

    CKENTITYVISUALIZER_API auto
    GetVisibilityMode() -> ECk_EntityVisualizer_VisibilityMode;
}

// --------------------------------------------------------------------------------------------------------------------

// Per-user non-PIE visibility policy shared by retained probe previews and entity transform gizmos.
// Editor Preferences edits and console changes are synchronized in both directions.
UCLASS(meta = (DisplayName = "Entity Visualizer"))
class CKENTITYVISUALIZER_API UCk_EntityVisualizer_UserSettings_UE : public UCk_Plugin_UserSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityVisualizer_UserSettings_UE);

    auto PostInitProperties() -> void override;

#if WITH_EDITOR
    auto PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent) -> void override;
#endif

private:
    UPROPERTY(Config, EditAnywhere, Category = "Editor Visualization",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Controls retained probe previews and entity transform gizmos outside PIE. Disabled creates no visuals. Selected Only shows visuals owned by selected actors. All shows every entity, including ownerless entities."))
    ECk_EntityVisualizer_VisibilityMode _VisibilityMode =
        ECk_EntityVisualizer_VisibilityMode::SelectedOnly;

public:
    CK_PROPERTY(_VisibilityMode);
};

// --------------------------------------------------------------------------------------------------------------------
