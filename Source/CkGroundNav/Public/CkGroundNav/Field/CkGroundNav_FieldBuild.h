#pragma once

#include "CkGroundNav/Field/CkGroundNav_Field.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    struct FCk_GroundNav_FieldBuildState;

    // ----------------------------------------------------------------------------------------------------------------

    /** Start a build. Bakes nothing — it sizes the field and sets the resume point to the first tile. */
    CKGROUNDNAV_API auto
    Request_BeginBuild(
        const FCk_GroundNav_FieldParams& InParams,
        const FCk_GroundNav_Epoch&       InEpoch,
        FCk_GroundNav_FieldBuildState&   OutState) -> FCk_GroundNav_BakeStageResult;

    /**
     * Bake tiles until the probe budget is spent or the build finishes.
     *
     * THE RESUMABLE UNIT IS THE TILE. The budget decides whether the next tile STARTS; a tile that has
     * started always runs to completion, so a slice can overshoot the budget by one tile. That is the
     * deliberate trade: a resume point inside a stage would have to checkpoint the rasterizer or the
     * distance transform, and any error in that checkpoint shows up as re-probing at the boundary —
     * which makes the probe count depend on the slice size, and a probe count that moves with the
     * schedule can no longer be asserted against a budget at all. A tile that is never split cannot
     * re-probe.
     *
     * At least one tile is baked per call, so a build always advances however small the budget.
     *
     * Returns Completed when the build finished this slice, BudgetExhausted when it paused with a
     * resume point recorded, and the underlying failure otherwise.
     */
    CKGROUNDNAV_API auto
    Request_AdvanceBuild(
        const ICk_GroundNav_GeometryBackend& InBackend,
        int32                                InProbeBudget,
        FCk_GroundNav_FieldBuildState&       InOutState) -> FCk_GroundNav_BakeStageResult;

    /**
     * The finished field, or nullptr while any slice is still outstanding.
     *
     * The only way to reach the field a build is assembling, and it refuses until the build is whole.
     * A half-baked field reads exactly like a world whose missing tiles have no floor, and every query
     * against it would be answered confidently and wrongly.
     */
    CKGROUNDNAV_API auto
    Get_CompletedField(
        const FCk_GroundNav_FieldBuildState& InState) -> const FCk_GroundNav_Field*;

    /**
     * Take the finished field out of the build state, leaving the build spent.
     *
     * Moves rather than copies: a field is the largest thing this module produces, and publishing it is
     * the one moment a copy would be paid for on the game thread. Returns null if the build is not
     * complete, on the same terms as Get_CompletedField.
     */
    CKGROUNDNAV_API auto
    Request_ReleaseCompletedField(
        FCk_GroundNav_FieldBuildState& InOutState) -> FCk_GroundNav_FieldPtr;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * A field build in progress: where it stopped, what it has spent, and the field it is accumulating.
     *
     * The accumulating field is deliberately not a public member. It is reachable only through
     * Get_CompletedField, which refuses until the build is done — the one place where hiding a member
     * earns its keep in a module whose value types are otherwise plain aggregates.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_FieldBuildState
    {
    public:
        FCk_GroundNav_FieldParams _Params;

        FCk_GroundNav_Epoch _Epoch;

        // The next tile to bake, and the resume point. Equal to the tile count once the build is done.
        int32 _NextTileIndex = 0;

        // Probes spent by the WHOLE build so far, across every slice. Deterministic for a given fixture
        // and config, which is why the budget is expressed in probes rather than in time.
        int32 _ProbesSpent = 0;

        ECk_GroundNav_BuildStatus _Status = ECk_GroundNav_BuildStatus::Unbuilt;

        // The backend's world revision as it stood when the FIRST slice ran, and the whole build's claim
        // to be a statement about ONE world. Every later slice re-reads it and the build FAILS CLOSED on a
        // mismatch: tiles baked either side of a world change disagree at their shared seam, and the seam
        // portals derived from them are the one structure with no local evidence that they are wrong — a
        // vanished portal is a permanently unwalkable doorway that every query afterwards answers
        // confidently.
        //
        // FALSE POSITIVES ARE ACCEPTED. The token is world-wide, so a door opening on the far side of the
        // level aborts a build it could not have affected. A rebuild costs one more pass over the tiles;
        // the alternative costs a portal nobody can find again.
        uint64 _GeometryRevision = 0;

        // Whether _GeometryRevision has been captured yet. Explicit rather than inferred from
        // _NextTileIndex == 0, because Request_ReleaseCompletedField resets that index to spend the build
        // — a capture condition riding on it would silently re-arm on a spent state.
        bool _HasGeometryRevision = false;

    private:
        // The export macro has to be repeated here: these are REdeclarations of the exported functions
        // above, and a friend declaration that drops it disagrees with them about dll linkage.
        friend CKGROUNDNAV_API auto Request_BeginBuild(
            const FCk_GroundNav_FieldParams&, const FCk_GroundNav_Epoch&,
            FCk_GroundNav_FieldBuildState&) -> FCk_GroundNav_BakeStageResult;

        friend CKGROUNDNAV_API auto Request_AdvanceBuild(
            const ICk_GroundNav_GeometryBackend&, int32,
            FCk_GroundNav_FieldBuildState&) -> FCk_GroundNav_BakeStageResult;

        friend CKGROUNDNAV_API auto Get_CompletedField(
            const FCk_GroundNav_FieldBuildState&) -> const FCk_GroundNav_Field*;

        friend CKGROUNDNAV_API auto Request_ReleaseCompletedField(
            FCk_GroundNav_FieldBuildState&) -> FCk_GroundNav_FieldPtr;

        FCk_GroundNav_Field _Partial;

    public:
        auto Get_IsComplete() const -> bool
        {
            return _Status == ECk_GroundNav_BuildStatus::Built &&
                   _NextTileIndex >= _Partial.Get_TileCount();
        }

        auto Get_TileCount() const -> int32 { return _Partial.Get_TileCount(); }
    };
}

// --------------------------------------------------------------------------------------------------------------------
