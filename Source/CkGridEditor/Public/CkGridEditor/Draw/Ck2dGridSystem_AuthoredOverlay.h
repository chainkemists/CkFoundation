#pragma once

#include "CoreMinimal.h"

#include "Math/IntPoint.h"
#include "Math/Transform.h"

// --------------------------------------------------------------------------------------------------------------------

class ACk_EntitySpawner_UE;
class UCk_2dGridSystem_Spec;

class FCanvas;
class FPrimitiveDrawInterface;
class FSceneView;

// --------------------------------------------------------------------------------------------------------------------

// Shared authored-state grid overlay draw. Factored out of the Grid Paint UEdMode (::Render) so the
// SAME wireframe + per-cell state colors are used both IN the paint mode (UEdMode::Render) and OUT of
// it (the spawner component visualizer). Previously the out-of-mode preview used a different path (the
// runtime OBB DebugDraw), which could not tell a blocker-disabled cell from a shape-disabled one and
// drew none of the tag-blue / authored colors — so the two views disagreed. Everything here reads the
// authoring Spec directly (the single source of truth for authored state).
//
// Coordinate convention matches the runtime grid (UCk_Utils_Grid2D_UE::Get_CoordinateAsLocation): cell
// (x,y)'s MIN corner is at grid-local (x*CellSize.X, y*CellSize.Y, 0); InTransform maps local to world.
namespace ck::grid_editor
{
    // Authored-state color convention for the cell overlay. Priority is disabled > blocker > tagged >
    // enabled (a cell that is both disabled and inside a blocker reads as disabled). Mirrored by
    // Resolve_CellColor below and by the toolkit Details-panel state readout.
    constexpr auto ColorEnabled  = FLinearColor(0.0f, 1.0f, 0.0f); // green
    constexpr auto ColorDisabled = FLinearColor(1.0f, 0.0f, 0.0f); // red
    constexpr auto ColorBlocker  = FLinearColor(1.0f, 0.5f, 0.0f); // orange
    constexpr auto ColorTagged   = FLinearColor(0.2f, 0.4f, 1.0f); // blue tint
    constexpr auto ColorPivot    = FLinearColor(1.0f, 1.0f, 0.0f); // yellow

    constexpr auto CellLineThickness  = 1.0f;
    constexpr auto CellMarkerInset     = 0.12;
    constexpr auto CellMarkerThickness = 2.0f;
    constexpr auto PivotMarkerSize     = 20.0f;
    constexpr auto PivotLineThickness  = 3.0f;

    // What the shared overlay draws. The interaction overlays (hover/select/blocker-drag/blocker-group)
    // are NOT here — they are paint-mode-only and stay in the UEdMode.
    struct FAuthoredOverlayOptions
    {
        bool bDrawBaseGrid     = true;
        bool bDrawStateMarkers = true;
        bool bDrawPivot        = true;
    };

    // Resolves the grid authoring Spec hosted by InSpawner's entity script, or nullptr when the spawner
    // is null / does not host a UCk_2dGridSystem_EntityScript / has no Spec assigned. The Spec is a
    // private UPROPERTY on the grid script (no public getter), so this reads it reflectively — the single
    // place that reflective read lives, shared by the UEdMode selection resolve and the visualizer.
    CKGRIDEDITOR_API auto
    Resolve_SpecFromSpawner(
        const ACk_EntitySpawner_UE* InSpawner) -> UCk_2dGridSystem_Spec*;

    // Returns true if InCoordinate is covered by any of the Spec's blocker [RangeMin, RangeMax] rects.
    CKGRIDEDITOR_API auto
    Is_CoveredByBlocker(
        const UCk_2dGridSystem_Spec* InSpec,
        const FIntPoint&             InCoordinate) -> bool;

    // Returns true if InCoordinate carries any authored tag (grid-wide default or per-cell override).
    CKGRIDEDITOR_API auto
    Has_AuthoredTag(
        const UCk_2dGridSystem_Spec* InSpec,
        const FIntPoint&             InCoordinate) -> bool;

    // Authored color for a single cell, applying the disabled > blocker > tagged > enabled priority.
    CKGRIDEDITOR_API auto
    Resolve_CellColor(
        const UCk_2dGridSystem_Spec* InSpec,
        const FIntPoint&             InCoordinate) -> FLinearColor;

    // Draws the authored-state PDI overlay for InSpec at InTransform: the green base-grid wireframe, an
    // inset state-color marker on every non-enabled cell, and the pivot gizmo (gated by InOptions). No-op
    // on an invalid Spec or non-positive CellSize/Dimensions.
    CKGRIDEDITOR_API auto
    Draw_GridAuthoredOverlay(
        FPrimitiveDrawInterface*       InPDI,
        const UCk_2dGridSystem_Spec*   InSpec,
        const FTransform&              InTransform,
        const FAuthoredOverlayOptions& InOptions = FAuthoredOverlayOptions{}) -> void;

    // The per-cell tag-text toggle (cvar ck.Grid.PreviewShowTags). Read by BOTH the UEdMode (DrawHUD) and
    // the spawner visualizer (DrawVisualizationHUD) so the in/out-of-mode tag labels match. Default off
    // (text is noisy on large grids).
    CKGRIDEDITOR_API auto
    Should_ShowCellTags() -> bool;

    // The effective tag text for a cell: grid-wide DefaultCellTags unioned with the cell's PerCellTags
    // override. Empty string when the cell carries no tags. Used for the world-space tag labels.
    CKGRIDEDITOR_API auto
    Get_CellTagText(
        const UCk_2dGridSystem_Spec* InSpec,
        const FIntPoint&             InCoordinate) -> FString;

    // Draws per-cell tag TEXT labels onto the HUD canvas (world cell-center projected to screen). No-op
    // when Should_ShowCellTags() is false or the Spec/view is invalid. Shared by the UEdMode DrawHUD and
    // the visualizer DrawVisualizationHUD.
    CKGRIDEDITOR_API auto
    Draw_GridTagLabels(
        FCanvas*                     InCanvas,
        const FSceneView*            InView,
        const UCk_2dGridSystem_Spec* InSpec,
        const FTransform&            InTransform) -> void;
}

// --------------------------------------------------------------------------------------------------------------------
