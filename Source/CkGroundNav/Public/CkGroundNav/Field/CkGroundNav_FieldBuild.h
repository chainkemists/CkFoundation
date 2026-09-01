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
