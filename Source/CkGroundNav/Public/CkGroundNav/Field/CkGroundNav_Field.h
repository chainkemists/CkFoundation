#pragma once

#include "CkGroundNav/Backend/CkGroundNav_GeometryBackend.h"
#include "CkGroundNav/Field/CkGroundNav_TileBake.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * What every tile of one field shares: where the field starts, how it is divided, and the single
     * set of bake settings all its tiles were produced under.
     *
     * Held once rather than per tile on purpose. Two tiles of one field baked at different cell sizes
     * or different agent profiles would each be internally consistent and jointly meaningless, and
     * nothing downstream could detect it from the tiles alone.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_FieldParams
    {
    public:
        FVector2D _OriginXY = FVector2D::ZeroVector;

        FIntPoint _Divisions = FIntPoint{1, 1};

        float _MinZUu = 0.0f;
        float _MaxZUu = 0.0f;

        FCk_GroundNav_BakeConfig _Config;
        FCk_GroundNav_AgentProfile _Profile;
        FCk_GroundNav_MergeTunables _MergeTunables;

        float _MaxClearanceUu = 200.0f;

    public:
        auto Get_IsValid() const -> bool;

        auto Get_TileCount() const -> int32 { return _Divisions.X * _Divisions.Y; }

        /** Edge length of one tile, snapped to a whole number of cells. */
        auto Get_TileSpanUu() const -> double;

        auto Get_Bounds() const -> FBox;

        /** The params one tile bakes under, which is this shape plus that tile's own identity. */
        auto Get_TileBakeParams(
            const FCk_GroundNav_TileCoord& InCoord,
            const FCk_GroundNav_Epoch&     InEpoch) const -> FCk_GroundNav_TileBakeParams;

        /** Which tile covers a world position, or an out-of-range coord if the field does not reach it. */
        auto Get_TileCoordAt(
            const FVector& InWorldPosition) const -> FCk_GroundNav_TileCoord;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * One crossing between two TILES.
     *
     * Lives on the field rather than on either tile because it depends on both, and either can be
     * rebuilt on its own. Re-derived whenever the field is composed and never patched — a seam portal
     * that outlived the tile it crossed into would be a route that no longer exists, which is the one
     * error a path consumer cannot detect for itself.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_SeamPortal
    {
    public:
        int32 _TileIndexA = INDEX_NONE;
        int32 _TileIndexB = INDEX_NONE;

        // Plate indices are TILE-LOCAL, so a plate is only addressable through the tile beside it here.
        int32 _PlateA = FCk_GroundNav_Plate::kNoPlate;
        int32 _PlateB = FCk_GroundNav_Plate::kNoPlate;

        // From A to B: only 0 (+X) and 1 (+Y) occur, so a seam is never composed twice.
        int32 _Direction = 0;

        // The run along the shared edge, in A's cell coordinates, inclusive.
        int32 _AlongMin = 0;
        int32 _AlongMax = 0;

        float _MinEndZUu = 0.0f;
        float _MaxEndZUu = 0.0f;

        float _TraversalClearanceUu = 0.0f;

    public:
        auto Get_CellCount() const -> int32 { return (_AlongMax - _AlongMin) + 1; }
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * A whole ground field: its tiles and the settings they were baked under.
     *
     * TREAT AS IMMUTABLE ONCE PUBLISHED. Readers hold a shared reference to one of these and are
     * guaranteed a self-consistent structure for as long as they hold it; a rebuild produces a NEW
     * field and swaps the pointer, so a reader mid-query never sees a half-rebuilt world. That is also
     * what makes an off-thread query possible without a lock discipline, and what makes repair unable
     * to corrupt anything.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_Field
    {
    public:
        FCk_GroundNav_FieldParams _Params;

        // Get_TileCount() entries, addressed by Get_TileIndex. A tile that never baked is present and
        // says Unbuilt — a missing entry and an unbuilt one would answer the same query differently.
        TArray<FCk_GroundNav_Tile> _Tiles;

        // Derived at composition from the tiles' seam stubs, never carried across a rebuild.
        TArray<FCk_GroundNav_SeamPortal> _SeamPortals;

        // Where each tile's plates begin in _ReachabilityLabels. Get_TileCount() + 1 entries, so the
        // last one is the total and every tile's range is a subtraction away.
        TArray<int32> _TilePlateOffsets;

        // One component label per plate of the whole field. Valid only within this field: labels are
        // re-derived on every composition and never carried across a rebuild, because a plate index
        // that survived a rebuild by luck would carry a label that did not.
        TArray<int32> _ReachabilityLabels;

        FCk_GroundNav_Epoch _Epoch;

    public:
        auto Get_TileCount() const -> int32 { return _Tiles.Num(); }

        auto Get_SeamPortalCount() const -> int32 { return _SeamPortals.Num(); }

        auto Get_Tile(const FCk_GroundNav_TileCoord& InCoord) const -> const FCk_GroundNav_Tile*;

        auto Get_TileAt(const FVector& InWorldPosition) const -> const FCk_GroundNav_Tile*;

        auto Get_BuiltTileCount() const -> int32;

        /**
         * Which connected component a plate belongs to, or INDEX_NONE if it has no label.
         *
         * COMPONENTS IGNORE AGENT SIZE. They are computed over the crossings that exist at all, not
         * over the crossings a given body fits through, which makes the guarantee one-directional: a
         * DIFFERENT label proves two plates cannot reach each other, and the SAME label proves only
         * that a body of zero width could. Anything that has to hold for a real agent must still check
         * the clearance of the crossings on its route.
         */
        auto Get_ReachabilityLabel(int32 InTileIndex, int32 InPlateIndex) const -> int32;

        /**
         * Whether two plates are provably out of each other's reach.
         *
         * The name is the contract. This is the only direction the labels can answer on their own, so
         * there is deliberately no Get_AreReachable beside it to be reached for by mistake.
         */
        auto Get_AreProvablyDisconnected(
            int32 InTileIndexA, int32 InPlateIndexA,
            int32 InTileIndexB, int32 InPlateIndexB) const -> bool;

        auto Get_ReachabilityComponentCount() const -> int32;

        /**
         * One monotone number for the whole field: the sum of its tiles' epochs.
         *
         * A sum rather than a max, because tiles rebuild independently. Two fields whose newest tile
         * has the same epoch can still differ in every other tile, and a max would call them equal.
         */
        auto Get_AggregatedTileEpochSum() const -> int64;
    };

    using FCk_GroundNav_FieldPtr = TSharedPtr<const FCk_GroundNav_Field>;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Owns the field that is currently published, and the counter that says which one it is.
     *
     * Publishing SWAPS a pointer; it never edits what is already out. A reader that took a reference
     * before a rebuild keeps reading exactly the field it took, and finds out it is behind by comparing
     * epochs — staleness is derived at the read boundary and never stored as a flag, because a stored
     * flag has to be cleared by somebody and the one that gets missed is a reader trusting a field that
     * was rebuilt underneath it.
     *
     * A failed build leaves the published pointer untouched and records the failure. The previous
     * answer is old; it is not wrong, and it is the only answer there is.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_FieldPublisher
    {
    private:
        FCk_GroundNav_FieldPtr _Published;

        FCk_GroundNav_Epoch _Epoch;

        ECk_GroundNav_BuildStatus _Status = ECk_GroundNav_BuildStatus::Unbuilt;

    public:
        auto Get_Published() const -> FCk_GroundNav_FieldPtr { return _Published; }

        auto Get_Epoch() const -> FCk_GroundNav_Epoch { return _Epoch; }

        auto Get_Status() const -> ECk_GroundNav_BuildStatus { return _Status; }

        auto Get_HasPublished() const -> bool { return _Published.IsValid(); }

        /** Whether a reader holding a field of the given epoch is looking at something superseded. */
        auto Get_IsStale(const FCk_GroundNav_Epoch& InObservedEpoch) const -> bool
        {
            return _Epoch.Get_IsNewerThan(InObservedEpoch);
        }

        /** The epoch the NEXT build will publish under. A builder stamps its tiles with this. */
        auto Get_NextEpoch() const -> FCk_GroundNav_Epoch { return _Epoch.Get_Next(); }

    public:
        /**
         * Swap in a new field and advance the epoch.
         *
         * Takes the field by shared reference and keeps it const, so the caller cannot retain a mutable
         * path into something readers are already holding.
         */
        auto Request_Publish(
            const TSharedRef<const FCk_GroundNav_Field>& InField) -> void;

        /** Record that a build failed. What is published stays published. */
        auto Request_RecordFailure() -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Bake every tile of a field in one pass and return it ready to publish.
     *
     * Each tile is baked from geometry fetched for its HALO bounds, not its own — a tile handed only
     * its own geometry produces a false pinch at every seam, and nothing about the result says so.
     *
     * A tile whose bake fails is present in the field carrying its failure status. The field as a whole
     * completes: one unbuilt tile is a hole in what is known, and the rest of the world is still worth
     * publishing.
     *
     * Pure but for the backend: no world, no registry, no physics of its own.
     */
    /**
     * Derive every crossing between neighbouring tiles from the stubs those tiles recorded.
     *
     * Two stubs describe one crossing only when each side's account agrees with the other's — this
     * tile's far surface is the neighbour's near surface, and the reverse — which mirrors the
     * mutual-agreement rule the connection field already enforces within a tile and keeps a single
     * definition of adjacency in the codebase.
     *
     * A neighbour that is not built yields NO seam portal. The boundary is then a hard edge and a path
     * across it fails as unbuilt, never as blocked: the first is a place nothing is known about, and
     * telling a caller it is impassable would be a lie it cannot check.
     */
    CKGROUNDNAV_API auto
    DoDerive_SeamPortals(
        FCk_GroundNav_Field& InOutField) -> void;

    /**
     * Label every plate of the field with the component it belongs to, over both within-tile and
     * cross-tile crossings.
     *
     * Labels are assigned in tile-then-plate scan order after the merging is done, so the numbering
     * depends only on what is connected to what and not on the order tiles happened to be built or
     * merged in. Two identical worlds therefore label identically, whatever schedule produced them.
     */
    CKGROUNDNAV_API auto
    DoLabel_Reachability(
        FCk_GroundNav_Field& InOutField) -> void;

    CKGROUNDNAV_API auto
    DoBake_Field(
        const ICk_GroundNav_GeometryBackend& InBackend,
        const FCk_GroundNav_FieldParams&     InParams,
        const FCk_GroundNav_Epoch&           InEpoch,
        FCk_GroundNav_Field&                 OutField) -> FCk_GroundNav_BakeStageResult;
}

// --------------------------------------------------------------------------------------------------------------------
