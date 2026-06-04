#pragma once

#include "CoreMinimal.h"

#include "ComponentVisualizer.h"

// --------------------------------------------------------------------------------------------------------------------

// Out-of-paint-mode preview for a grid spawner. Draws the SAME authored-state overlay the Grid Paint
// UEdMode draws (ck::grid_editor::Draw_GridAuthoredOverlay), so the selected-spawner preview matches the
// in-mode view exactly — previously the out-of-mode preview came from the runtime OBB DebugDraw path,
// which showed only green/red boxes (no blocker-orange, no tag-blue, no tags) and depth-shaded the green.
//
// Registered against USceneComponent (the spawner's root is a plain USceneComponent — the spawner is a
// generic entity host, not a grid-specific actor), so this fires for any selected actor's scene
// component and early-outs unless the owner is a grid spawner. Drawing is suppressed while the Grid
// Paint mode is active (the UEdMode draws the overlay itself then — this avoids doubled lines).
class FCk_2dGridSystem_SpawnerVisualizer : public FComponentVisualizer
{
public:
    virtual void DrawVisualization(
        const UActorComponent*   Component,
        const FSceneView*        View,
        FPrimitiveDrawInterface* PDI) override;

    virtual void DrawVisualizationHUD(
        const UActorComponent* Component,
        const FViewport*       Viewport,
        const FSceneView*      View,
        FCanvas*               Canvas) override;
};

// --------------------------------------------------------------------------------------------------------------------
