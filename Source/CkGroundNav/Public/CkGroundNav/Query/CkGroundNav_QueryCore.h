#pragma once

#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Query/CkGroundNav_QueryTypes.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// The cell arithmetic every query shares. Nothing here searches; it turns world positions into cells,
// cells into tiles, and a cell of a layer into the one surface record a query reasons about.
//
// THREAD CONTRACT: every function here reads the field it is handed and nothing else, and writes only
// into its out-parameters. A caller holding a FCk_GroundNav_FieldPtr may call any of them from any
// thread; the immutable publish is the whole concurrency design.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * A cell addressed the way a tile stores it: which tile, then tile-local X and Y.
     *
     * The field's lattice is uniform — every tile shares the origin, the cell size and a tile span
     * that is a whole number of cells — so a field-wide cell index converts to this and back with
     * integer division and nothing that can drift.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_CellAddress
    {
    public:
        int32 _TileIndex = INDEX_NONE;
        int32 _CellX = INDEX_NONE;
        int32 _CellY = INDEX_NONE;

    public:
        auto Get_IsValid() const -> bool { return _TileIndex != INDEX_NONE; }

        auto operator==(const FCk_GroundNav_CellAddress&) const -> bool = default;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** How many cells one tile spans along each axis. */
    CKGROUNDNAV_API auto
    Get_CellsPerTile(
        const FCk_GroundNav_FieldParams& InParams) -> int32;

    /**
     * The field-wide cell under a world XY, in the lattice that starts at the field origin. May lie
     * outside the field; Get_CellAddress is what says whether a tile covers it.
     */
    CKGROUNDNAV_API auto
    Get_FieldCellAt(
        const FCk_GroundNav_FieldParams& InParams,
        const FVector2D&                 InWorldXY) -> FIntPoint;

    /** The tile and tile-local cell for a field-wide cell, or an invalid address outside the field. */
    CKGROUNDNAV_API auto
    Get_CellAddress(
        const FCk_GroundNav_Field& InField,
        const FIntPoint&           InFieldCell) -> FCk_GroundNav_CellAddress;

    CKGROUNDNAV_API auto
    Get_CellAddressAt(
        const FCk_GroundNav_Field& InField,
        const FVector2D&           InWorldXY) -> FCk_GroundNav_CellAddress;

    /**
     * Every cell whose CLOSED square holds the XY, the floor cell first.
     *
     * A point exactly on a cell line belongs to the cells on both sides of it. A projection answers
     * with the nearest point of a closed square, which can be its edge; a lookup that floored that
     * point into the neighbour would hand a walk a start it was never given: the wall top above the
     * floor it was projected onto, or the tight cell beside the one that admitted the body. Exact
     * equality, deliberately: the edge coordinate and the cell line come from the same arithmetic.
     */
    CKGROUNDNAV_API auto
    Get_CellAddressesAt(
        const FCk_GroundNav_Field&                              InField,
        const FVector2D&                                        InWorldXY,
        TArray<FCk_GroundNav_CellAddress, TInlineAllocator<4>>& OutCells) -> void;

    /** World XY of a field-wide cell's min corner. */
    CKGROUNDNAV_API auto
    Get_CellMinXY(
        const FCk_GroundNav_FieldParams& InParams,
        const FIntPoint&                 InFieldCell) -> FVector2D;

    /** The point of a cell's square nearest to a world XY — the XY itself when it lies inside. */
    CKGROUNDNAV_API auto
    Get_ClosestPointInCellXY(
        const FCk_GroundNav_FieldParams& InParams,
        const FIntPoint&                 InFieldCell,
        const FVector2D&                 InWorldXY) -> FVector2D;

    /** Distance from a world XY to a cell's square: zero inside, the gap to the nearest edge outside. */
    CKGROUNDNAV_API auto
    Get_HorizontalDistanceToCell(
        const FCk_GroundNav_FieldParams& InParams,
        const FIntPoint&                 InFieldCell,
        const FVector2D&                 InWorldXY) -> double;

    // ----------------------------------------------------------------------------------------------------------------

    /** A tile's build status by index; Unbuilt for an index the field does not have. */
    CKGROUNDNAV_API auto
    Get_TileStatus(
        const FCk_GroundNav_Field& InField,
        int32                      InTileIndex) -> ECk_GroundNav_BuildStatus;

    /**
     * The walkable surface at one cell of one layer.
     *
     * True only when the tile is built, the cell exists, the layer has a surface there and a plate
     * owns it — every walkable cell belongs to a plate, so a surface with no plate is not a surface.
     * The out-parameters are written only on success. One cell read, whatever the answer.
     */
    CKGROUNDNAV_API auto
    Get_SurfaceAt(
        const FCk_GroundNav_Field&      InField,
        const FCk_GroundNav_CellAddress& InCell,
        int32                           InLayer,
        FCk_GroundNav_SurfaceRef&       OutSurface,
        float&                          OutSurfaceZUu,
        float&                          OutClearanceUu) -> bool;

    /**
     * The surface normal at a surface, recovered from the heights of its plate.
     *
     * A published tile stores heights and clearance per cell and nothing else per cell, so the normal
     * is derived here rather than read: central differences of the surface height along each axis,
     * taken only over neighbouring cells of the SAME plate (one-sided at a plate edge, level where
     * the plate is one cell wide). A plate is planar within the merge tolerance, so this is the
     * plate's plane to within that tolerance, and a flat plate answers exactly up.
     */
    CKGROUNDNAV_API auto
    Get_SurfaceNormal(
        const FCk_GroundNav_Field&      InField,
        const FCk_GroundNav_SurfaceRef& InSurface) -> FVector;

    /** World-space centre of a surface's cell, at its surface height. */
    CKGROUNDNAV_API auto
    Get_SurfaceCentre(
        const FCk_GroundNav_Field&      InField,
        const FCk_GroundNav_SurfaceRef& InSurface) -> FVector;

    // ----------------------------------------------------------------------------------------------------------------

    /** Whether a cell with this much room admits this body. A zero radius is admitted everywhere. */
    CKGROUNDNAV_API auto
    Get_IsAdmitted(
        float                          InClearanceUu,
        const FCk_GroundNav_QueryAgent& InAgent) -> bool;

    /**
     * Whether the field can answer for this body at all.
     *
     * Clearance saturates at the field's ceiling: every cell more open than that reads exactly the
     * ceiling, so a radius above it cannot be admitted on clearance alone. A query for such a body
     * refuses with Blocked rather than mis-admitting; a caller that needs it rebakes with a higher
     * ceiling.
     */
    CKGROUNDNAV_API auto
    Get_IsRadiusAnswerable(
        const FCk_GroundNav_Field&      InField,
        const FCk_GroundNav_QueryAgent& InAgent) -> bool;
}

// --------------------------------------------------------------------------------------------------------------------
