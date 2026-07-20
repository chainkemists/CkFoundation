#pragma once

#include "CkMinimap/CkMinimap_Fragment_Data.h"
#include "CkMinimap/CkMinimap_WorldBounds.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Pure minimap math — no entities, no world. Unit-tested in isolation (Tests/Test_CkMinimap_Math.cpp).
//
// Conventions (documented contract, shared with CkCompass): world yaw degrees, 0 = North = +X world,
// 90 = East = +Y world. FRAME space is center-origin with +X = screen right and +Y = screen DOWN (UMG),
// in units of the half-frame — the visible frame is [-1, 1]². North-up mapping:
//   Frame.X = WorldDelta.Y / Extent;  Frame.Y = -WorldDelta.X / Extent
// (a POI due North projects to (0, -1) — screen up; due East to (1, 0) — screen right).

namespace ck::minimap
{
    // Projects a world position into observer-centric frame space. NorthLocked ignores InViewYawDegrees;
    // RotateWithObserver rotates the world delta by -InViewYawDegrees first (observer's facing = screen up).
    // A non-positive InViewExtent returns (0, 0) — callers validate extent at the request boundary.
    CKMINIMAP_API auto
    Get_WorldToFrame(
        const FVector& InWorldPos,
        const FVector& InViewOrigin,
        float InViewYawDegrees,
        float InViewExtent,
        ECk_Minimap_RotationMode InRotationMode) -> FVector2D;

    // Exact inverse of Get_WorldToFrame for the XY plane (cursor -> world for map-click pings).
    // Returns world XY only — callers pick their own Z (trace or gameplay), never an invented one.
    CKMINIMAP_API auto
    Get_FrameToWorld(
        const FVector2D& InFramePos,
        const FVector& InViewOrigin,
        float InViewYawDegrees,
        float InViewExtent,
        ECk_Minimap_RotationMode InRotationMode) -> FVector2D;

    // Projects a world position into [-1, 1]² frame space over a fixed world rectangle (always north-up —
    // the world-map projection). Invalid bounds return (0, 0).
    CKMINIMAP_API auto
    Get_BoundsToFrame(
        const FVector& InWorldPos,
        const FCk_Minimap_WorldBounds& InBounds) -> FVector2D;

    // Exact inverse of Get_BoundsToFrame (world-map cursor -> world XY).
    CKMINIMAP_API auto
    Get_FrameToWorld_Bounds(
        const FVector2D& InFramePos,
        const FCk_Minimap_WorldBounds& InBounds) -> FVector2D;

    // Frame-membership test. Edge-inclusive: |1| is INSIDE (mirrors the compass' edge-inclusive arc).
    CKMINIMAP_API auto
    Get_IsInsideFrame(
        const FVector2D& InFramePos,
        ECk_Minimap_FrameShape InFrameShape) -> bool;

    // TOTAL boundary projection: positions already inside the frame (and the zero vector) return
    // unchanged; outside positions are pinned onto the frame boundary preserving direction
    // (rect: / max(|x|, |y|); circle: / length).
    CKMINIMAP_API auto
    Get_ClampToFrame(
        const FVector2D& InFramePos,
        ECk_Minimap_FrameShape InFrameShape) -> FVector2D;

    // Grid dimensions for a fog grid over InBounds: ceil(extent / cell size) per axis, min 1x1.
    // X counts cells along world X, Y along world Y. Cells are InCellSize-sized, anchored at the bounds
    // min corner — the last row/column may overshoot the bounds (partial cells).
    CKMINIMAP_API auto
    Get_CellCounts(
        const FCk_Minimap_WorldBounds& InBounds,
        float InCellSize) -> FIntPoint;

    // Row-major cell index (CellY * Counts.X + CellX) for a world position, or INDEX_NONE outside the
    // bounds. The bounds test is inclusive on BOTH edges; in-bounds cell coords are clamped-floor into
    // [0, Counts-1] (max-edge positions belong to the last cell).
    CKMINIMAP_API auto
    Get_CellIndex(
        const FVector& InWorldPos,
        const FCk_Minimap_WorldBounds& InBounds,
        float InCellSize,
        const FIntPoint& InCellCounts) -> int32;

    // World-XY center of a cell (reveal stamps test THIS point against the reveal radius).
    CKMINIMAP_API auto
    Get_CellCenter(
        int32 InCellX,
        int32 InCellY,
        const FCk_Minimap_WorldBounds& InBounds,
        float InCellSize) -> FVector2D;

    // UV rect of a cell for painters, normalized over the BOUNDS extent and clamped to [0, 1] — NOT over
    // the (possibly overshooting) grid extent, so partial edge cells never seam past the map texture.
    // UV convention matches Get_BoundsToFrame: UV = (Frame + 1) / 2 (U right = world +Y, V down = world -X).
    CKMINIMAP_API auto
    Get_CellUVRect(
        int32 InCellIndex,
        const FCk_Minimap_WorldBounds& InBounds,
        float InCellSize) -> FBox2D;
}

// --------------------------------------------------------------------------------------------------------------------
