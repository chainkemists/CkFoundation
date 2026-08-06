#pragma once

#include "CoreMinimal.h"

#include "Math/IntPoint.h"
#include "Math/Transform.h"

#include <GameplayTagContainer.h>

// --------------------------------------------------------------------------------------------------------------------

class ACk_EntitySpawner_UE;
class UCk_2dGridSystem_AuthoringSpec;

class FCanvas;
class FPrimitiveDrawInterface;
class FSceneView;

// --------------------------------------------------------------------------------------------------------------------

// Shared authored-state grid overlay, drawn identically by the Grid Paint UEdMode and the out-of-mode
// spawner visualizer, and read straight off the authoring Spec (the source of truth for authored state).
// Coordinate convention matches the runtime grid (UCk_Utils_Grid2D_UE::Get_CoordinateAsLocation): cell
// (x,y)'s MIN corner is at grid-local (x*CellSize.X, y*CellSize.Y, 0); InTransform maps local to world.
namespace ck::grid_editor
{
    // Priority: disabled > blocker > tagged > enabled. Mirrored by Resolve_CellColor and the Details readout.
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

    // A DOUBLE inset ring, because a single thin state marker in the tag color read as barely visible.
    constexpr auto TaggedMarkerInsetOuter = 0.06;
    constexpr auto TaggedMarkerInsetInner = 0.22;
    constexpr auto TaggedMarkerThickness  = 3.0f;

    // The interaction overlays (hover/select/blocker) are NOT here — they are paint-mode-only.
    struct FAuthoredOverlayOptions
    {
        bool bDrawBaseGrid     = true;
        bool bDrawStateMarkers = true;
        bool bDrawPivot        = true;
    };

    // nullptr when the spawner is null, hosts no UCk_2dGridSystem_EntityScript, or has no Spec assigned.
    // Spec is a private UPROPERTY with no getter, so this is the ONE place that reflective read lives.
    CKGRIDEDITOR_API auto
    Resolve_SpecFromSpawner(
        const ACk_EntitySpawner_UE* InSpawner) -> UCk_2dGridSystem_AuthoringSpec*;

    // Returns true if InCoordinate is covered by any of the Spec's blocker [RangeMin, RangeMax] rects.
    CKGRIDEDITOR_API auto
    Is_CoveredByBlocker(
        const UCk_2dGridSystem_AuthoringSpec* InSpec,
        const FIntPoint&             InCoordinate) -> bool;

    // Returns true if InCoordinate carries any authored tag (grid-wide default or per-cell override).
    CKGRIDEDITOR_API auto
    Has_AuthoredTag(
        const UCk_2dGridSystem_AuthoringSpec* InSpec,
        const FIntPoint&             InCoordinate) -> bool;

    CKGRIDEDITOR_API auto
    Resolve_CellColor(
        const UCk_2dGridSystem_AuthoringSpec* InSpec,
        const FIntPoint&             InCoordinate) -> FLinearColor;

    // Hue hashed from InName — stable across sessions, independent of tag order/count.
    CKGRIDEDITOR_API auto
    Resolve_TagColor_FromName(
        const FName& InName) -> FLinearColor;

    // Invalid tag -> ColorTagged.
    CKGRIDEDITOR_API auto
    Resolve_TagColor(
        const FGameplayTag& InTag) -> FLinearColor;

    // First tag of the cell's PerCellTags entry; invalid when it has no override. DefaultCellTags are
    // deliberately NOT considered — grid-wide defaults would recolor the whole grid; they only reach the legend.
    CKGRIDEDITOR_API auto
    Resolve_PrimaryCellTag(
        const UCk_2dGridSystem_AuthoringSpec* InSpec,
        const FIntPoint&             InCoordinate) -> FGameplayTag;

    // A cell with N tags counts once per tag. Sorted by tag name for stable list ordering. Grid-wide
    // DefaultCellTags are NOT included (the toolkit lists those separately).
    CKGRIDEDITOR_API auto
    Collect_PerCellTagsWithCounts(
        const UCk_2dGridSystem_AuthoringSpec* InSpec) -> TArray<TPair<FGameplayTag, int32>>;

    // Every coordinate whose PerCellTags entry contains InTag. Empty when the Spec/tag is invalid.
    CKGRIDEDITOR_API auto
    Get_CellsWithTag(
        const UCk_2dGridSystem_AuthoringSpec* InSpec,
        const FGameplayTag&          InTag) -> TArray<FIntPoint>;

    // No-op on an invalid Spec or non-positive CellSize/Dimensions.
    CKGRIDEDITOR_API auto
    Draw_GridAuthoredOverlay(
        FPrimitiveDrawInterface*       InPDI,
        const UCk_2dGridSystem_AuthoringSpec*   InSpec,
        const FTransform&              InTransform,
        const FAuthoredOverlayOptions& InOptions = FAuthoredOverlayOptions{}) -> void;

    // Cvar ck.Grid.PreviewShowTags, default off (text is noisy on large grids).
    CKGRIDEDITOR_API auto
    Should_ShowCellTags() -> bool;

    // Grid-wide DefaultCellTags unioned with the cell's PerCellTags override; empty when it carries none.
    CKGRIDEDITOR_API auto
    Get_CellTagText(
        const UCk_2dGridSystem_AuthoringSpec* InSpec,
        const FIntPoint&             InCoordinate) -> FString;

    // No-op when Should_ShowCellTags() is false or the Spec/view is invalid.
    CKGRIDEDITOR_API auto
    Draw_GridTagLabels(
        FCanvas*                     InCanvas,
        const FSceneView*            InView,
        const UCk_2dGridSystem_AuthoringSpec* InSpec,
        const FTransform&            InTransform) -> void;
}

// --------------------------------------------------------------------------------------------------------------------
