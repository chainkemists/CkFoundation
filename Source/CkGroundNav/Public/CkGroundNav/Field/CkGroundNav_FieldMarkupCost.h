#pragma once

#include "CkGroundNav/Bake/CkGroundNav_BakeTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_MarkupTypes.h"
#include "CkGroundNav/Field/CkGroundNav_Field.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * The world box one tile's published cells cover, reconstructed from what the tile carries plus
     * the field's vertical slab.
     *
     * The tile knows its origin and its cell count, and the field knows the Z extent every one of its
     * tiles spans — so nothing here re-derives a tile's position from its coordinate, and a tile that
     * moved would move its own bounds with it. Shared rather than file-local because the cost derive
     * and the live-markup probe both ask which tiles a record's footprint reaches, and two answers to
     * that would put a markup live on ground it never priced.
     */
    CKGROUNDNAV_API auto
    Get_TileWorldBounds(
        const FCk_GroundNav_FieldParams& InParams,
        const FCk_GroundNav_Tile&        InTile) -> FBox;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * A new field that differs from the one given only in what its plates COST.
     *
     * The published field is never touched. Its value is copied, the copy's plates are restamped, and
     * the caller swaps the pointer — which is the same contract a rebuild answers under, and the
     * reason a reader mid-query cannot see a half-changed world. Patching the published field instead
     * would make corruption representable, which is the property this representation exists to avoid.
     *
     * READS NO CELL, NO SPAN AND NO GEOMETRY. Cost is a plate label, not a shape: nothing about which
     * ground is walkable, how much room it has, or where a crossing lies depends on what it costs to
     * walk. So there is nothing here to rasterize, filter, decompose or extract, and every array but
     * the plates' policy fields is carried across untouched. _ProbesSpent is therefore exactly ZERO by
     * construction rather than merely small — a derive that spent probes would be a bake wearing a
     * cheaper name, and a caller could not tell the two apart from the result.
     *
     * EVERY BUILT TILE IS RESTAMPED FROM THE WHOLE LIST, and the list is the only account of what the
     * field costs. Stamping is pure per-plate work over rectangles the tile already carries, so
     * restamping a tile no record reaches costs the same nothing as deciding not to — and it is what
     * makes the answer depend on the records that EXIST rather than on which of them happen to be
     * remembered. A pass that restamped only the tiles a listed record touches cannot see a record
     * that was deleted outright: that record names no tile, so the ground it priced would keep its
     * price forever. There is therefore no contract on the caller to keep a switched-off record in
     * the list; disabling one and deleting it converge on the same field.
     *
     * A tile takes InEpoch when its plates' policy fields — _AreaPolicyIndex, _CostMultiplier and the
     * tile's interned _AreaPolicies — actually CHANGED, compared exactly, OR when an ENABLED record
     * this tile has not yet observed reaches its world bounds — one whose _RequestedAtEpoch is at or
     * past the tile's own epoch, since that stamp is the epoch the field was published at when the
     * record was admitted. The field takes InEpoch when at least one tile did.
     *
     * The second half is what lets a record that moves no label become live. Liveness asks for a tile
     * epoch strictly PAST the record's stamp, so a duplicate paint, or one over ground already at that
     * policy, would otherwise wait forever on a publish that had nothing to change. A restamp that
     * neither moves a label nor answers a pending record still moves nothing, so a reader diffing
     * epochs still learns exactly which ground has news.
     *
     * Pure: no world, no registry, no physics.
     */
    CKGROUNDNAV_API auto
    Get_FieldWithMarkupCost(
        const FCk_GroundNav_Field&                  InField,
        TConstArrayView<FCk_GroundNav_MarkupRecord> InMarkups,
        const FCk_GroundNav_Epoch&                  InEpoch)
        -> TPair<FCk_GroundNav_FieldPtr, FCk_GroundNav_BakeStageResult>;
}

// --------------------------------------------------------------------------------------------------------------------
