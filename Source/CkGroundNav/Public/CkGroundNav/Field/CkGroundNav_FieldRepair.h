#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkGroundNav/Field/CkGroundNav_Field.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    struct FCk_GroundNav_FieldRepairState;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Every tile a dirty world box can have changed the answer for: the ones its HALO-INFLATED footprint
     * reaches, in ascending index order.
     *
     * Inflated in XY by the field's own halo width, because a tile bakes against ground that far outside
     * itself — geometry that moved just beyond a tile's edge still moved that tile's clearance and its
     * ledge filtering. Z is not inflated: the halo is an XY expansion (Get_TileHaloBounds), and every
     * tile of a field spans the one vertical slab anyway.
     *
     * A tile is placed from where the FIELD PARAMS say it sits rather than from what the tile carries, so
     * a tile that never built or that failed is selected on the same terms as a built one — those are
     * exactly the tiles a repair should get another attempt at, and a failed tile carries no origin and
     * no cell count to be placed by. An invalid dirty box selects nothing.
     *
     * Pure.
     */
    CKGROUNDNAV_API auto
    Get_RepairTileIndices(
        const FCk_GroundNav_Field& InSource,
        const FBox&                InDirtyBounds) -> TArray<int32>;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Open a repair against a PUBLISHED field. Bakes nothing — it copies the field, fixes the tile set
     * the dirty box reaches, and sets the resume point to the first of them.
     *
     * The state holds its own reference to the source for the whole repair, so the publisher may swap
     * something else in mid-repair without pulling the ground out from under it.
     *
     * THE REPAIR BAKES UNDER THE MARKUP RECORDS THE CALLER HOLDS NOW, never the ones the source field
     * was built with. A published field's records are the ones its last PUBLISH was made under, and a
     * paint admitted since then is held on the volume and nowhere else — so a repair reading them off
     * the source would answer a markup change with the very records that change replaced. Everything
     * else in the source's params is kept exactly as it is: it is the only account of where the tiles
     * sit and what the carried-over tiles were produced under, and a repair handed fresh params would
     * publish two lattices in one field with nothing downstream able to reconcile them.
     *
     * A dirty box that reaches NO tile is not an error: the repair completes here, holding a copy of the
     * source with every epoch untouched. A paint outside every tile is still a request the caller made
     * and is owed an answer to, and the honest answer is that no ground changed — the same answer the
     * cost derive gives when nothing it restamped moved.
     */
    CKGROUNDNAV_API auto
    Request_BeginRepair(
        FCk_GroundNav_FieldRepairState&             OutState,
        const FCk_GroundNav_FieldPtr&               InSource,
        const FBox&                                 InDirtyBounds,
        const FCk_GroundNav_Epoch&                  InEpoch,
        TConstArrayView<FCk_GroundNav_MarkupRecord> InCurrentMarkupRecords) -> FCk_GroundNav_BakeStageResult;

    /**
     * Re-bake repair tiles until the probe budget is spent or the repair finishes.
     *
     * THE RESUMABLE UNIT IS THE TILE, on the same terms and for the same reason as a build's: the budget
     * decides whether the next tile STARTS, at least one tile runs per call, and a tile is never split.
     * The tile SET is fixed at Begin, which is what makes a sliced repair produce the field a one-shot
     * repair produces however the slices fell.
     *
     * Returns Completed when the repair finished this slice, BudgetExhausted when it paused with a
     * resume point recorded, and the underlying failure otherwise.
     */
    CKGROUNDNAV_API auto
    Request_AdvanceRepair(
        const ICk_GroundNav_GeometryBackend& InBackend,
        int32                                InProbeBudget,
        FCk_GroundNav_FieldRepairState&      InOutState) -> FCk_GroundNav_BakeStageResult;

    /**
     * The repaired field, or nullptr while any slice is still outstanding.
     *
     * Refuses until the repair is whole, on the same terms as Get_CompletedField: a field whose tiles
     * disagree about which world they describe answers every query afterwards confidently and wrongly.
     */
    CKGROUNDNAV_API auto
    Get_RepairedField(
        const FCk_GroundNav_FieldRepairState& InState) -> FCk_GroundNav_FieldPtr;

    /**
     * Take the repaired field out of the state, leaving the repair spent.
     *
     * Moves rather than copies, and drops the source reference with it: a spent repair still holding the
     * field it opened against would pin the largest thing this module produces for as long as the state
     * lives. Returns null unless the repair is complete, on the same terms as Get_RepairedField.
     */
    CKGROUNDNAV_API auto
    Request_ReleaseRepairedField(
        FCk_GroundNav_FieldRepairState& InOutState) -> FCk_GroundNav_FieldPtr;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * A LOCAL REPAIR in flight: the published field it opened against, the copy it is re-baking into, and
     * where it stopped.
     *
     * THE CONTRACT:
     *
     * 1. The published source is NEVER mutated. The repair assembles a whole new field value and hands it
     *    over, so a query walking the published one keeps walking a consistent structure for as long as
     *    it holds the pointer. That is the publish discipline a build already answers under, and it is
     *    what makes a repair unable to corrupt anything without a lock discipline.
     *
     * 2. Every tile OUTSIDE the halo-inflated dirty box is carried across byte for byte. Nothing is
     *    re-probed for it and its epoch does not move.
     *
     * 3. The field-level passes are RE-DERIVED IN FULL, never patched: the seam portals, the tile edge
     *    boundary, the reachability labels and the open-body report are all recomputed over the whole
     *    tile set. A seam portal that outlived the tile it crossed into is the one error a path consumer
     *    cannot detect for itself, and a plate label that survived a rebuild by luck is the other.
     *
     * Together those give the property the repair exists for: the result equals a FULL BAKE of the same
     * scene byte for byte, tile epochs excluded. Only the re-baked tiles carry the new epoch, which is
     * what lets Get_ChangedTileBounds report exactly the ground the repair touched and nothing besides.
     *
     * The accumulating field is deliberately unreachable except through Get_RepairedField, which refuses
     * until the repair is whole — the same reason a build hides its own partial.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_FieldRepairState
    {
    public:
        CK_GENERATED_BODY(FCk_GroundNav_FieldRepairState);

    private:
        // The export macro has to be repeated here: these are REdeclarations of the exported functions
        // above, and a friend declaration that drops it disagrees with them about dll linkage.
        friend CKGROUNDNAV_API auto Request_BeginRepair(
            FCk_GroundNav_FieldRepairState&, const FCk_GroundNav_FieldPtr&,
            const FBox&, const FCk_GroundNav_Epoch&,
            TConstArrayView<FCk_GroundNav_MarkupRecord>) -> FCk_GroundNav_BakeStageResult;

        friend CKGROUNDNAV_API auto Request_AdvanceRepair(
            const ICk_GroundNav_GeometryBackend&, int32,
            FCk_GroundNav_FieldRepairState&) -> FCk_GroundNav_BakeStageResult;

        friend CKGROUNDNAV_API auto Get_RepairedField(
            const FCk_GroundNav_FieldRepairState&) -> FCk_GroundNav_FieldPtr;

        friend CKGROUNDNAV_API auto Request_ReleaseRepairedField(
            FCk_GroundNav_FieldRepairState&) -> FCk_GroundNav_FieldPtr;

    private:
        // Held for the whole repair and never written through. Its params are also the only account of
        // where the tiles sit and what they were baked under, so the repair re-bakes under exactly the
        // config the untouched tiles were produced with — a repair handed fresh params would publish two
        // lattices in one field, and nothing downstream could reconcile them. The markup records are the
        // one part NOT taken from here, for the reason Request_BeginRepair states.
        FCk_GroundNav_FieldPtr _Source;

        TSharedPtr<FCk_GroundNav_Field> _Repaired;

        FBox _DirtyBounds = FBox{ForceInit};

        // Ascending, and FIXED AT BEGIN. A set recomputed per slice would move under a world that kept
        // changing, and the slices either side of that move would disagree about which tiles are current.
        TArray<int32> _RepairTileIndices;

        int32 _NextRepairCursor = 0;

        FCk_GroundNav_Epoch _Epoch;

        int32 _ProbesSpent = 0;

        // The backend's world revision as it stood when the FIRST slice ran, re-read per slice AND per
        // tile, failing the repair closed on a mismatch. Same contract and same accepted false positives
        // as a sliced build's: tiles produced either side of a world change disagree at their shared
        // seam, and the seam portal derived from them is the one structure with no local evidence that
        // it is wrong.
        uint64 _GeometryRevision = 0;

        // Explicit rather than inferred from the cursor, because releasing the field resets that cursor
        // to spend the state — a capture condition riding on it would silently re-arm.
        bool _HasGeometryRevision = false;

        // Bodies the closure pass has already fetched and judged. Cleared before that pass runs, so a
        // body's whole mesh is read once and _OpenBodies comes out in the same first-touch order a full
        // bake produces.
        TSet<uint64> _CheckedBodies;

        ECk_GroundNav_BuildStatus _Status = ECk_GroundNav_BuildStatus::Unbuilt;

    public:
        CK_PROPERTY_GET(_Source);
        CK_PROPERTY_GET(_DirtyBounds);
        CK_PROPERTY_GET(_RepairTileIndices);
        CK_PROPERTY_GET(_Epoch);
        CK_PROPERTY_GET(_ProbesSpent);
        CK_PROPERTY_GET(_GeometryRevision);
        CK_PROPERTY_GET(_Status);

    public:
        auto Get_IsRepairing() const -> bool
        {
            return _Repaired.IsValid() && _Status == ECk_GroundNav_BuildStatus::Unbuilt;
        }

        auto Get_IsBuilt() const -> bool
        {
            return _Repaired.IsValid() && _Status == ECk_GroundNav_BuildStatus::Built;
        }

        auto Get_RepairTileCount() const -> int32 { return _RepairTileIndices.Num(); }

        auto Get_RemainingTileCount() const -> int32
        {
            return FMath::Max(0, _RepairTileIndices.Num() - _NextRepairCursor);
        }
    };
}

// --------------------------------------------------------------------------------------------------------------------
