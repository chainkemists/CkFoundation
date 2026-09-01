#pragma once

#include "CkGroundNav/Bake/CkGroundNav_AgentProfile.h"
#include "CkGroundNav/Bake/CkGroundNav_BakeTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_GeometryBatch.h"
#include "CkGroundNav/Bake/CkGroundNav_Plates.h"
#include "CkGroundNav/Field/CkGroundNav_FieldTypes.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * Everything one tile's bake needs, including where the tile sits in a field it knows nothing else
     * about.
     *
     * The tile's world size is derived from its CELL COUNT rather than taken from the authored size, so
     * a tile is cell-aligned however the size was configured. A tile whose edge fell mid-cell would put
     * the same piece of ground in two tiles at two different offsets, and nothing downstream could
     * reconcile the two answers.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_TileBakeParams
    {
    public:
        FCk_GroundNav_TileCoord _Coord;

        // The epoch this bake publishes as. The caller owns the counter; a tile cannot know what came
        // before it.
        FCk_GroundNav_Epoch _Epoch;

        // World min corner of tile (0,0) in XY, and the vertical slab every tile of the field spans.
        FVector2D _FieldOriginXY = FVector2D::ZeroVector;

        float _MinZUu = 0.0f;
        float _MaxZUu = 0.0f;

        FCk_GroundNav_BakeConfig _Config;
        FCk_GroundNav_AgentProfile _Profile;
        FCk_GroundNav_MergeTunables _MergeTunables;

        // The clearance ceiling, and therefore the halo width. Below it the field is exact; at or above
        // it every cell reads this number, which is what makes a tiled bake agree with a one-shot bake
        // instead of merely approximating it.
        float _MaxClearanceUu = 200.0f;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** How many cells of neighbouring ground a tile must see to answer correctly at its own edge. */
    CKGROUNDNAV_API auto
    Get_HaloCellCount(
        float InMaxClearanceUu,
        float InCellSizeUu) -> int32;

    /** The tile's own world bounds — what its published cells cover. */
    CKGROUNDNAV_API auto
    Get_TileBounds(
        const FCk_GroundNav_TileBakeParams& InParams) -> FBox;

    /**
     * The bounds the CALLER must supply geometry for: the tile plus its halo.
     *
     * Handing this only the tile's own bounds does not fail loudly — it produces a tile whose edge
     * cells read short on clearance and whose border may be eaten by the ledge filter, which looks
     * exactly like a world with a wall around every tile.
     */
    CKGROUNDNAV_API auto
    Get_TileHaloBounds(
        const FCk_GroundNav_TileBakeParams& InParams) -> FBox;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Bake one tile from geometry covering its halo bounds.
     *
     * Rasterization, walkability, layers and clearance all run over the halo-expanded lattice, so the
     * tile's edge cells are decided against real neighbouring ground rather than against the edge of
     * the world. The halo is then masked out BEFORE plates are decomposed, which is what keeps every
     * plate and every portal inside one tile; the published cells are tile-local and refer to nothing
     * outside.
     *
     * Clearance is clamped to the params' ceiling. Given the halo that ceiling implies, the clamp is
     * exactly equivalent to computing the transform over the whole world and clamping there, which is
     * the property that makes two tiles agree at their seam.
     *
     * A failed bake writes a tile whose status says so and whose cells are empty. It is never an empty
     * Built tile.
     *
     * Pure: no world, no registry, no physics.
     */
    CKGROUNDNAV_API auto
    DoBake_Tile(
        const FCk_GroundNav_GeometryBatch&  InGeometry,
        const FCk_GroundNav_TileBakeParams& InParams,
        FCk_GroundNav_Tile&                 OutTile) -> FCk_GroundNav_BakeStageResult;
}

// --------------------------------------------------------------------------------------------------------------------
