#pragma once

#include <ComponentVisualizer.h>

// ----------------------------------------------------------------------------------------------------------------

class FCk_ActorComponent_Visualizer : public FComponentVisualizer
{
private:
    auto
    DrawVisualization(
        const UActorComponent* InComponent,
        const FSceneView* InView,
        FPrimitiveDrawInterface* PDI) -> void override;
};

// ----------------------------------------------------------------------------------------------------------------
