#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/UserSettings/CkUserSettings.h"

#include "CkIskmRenderer_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM()
enum class ECk_Iskm_EditorPreviewAnimationMode : uint8
{
    Disabled UMETA(DisplayName = "Disabled"),
    SelectedOnly UMETA(DisplayName = "Selected Only"),
    All UMETA(DisplayName = "All"),
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::iskm_editor_preview
{
    inline auto
        IsEnabled(ECk_Iskm_EditorPreviewAnimationMode InMode)
        -> bool
    {
        return InMode == ECk_Iskm_EditorPreviewAnimationMode::SelectedOnly ||
            InMode == ECk_Iskm_EditorPreviewAnimationMode::All;
    }

    inline auto
        ShouldAnimate(
            ECk_Iskm_EditorPreviewAnimationMode InMode,
            bool InSelectionOwnerIsSelected)
        -> bool
    {
        switch (InMode)
        {
            case ECk_Iskm_EditorPreviewAnimationMode::SelectedOnly:
                return InSelectionOwnerIsSelected;
            case ECk_Iskm_EditorPreviewAnimationMode::All:
                return true;
            case ECk_Iskm_EditorPreviewAnimationMode::Disabled:
            default:
                return false;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

// Per-user non-PIE preview policy. The properties and CVars are kept bidirectionally synchronized:
// Editor Preferences edits apply immediately, and console changes persist to EditorPerProjectUserSettings.ini.
UCLASS(meta = (DisplayName = "ISKM Renderer"))
class CKISKMRENDERER_API UCk_IskmRenderer_UserSettings_UE : public UCk_Plugin_UserSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_IskmRenderer_UserSettings_UE);

    auto PostInitProperties() -> void override;

#if WITH_EDITOR
    auto PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent) -> void override;
#endif

private:
    UPROPERTY(Config, EditAnywhere, Category = "Editor Preview Animation",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Controls continuous batched ISKM animation outside PIE. Disabled performs no recurring preview work. Selected Only advances previews owned by selected actors. All advances every preview crowd."))
    ECk_Iskm_EditorPreviewAnimationMode _EditorPreviewAnimationMode = ECk_Iskm_EditorPreviewAnimationMode::SelectedOnly;

    UPROPERTY(Config, EditAnywhere, Category = "Editor Preview Animation",
        meta = (AllowPrivateAccess = true, ClampMin = "5", ClampMax = "60", UIMin = "5", UIMax = "60",
            ToolTip = "Maximum non-PIE preview update frequency. Playback time remains correct while render-data pushes are bounded to this cadence."))
    int32 _EditorPreviewAnimationFrequency = 30;

public:
    CK_PROPERTY(_EditorPreviewAnimationMode);
    CK_PROPERTY(_EditorPreviewAnimationFrequency);
};

// --------------------------------------------------------------------------------------------------------------------
