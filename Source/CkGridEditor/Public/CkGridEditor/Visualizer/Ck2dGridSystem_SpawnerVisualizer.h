#pragma once

#include "CoreMinimal.h"

#include "ComponentVisualizer.h"

// --------------------------------------------------------------------------------------------------------------------

// Out-of-paint-mode preview for a grid spawner, drawing the SAME authored-state overlay the Grid Paint
// UEdMode draws. Registered against USceneComponent (the spawner's root is a plain one — it is a generic
// entity host), so it fires for ANY selected actor and must early-out unless the owner is a grid spawner.
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
