#pragma once

#include "CkCore/Format/CkFormat.h"

#include "CkGroundNav/Bake/CkGroundNav_Boundary.h"
#include "CkGroundNav/Bake/CkGroundNav_Clearance.h"
#include "CkGroundNav/Bake/CkGroundNav_LinkTypes.h"
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

/**
 * How reading a serialized field ended.
 *
 * A blob a reader cannot use is REFUSED WITH A STATUS, never with an ensure. A cook older than the
 * code reading it, or a file that is not a field at all, is an ordinary state of a shipped game - the
 * caller bakes at runtime instead - and an ensure would turn the ordinary case into a crash in a
 * development build while telling a shipping build nothing. The field the caller passed is left
 * untouched in every case but Loaded, so the fallback has something to fall back TO.
 */
UENUM(BlueprintType)
enum class ECk_GroundNav_LoadStatus : uint8
{
    // The blob was read and every derived array re-derived. The field is usable.
    Loaded,

    // The leading bytes are not a ground-field blob, or are one of the other granularity.
    WrongMagic,

    // A ground-field blob written under a different version of the format. Nothing is decoded: the
    // members would be read at the wrong offsets and the field would come out plausible and wrong.
    WrongVersion,

    // The blob ended before the field did.
    Truncated,

    // The blob names a gameplay tag this process does not have. A tag is part of what the field
    // MEANS - an area policy, a link's user type - so a field loaded without one would answer
    // differently from the field that was written.
    UnknownTag,

    // A tile blob read against a field divided differently. A tile carries tile-local indices and
    // nothing else, so placing one against a lattice that did not produce it is cells over the wrong
    // ground.
    LatticeMismatch,

    // The blob spells a value the field cannot hold: a non-finite coordinate, or a rotation that is
    // not a unit quaternion. Neither is caught by anything downstream - a NaN bound compares false
    // against everything it is tested with, and a rotation that is not unit is silently renormalised
    // on first use - so it is refused where it is read.
    Corrupt
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GroundNav_LoadStatus);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Where the ground a volume has published came from, and - when it did not come from the cook - why
 * not.
 *
 * PROVENANCE, never health. A field baked at runtime because no cook was found answers every query
 * exactly as well as one loaded from a cook; what this says is which of the two happened, so a level
 * that was supposed to ship cooked ground can be caught reporting that it did not. Whether the ground
 * is usable at all is what ECk_NavSurface_ProviderHealth answers, and the two are deliberately not
 * folded together.
 */
UENUM(BlueprintType)
enum class ECk_GroundNav_CookStatus : uint8
{
    // The volume authored no cook key. No cooked field is ever looked up for it and none is ever
    // written - the honest state of every gym, test and prototype volume.
    RuntimeOnly,

    // A key is authored and no index asset exists at the convention path for {this level package,
    // that key}. Absence is LEGAL - a level opts into cooked ground - and the volume bakes at runtime.
    MissingCook,

    // An index exists and cannot be used: its fingerprint names inputs that have since moved, its
    // format version is one the reader does not speak, its lattice is not this volume's, or a tile it
    // names would not load. The volume bakes at runtime rather than reading ground that describes
    // another world.
    StaleCook,

