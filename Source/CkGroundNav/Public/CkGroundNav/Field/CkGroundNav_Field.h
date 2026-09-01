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

        FCk_GroundNav_Epoch _Epoch;

    public:
        auto Get_TileCount() const -> int32 { return _Tiles.Num(); }

        auto Get_Tile(const FCk_GroundNav_TileCoord& InCoord) const -> const FCk_GroundNav_Tile*;

        auto Get_TileAt(const FVector& InWorldPosition) const -> const FCk_GroundNav_Tile*;

        auto Get_BuiltTileCount() const -> int32;

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
    CKGROUNDNAV_API auto
    DoBake_Field(
        const ICk_GroundNav_GeometryBackend& InBackend,
        const FCk_GroundNav_FieldParams&     InParams,
        const FCk_GroundNav_Epoch&           InEpoch,
        FCk_GroundNav_Field&                 OutField) -> FCk_GroundNav_BakeStageResult;
}

// --------------------------------------------------------------------------------------------------------------------
