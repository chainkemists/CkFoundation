#include "CkGridEditor/Visualizer/Ck2dGridSystem_SpawnerVisualizer.h"

#include "CkGridEditor/Draw/Ck2dGridSystem_AuthoredOverlay.h"
#include "CkGridEditor/EdMode/Ck2dGridSystem_EdMode.h"

#include "CkEntitySpawner/CkEntitySpawner_Actor.h"

#include "Components/SceneComponent.h"
#include "Editor.h"
#include "EditorModeManager.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_grid_spawner_visualizer
{
    // Shared early-out for both draw entry points: resolves the grid spawner that owns InComponent (and
    // its Spec), or returns nullptr when this isn't a grid-spawner root we should draw for. Guards:
    //   - owner must be an ACk_EntitySpawner_UE,
    //   - InComponent must be the spawner's ROOT (so we draw once per spawner, not per scene component),
    //   - the Grid Paint UEdMode must NOT be active (the UEdMode draws the overlay itself then),
    //   - the spawner must host a grid script with a Spec assigned.
    auto
        Resolve_DrawableSpec(
            const UActorComponent* InComponent,
            const ACk_EntitySpawner_UE*& OutSpawner) -> const UCk_2dGridSystem_Spec*
    {
        OutSpawner = nullptr;

        if (InComponent == nullptr)
        { return nullptr; }

        const auto* Spawner = Cast<ACk_EntitySpawner_UE>(InComponent->GetOwner());
        if (Spawner == nullptr)
        { return nullptr; }

        // Draw once on the root component only — the visualizer is registered on the broad USceneComponent
        // class, so without this guard a spawner with multiple scene components would draw N times.
        if (Spawner->GetRootComponent() != InComponent)
        { return nullptr; }

        // While the Grid Paint mode is active it renders the authored overlay itself (plus interaction
        // highlights); skip here so the two don't double up.
        if (GLevelEditorModeTools().IsModeActive(UCk_2dGridSystem_EdMode::EM_Ck2dGridSystemPaintModeId))
        { return nullptr; }

        const auto* Spec = ck::grid_editor::Resolve_SpecFromSpawner(Spawner);
        if (Spec == nullptr)
        { return nullptr; }

        OutSpawner = Spawner;
        return Spec;
    }
}

// --------------------------------------------------------------------------------------------------------------------

void
    FCk_2dGridSystem_SpawnerVisualizer::
    DrawVisualization(
        const UActorComponent*   Component,
        const FSceneView*        View,
        FPrimitiveDrawInterface* PDI)
{
    const ACk_EntitySpawner_UE* Spawner = nullptr;
    const auto* Spec = ck_grid_spawner_visualizer::Resolve_DrawableSpec(Component, Spawner);
    if (Spec == nullptr)
    { return; }

    ck::grid_editor::Draw_GridAuthoredOverlay(PDI, Spec, Spawner->GetActorTransform());
}

void
    FCk_2dGridSystem_SpawnerVisualizer::
    DrawVisualizationHUD(
        const UActorComponent* Component,
        const FViewport*       Viewport,
        const FSceneView*      View,
        FCanvas*               Canvas)
{
    const ACk_EntitySpawner_UE* Spawner = nullptr;
    const auto* Spec = ck_grid_spawner_visualizer::Resolve_DrawableSpec(Component, Spawner);
    if (Spec == nullptr)
    { return; }

    ck::grid_editor::Draw_GridTagLabels(Canvas, View, Spec, Spawner->GetActorTransform());
}

// --------------------------------------------------------------------------------------------------------------------