    // The published field came out of the cook.
    Cooked
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GroundNav_CookStatus);

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
     * Where on the field a surface answer lives. Integer identity only, so a result can be held,
     * compared and drawn after the field it came from has been rebuilt; it is valid only against
     * the field epoch it was answered from.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_SurfaceRef
    {
    public:
        int32 _TileIndex = INDEX_NONE;
        int32 _LayerIndex = INDEX_NONE;

        // Tile-local, like every index a tile carries.
        int32 _CellX = INDEX_NONE;
        int32 _CellY = INDEX_NONE;
        int32 _PlateIndex = FCk_GroundNav_Plate::kNoPlate;

    public:
        auto Get_IsValid() const -> bool
        {
            return _TileIndex != INDEX_NONE && _PlateIndex != FCk_GroundNav_Plate::kNoPlate;
        }

        auto operator==(const FCk_GroundNav_SurfaceRef&) const -> bool = default;
    };

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
     * What the rasterizer and the filters did to produce one tile, kept beside the tile so a viewer can
     * report a FIELD the way it reports a single-region bake. Three integers; a tile that never baked
     * carries zeros, and its status says why.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_TileBakeStats
    {
    public:
        // Triangles handed to the rasterizer for this tile's halo bounds, before any drop.
        int32 _SourceTriangleCount = 0;

        // Spans in the halo-expanded column field after rasterization, before filtering.
        int32 _RasterizedSpanCount = 0;

        // Cells the rasterizer accepted on slope that the walkability filters then demoted.
        int32 _RejectedCellCount = 0;
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

        // Every plate edge no crossing covers, with its winding fixed and a coarse index over it, so a
        // boundary query is a bounded scan and never re-derives an edge. Runs on the tile rim wait on
        // the field, which knows the neighbours.
        FCk_GroundNav_BoundaryField _Boundary;

        // Crossings that leave this tile, for whoever composes it with its neighbours. Held on the tile
        // because only the tile's own bake could observe them; the portals they become are held on the
        // field, because they depend on two tiles and must be re-derived whenever either one rebuilds.
        TArray<FCk_GroundNav_SeamStub> _SeamStubs;

        FCk_GroundNav_TileBakeStats _BakeStats;

    public:
        auto Get_IsBuilt() const -> bool { return _Status == ECk_GroundNav_BuildStatus::Built; }

        /** Bytes this tile holds on the heap, every array included. Exact, and therefore comparable. */
        auto Get_AllocatedSize() const -> SIZE_T;

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

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * What one authored link RESOLVED to against the field that carries it.
     *
     * Kept apart from the authored record for the same reason a markup's footprint is: the record holds
     * two world points and nothing a rebuild could invalidate, while every index here — tile, cell,
     * plate — is valid only against the field it was derived on, exactly like a reachability label. The
     * whole array is re-derived on every publish rather than patched, so a resolution can never outlive
     * the plate numbering it was answered under.
     *
     * An end that projected onto nothing is HELD, not dropped: its status says whether the ground under
     * it is missing or merely unbaked, and the record stays on its volume either way, so the next
     * publish over that tile resolves it without the author doing anything.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_ResolvedLink
    {
    public:
        int32 _Id = INDEX_NONE;

        FVector _Start = FVector::ZeroVector;
        FVector _End = FVector::ZeroVector;

        FCk_GroundNav_SurfaceRef _StartSurface;
        FCk_GroundNav_SurfaceRef _EndSurface;

        // Flat plate indices, the index space the reachability labels and the crossings speak.
        int32 _StartFlatPlate = INDEX_NONE;
        int32 _EndFlatPlate = INDEX_NONE;

        ECk_NavSurface_QueryStatus _StartStatus = ECk_NavSurface_QueryStatus::NoSurface;
        ECk_NavSurface_QueryStatus _EndStatus = ECk_NavSurface_QueryStatus::NoSurface;

        // Copied from the record so a consumer reading a resolved link never has to go back to the
        // volume that authored it, and so the field stays a self-contained answer.
        ECk_GroundNav_LinkDirection _Direction = ECk_GroundNav_LinkDirection::Bidirectional;

        float _CostMultiplierForward = 1.0f;
        float _CostMultiplierBackward = 1.0f;

        float _ClearanceUu = FCk_GroundNav_LinkRecord::kAdmitsAnyAgentClearanceUu;

        FGameplayTag _AreaTag;
        FGameplayTag _UserTypeTag;

        ECk_EnableDisable _Enable = ECk_EnableDisable::Enable;

    public:
        auto operator==(const FCk_GroundNav_ResolvedLink&) const -> bool = default;

        /** Both ends found ground. Says nothing about whether the link may be used. */
        auto Get_IsResolved() const -> bool
        {
            return _StartStatus == ECk_NavSurface_QueryStatus::Success &&
                   _EndStatus == ECk_NavSurface_QueryStatus::Success;
        }

        /** Resolved AND switched on: the two conditions under which the link joins anything. */
        auto Get_IsTraversable() const -> bool
        {
            return Get_IsResolved() && _Enable == ECk_EnableDisable::Enable;
        }
    };
}

// --------------------------------------------------------------------------------------------------------------------
