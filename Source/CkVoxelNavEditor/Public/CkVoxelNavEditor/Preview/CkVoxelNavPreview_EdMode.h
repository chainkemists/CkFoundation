#pragma once

#include "CoreMinimal.h"

#include "Tools/LegacyEdModeWidgetHelpers.h"

#include "CkVoxelNavPreview_EdMode.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class FPrimitiveDrawInterface;
class FSceneView;
class FViewport;
class FCanvas;
class FEditorViewportClient;

// --------------------------------------------------------------------------------------------------------------------

/** Read-only Level Editor overlay for exact cooked-Jolt VoxelNav preview snapshots. The mode is intentionally
 *  hidden from the standard mode palette: the Crowd Debugger owns its toggle and layer controls. */
UCLASS()
class CKVOXELNAVEDITOR_API UCk_VoxelNavPreview_EdMode : public UBaseLegacyWidgetEdMode
{
    GENERATED_BODY()

public:
    static const FEditorModeID EM_CkVoxelNavPreviewModeId;

    UCk_VoxelNavPreview_EdMode();

    auto Enter() -> void override;

    auto
    Render(
        const FSceneView* InView,
        FViewport* InViewport,
        FPrimitiveDrawInterface* InPdi) -> void override;

    auto
    DrawHUD(
        FEditorViewportClient* InViewportClient,
        FViewport* InViewport,
        const FSceneView* InView,
        FCanvas* InCanvas) -> void override;

public:
    static auto Set_LevelOverlayEnabled(bool InEnabled) -> bool;
    static auto Get_IsLevelOverlayEnabled() -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
