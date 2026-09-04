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
     * Start a build that produces ONE FIELD PER AGENT PROFILE out of a single pass over the geometry.
     *
     * The entries must differ ONLY in their _Profile: they share the lattice every tile is placed on,
     * the config it bakes under, the markup records it is priced with and the links it resolves. That
     * shared lattice is what lets one geometry collection per tile feed every profile, so it is
     * ENFORCED rather than assumed — a list disagreeing on anything else is refused at admission naming
     * the field it disagreed on. N fields baked on N lattices behind one tile index would each be
     * internally consistent and jointly meaningless, which is the failure FCk_GroundNav_FieldParams
     * already refuses to make representable across two tiles of one field.
     *
     * ALL-OR-NOTHING across profiles. An invalid entry, or one whose agent profile
     * Get_ProfileRejection refuses, terminates the whole build at admission rather than baking the
     * profiles that would have succeeded: a field that exists for one profile and not another is a hole
     * nothing downstream can detect by looking at the fields it does have.
     *
     * Request_BeginBuild is this with a one-element list, so one profile is the N = 1 case here rather
     * than a second implementation.
     */
    CKGROUNDNAV_API auto
    Request_BeginBuild_MultiProfile(
        TConstArrayView<FCk_GroundNav_FieldParams> InParams,
        const FCk_GroundNav_Epoch&                 InEpoch,
        FCk_GroundNav_FieldBuildState&             OutState) -> FCk_GroundNav_BakeStageResult;

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
     * ONE TILE IS ONE GEOMETRY COLLECTION, whatever the profile count. The slice collects the tile's
     * halo geometry once, judges the bodies in it once, bakes that one batch under each profile in
     * turn, and advances the resume point once — so every profile is always at the same tile and no two
     * of them can end up baked against different worlds. Probes SUM across profiles, because each
     * profile's tile bake spends its own cell and span reads; _GeometryFetches is what counts the
     * collections.
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
     * The FIRST profile's finished field, or nullptr while any slice is still outstanding.
     *
     * The only way to reach the field a build is assembling, and it refuses until the build is whole.
     * A half-baked field reads exactly like a world whose missing tiles have no floor, and every query
     * against it would be answered confidently and wrongly.
     */
    CKGROUNDNAV_API auto
    Get_CompletedField(
        const FCk_GroundNav_FieldBuildState& InState) -> const FCk_GroundNav_Field*;

    /**
     * Every profile's finished field, in the order their params were submitted, or an empty view while
     * any slice is still outstanding — on the same terms and for the same reason as Get_CompletedField,
     * of which this is the plural.
     */
    CKGROUNDNAV_API auto
    Get_CompletedFields(
        const FCk_GroundNav_FieldBuildState& InState) -> TConstArrayView<FCk_GroundNav_Field>;

    /**
     * Take the FIRST profile's finished field out of the build state, leaving the build spent.
     *
     * Moves rather than copies: a field is the largest thing this module produces, and publishing it is
     * the one moment a copy would be paid for on the game thread. Returns null if the build is not
     * complete, on the same terms as Get_CompletedField.
     *
     * A multi-profile build is released through Request_ReleaseCompletedFields. Releasing one through
     * this hands back the first profile's field and spends the build, so the others go with it.
     */
    CKGROUNDNAV_API auto
    Request_ReleaseCompletedField(
        FCk_GroundNav_FieldBuildState& InOutState) -> FCk_GroundNav_FieldPtr;

    /**
     * Take every profile's finished field out of the build state, index-aligned with the params the
     * build began with, leaving the build spent. Empty unless the build is complete.
     *
     * The plural is the primitive: a single-profile build releases a one-element array, and
     * Request_ReleaseCompletedField is that array's first element.
     */
    CKGROUNDNAV_API auto
    Request_ReleaseCompletedFields(
        FCk_GroundNav_FieldBuildState& InOutState) -> TArray<FCk_GroundNav_FieldPtr>;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * A field build in progress: where it stopped, what it has spent, and the fields it is accumulating.
     *
     * The accumulating fields are deliberately not a public member. They are reachable only through
     * Get_CompletedField(s), which refuse until the build is done — the one place where hiding a member
     * earns its keep in a module whose value types are otherwise plain aggregates.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_FieldBuildState
    {
    public:
        // One entry per agent profile, index-aligned with the fields being accumulated and differing
        // only in _Profile. A single-profile build holds exactly one.
        TArray<FCk_GroundNav_FieldParams> _Params;

        FCk_GroundNav_Epoch _Epoch;

        // The next tile to bake, and the resume point. Equal to the tile count once the build is done.
        // ONE index for every profile: they advance together, which is what stops one tile being baked
        // against two different worlds by two profiles.
        int32 _NextTileIndex = 0;

        // Probes spent by the WHOLE build so far, across every slice and every profile. Deterministic
        // for a given fixture and config, which is why the budget is expressed in probes rather than in
        // time.
        int32 _ProbesSpent = 0;

        // Tiles this build has collected geometry FOR, across every slice. ONE per tile however many
        // profiles bake out of that collection, and the only place the "one collection feeds every
        // profile" claim is observable: a probe is a cell or span read INSIDE a tile's bake, so N
        // profiles spend N times as many of them whether the geometry was collected once or N times.
        int32 _GeometryFetches = 0;

        ECk_GroundNav_BuildStatus _Status = ECk_GroundNav_BuildStatus::Unbuilt;

        // The backend's world revision as it stood when the FIRST slice ran, and the whole build's claim
        // to be a statement about ONE world. Every later slice re-reads it and the build FAILS CLOSED on a
        // mismatch: tiles baked either side of a world change disagree at their shared seam, and the seam
        // portals derived from them are the one structure with no local evidence that they are wrong — a
        // vanished portal is a permanently unwalkable doorway that every query afterwards answers
        // confidently.
        //
        // FALSE POSITIVES ARE ACCEPTED. The token is world-wide, so a static body streaming in on the far
        // side of the level aborts a build it could not have affected. A rebuild costs one more pass over
        // the tiles; the alternative costs a portal nobody can find again. It is STATIC-ONLY, though: the
        // bake reads static bodies and nothing else, and a token that moved for every despawned projectile
        // would starve a multi-frame build in any live scene.
        uint64 _GeometryRevision = 0;

        // Whether _GeometryRevision has been captured yet. Explicit rather than inferred from
        // _NextTileIndex == 0, because Request_ReleaseCompletedField resets that index to spend the build
        // — a capture condition riding on it would silently re-arm on a spent state.
        bool _HasGeometryRevision = false;

        // Bodies the closure check has already fetched and judged this build. A body straddles many
        // tiles' halos; its whole mesh is read once, not once per tile.
        TSet<uint64> _CheckedBodies;

        // What that check found. Held for the BUILD rather than per profile and copied onto every
        // profile's field at composition: closure is a property of the geometry, which every profile
        // bakes out of one collection of, so N profiles must not turn one bad asset into N reports.
        TArray<FCk_GroundNav_OpenBody> _OpenBodies;

    private:
        // The export macro has to be repeated here: these are REdeclarations of the exported functions
        // above, and a friend declaration that drops it disagrees with them about dll linkage.
        friend CKGROUNDNAV_API auto Request_BeginBuild_MultiProfile(
            TConstArrayView<FCk_GroundNav_FieldParams>, const FCk_GroundNav_Epoch&,
            FCk_GroundNav_FieldBuildState&) -> FCk_GroundNav_BakeStageResult;

        friend CKGROUNDNAV_API auto Request_AdvanceBuild(
            const ICk_GroundNav_GeometryBackend&, int32,
            FCk_GroundNav_FieldBuildState&) -> FCk_GroundNav_BakeStageResult;

        friend CKGROUNDNAV_API auto Get_CompletedFields(
            const FCk_GroundNav_FieldBuildState&) -> TConstArrayView<FCk_GroundNav_Field>;

        friend CKGROUNDNAV_API auto Request_ReleaseCompletedFields(
            FCk_GroundNav_FieldBuildState&) -> TArray<FCk_GroundNav_FieldPtr>;

        TArray<FCk_GroundNav_Field> _Partial;

    public:
        auto Get_ProfileCount() const -> int32 { return _Partial.Num(); }

        auto Get_IsComplete() const -> bool
        {
            return _Status == ECk_GroundNav_BuildStatus::Built &&
                   NOT _Partial.IsEmpty() &&
                   _NextTileIndex >= _Partial[0].Get_TileCount();
        }

        // Read off the first profile's field: every profile shares one lattice by admission, so the
        // tile count is the build's rather than any one profile's.
        auto Get_TileCount() const -> int32
        {
            return _Partial.IsEmpty() ? 0 : _Partial[0].Get_TileCount();
        }
    };
}

// --------------------------------------------------------------------------------------------------------------------
