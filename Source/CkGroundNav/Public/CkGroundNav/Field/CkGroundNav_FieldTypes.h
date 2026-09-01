#pragma once

#include "CkCore/Format/CkFormat.h"

#include "CkGroundNav/Bake/CkGroundNav_Clearance.h"
#include "CkGroundNav/Bake/CkGroundNav_Plates.h"
#include "CkGroundNav/Bake/CkGroundNav_Portals.h"

#include <CoreMinimal.h>

#include "CkGroundNav_FieldTypes.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Whether a tile holds a usable answer.
 *
 * Unbuilt and Failed are deliberately distinct from an empty Built tile. A region with no floor in it
 * and a region whose bake could not run look identical in the data and could not be less alike to a
 * path: one is a place with nowhere to walk, the other is a place nothing is known about, and only
 * the second must never be reported to a caller as impassable.
 */
UENUM(BlueprintType)
enum class ECk_GroundNav_BuildStatus : uint8
{
    // Never baked, or invalidated and not yet rebuilt. Paths across it fail as unbuilt.
    Unbuilt,

    // Baked and published. Its contents are the answer for its epoch.
    Built,

    // A bake ran and could not finish. Whatever was published before is still published.
    Failed
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GroundNav_BuildStatus);

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * A monotone build counter.
     *
     * Staleness is DERIVED by comparing epochs at the read boundary and is never stored as a flag: a
     * stored flag has to be cleared by somebody, and the one that is missed is a reader trusting a
     * field that was rebuilt underneath it.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_Epoch
    {
    public:
        int64 _Value = 0;

    public:
        auto operator==(const FCk_GroundNav_Epoch&) const -> bool = default;

        auto Get_IsBuilt() const -> bool { return _Value > 0; }

        auto Get_Next() const -> FCk_GroundNav_Epoch { return FCk_GroundNav_Epoch{_Value + 1}; }

        auto Get_IsNewerThan(const FCk_GroundNav_Epoch& InOther) const -> bool
        {
            return _Value > InOther._Value;
        }
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** Where a tile sits in the field's tile lattice. Two integers, and nothing that can dangle. */
    struct CKGROUNDNAV_API FCk_GroundNav_TileCoord
    {
    public:
        int32 _X = 0;
        int32 _Y = 0;

    public:
        auto operator==(const FCk_GroundNav_TileCoord&) const -> bool = default;
    };

    CKGROUNDNAV_API auto
    Get_TileIndex(
        const FIntPoint&              InDivisions,
        const FCk_GroundNav_TileCoord& InCoord) -> int32;

    CKGROUNDNAV_API auto
    Get_TileCoord(
        const FIntPoint& InDivisions,
        int32            InTileIndex) -> FCk_GroundNav_TileCoord;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * What one tile knows about a single crossing that LEAVES it.
     *
     * Recorded from the tile's own halo, where the neighbouring ground was still present, and kept so
     * that composing two tiles never has to ask a fresh question about whether their edge cells are
     * adjacent. A published tile keeps no span field and no connection field, so the alternative would
     * be a second definition of adjacency living beside the first — and two definitions drift.
     *
     * Both surfaces are recorded because that is what makes the match exact: this tile's FAR surface is
     * the neighbour's NEAR surface, so two stubs describe the same crossing only when each side's
     * account of it agrees with the other's.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_SeamStub
    {
    public:
        // Outward from the tile, as a direction index (Get_DirectionOffset).
        int32 _Direction = 0;

        // Position along the shared edge, in this tile's own cell coordinates.
        int32 _AlongIndex = 0;

        int32 _PlateIndex = FCk_GroundNav_Plate::kNoPlate;

        float _NearSurfaceZUu = 0.0f;
        float _FarSurfaceZUu = 0.0f;

        // Already under the tile's clearance ceiling, and already the tighter of the two sides.
        float _ClearanceUu = 0.0f;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * One tile of the ground field: everything a query needs about its patch of world, and nothing a
     * rebuild of a neighbour could invalidate.
     *
     * Per-cell arrays exist for exactly two things — the surface height and the clearance. Everything
     * else is addressed per PLATE, which is what keeps a tile's memory proportional to how flat the
     * ground is rather than to how large it is.
     *
     * Coordinates are tile-local. The halo a tile bakes with is cropped away before publication, so
     * cell (0,0) is the tile's own corner and no index here refers to a neighbour.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_Tile
    {
    public:
        // Height stored where a layer has no walkable surface in a column. Chosen so an unguarded
        // comparison against a real height loses rather than silently wins.
        static constexpr float kNoSurfaceZ = -TNumericLimits<float>::Max();

    public:
        FCk_GroundNav_TileCoord _Coord;

        FCk_GroundNav_Epoch _Epoch;

        ECk_GroundNav_BuildStatus _Status = ECk_GroundNav_BuildStatus::Unbuilt;

        // World min corner of this tile's cell (0,0).
        FVector _Origin = FVector::ZeroVector;

        float _CellSizeUu = 0.0f;

        // The clearance ceiling this tile baked under. A radius above it cannot be admitted on
        // clearance alone, because every cell that open reads exactly this number.
        float _MaxClearanceUu = 0.0f;

        int32 _SizeX = 0;
        int32 _SizeY = 0;
        int32 _LayerCount = 0;

        // _LayerCount planes of _SizeX * _SizeY, layer-major. kNoSurfaceZ where the layer is empty.
        TArray<float> _SurfaceZ;

        FCk_GroundNav_ClearanceField _Clearance;
        FCk_GroundNav_PlateField _Plates;
        FCk_GroundNav_PortalField _Portals;

        // Crossings that leave this tile, for whoever composes it with its neighbours. Held on the tile
        // because only the tile's own bake could observe them; the portals they become are held on the
        // field, because they depend on two tiles and must be re-derived whenever either one rebuilds.
        TArray<FCk_GroundNav_SeamStub> _SeamStubs;

    public:
        auto Get_IsBuilt() const -> bool { return _Status == ECk_GroundNav_BuildStatus::Built; }

        auto Get_IsValidCell(int32 InX, int32 InY, int32 InLayer) const -> bool
        {
            return InX >= 0 && InY >= 0 && InLayer >= 0 &&
                   InX < _SizeX && InY < _SizeY && InLayer < _LayerCount;
        }

        auto Get_SurfaceZAt(int32 InX, int32 InY, int32 InLayer) const -> float
        {
            return Get_IsValidCell(InX, InY, InLayer)
                ? _SurfaceZ[(InLayer * _SizeX * _SizeY) + (InY * _SizeX) + InX]
                : kNoSurfaceZ;
        }

        auto Get_HasSurfaceAt(int32 InX, int32 InY, int32 InLayer) const -> bool
        {
            return Get_SurfaceZAt(InX, InY, InLayer) != kNoSurfaceZ;
        }

        /** World-space centre of a cell's walkable surface. Only meaningful where one exists. */
        auto Get_CellCentre(int32 InX, int32 InY, int32 InLayer) const -> FVector;

        auto Get_WalkableCellCount() const -> int32;
    };
}

// --------------------------------------------------------------------------------------------------------------------
