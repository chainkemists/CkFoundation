#pragma once

#include "CkGroundNav/Bake/CkGroundNav_BakeTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_Clearance.h"
#include "CkGroundNav/Bake/CkGroundNav_Layers.h"
#include "CkGroundNav/Bake/CkGroundNav_Plates.h"
#include "CkGroundNav/Bake/CkGroundNav_SpanField.h"
#include "CkGroundNav/Bake/CkGroundNav_Walkability.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * One crossing between two plates: where it is, how wide the agent that uses it may be, and which
     * two plates it joins.
     *
     * A portal is a contiguous run of cell pairs along one lattice boundary. Two separate doors
     * between the same pair of rooms are therefore two portals with two clearances, not one portal
     * carrying the tighter of the two.
     *
     * Portals are DERIVED on every rebuild and never patched. A stale portal is a route that no
     * longer exists, which is the one error a path consumer cannot detect for itself.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_Portal
    {
    public:
        // The plates this crossing joins. _PlateA owns the _From cells and _PlateB owns the cells
        // across the direction, so the pair says which side is which rather than only which two
        // plates touch. Every boundary is enumerated from one side only, which is what keeps that
        // orientation — and therefore the portal ordering — identical between two bakes of one input.
        int32 _PlateA = FCk_GroundNav_Plate::kNoPlate;
        int32 _PlateB = FCk_GroundNav_Plate::kNoPlate;

        // Direction index (Get_DirectionOffset) from a cell on the _From side to the cell it crosses
        // into. Only 0 (+X) and 1 (+Y) occur: every boundary is enumerated from one side only, so a
        // crossing is never emitted twice.
        int32 _Direction = 0;

        // The crossing cells on the _From side, inclusive. They vary along the one axis the direction
        // does not step along; the far side of each is that cell plus the direction offset.
        FIntPoint _FromMin = FIntPoint::ZeroValue;
        FIntPoint _FromMax = FIntPoint::ZeroValue;

        // Surface height at either end of the interval, taken midway between the two sides. A crossing
        // onto a step has one side higher than the other, and the boundary itself sits between them.
        float _MinEndZUu = 0.0f;
        float _MaxEndZUu = 0.0f;

        // The tightest point on the WIDEST crossing this portal offers: per cell pair the room is
        // whichever side is tighter, and the portal keeps the best of those, because an agent picks
        // where along the interval to cross. Taking the tightest cell pair instead would report every
        // doorway as the half-cell of room its own jamb has, and nothing would ever pass.
        float _TraversalClearanceUu = 0.0f;

    public:
        auto Get_CellCount() const -> int32
        {
            return ((_FromMax.X - _FromMin.X) + 1) * ((_FromMax.Y - _FromMin.Y) + 1);
        }

        /**
         * The two ends of the boundary segment in world space, on the shared cell edge.
         *
         * Taking the lattice origin and cell size rather than the field they came from is what lets a
         * published tile answer this: a tile keeps its portals and its own origin, and deliberately
         * does not keep the span field they were derived from.
         */
        auto Get_Endpoints(
            const FVector& InOrigin,
            float          InCellSizeUu,
            FVector&       OutMinEnd,
            FVector&       OutMaxEnd) const -> void;

        auto Get_Endpoints(
            const FCk_GroundNav_SpanField& InSpans,
            FVector&                       OutMinEnd,
            FVector&                       OutMaxEnd) const -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** Every portal in the field, plus the table that answers "what can I reach from this plate". */
    struct CKGROUNDNAV_API FCk_GroundNav_PortalField
    {
    public:
        TArray<FCk_GroundNav_Portal> _Portals;

        // One entry per plate, holding the index of every portal that plate is an end of. Both ends
        // are listed, so the table reads the same whichever side the search arrives from.
        TArray<TArray<int32>> _PlateToPortals;

    public:
        auto Get_PortalCount() const -> int32 { return _Portals.Num(); }

        auto Get_PortalsForPlate(int32 InPlateIndex) const -> TConstArrayView<int32>;

        /** The other end of the given portal, seen from the given plate. */
        auto Get_OppositePlate(int32 InPortalIndex, int32 InPlateIndex) const -> int32;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Derive the portal graph from the plates, reading adjacency from the connection field alone.
     *
     * Cross-LAYER crossings need no special case. A connection names the neighbouring SPAN rather than
     * merely a direction, so a ramp that climbs onto another layer resolves to a different plate
     * through the same lookup a flat crossing uses. Recomputing adjacency from the plate rectangles
     * instead would create a second definition of "these cells are neighbours", and the two would
     * drift the moment a filter changed.
     *
     * A crossing shorter than one cell is still emitted, carrying its true tiny clearance: the search
     * may still need it, and silently dropping it would report a dead end where there is a gap.
     *
     * Pure: no world, no registry, no physics.
     */
    CKGROUNDNAV_API auto
    DoExtract_Portals(
        const FCk_GroundNav_SpanField&       InSpans,
        const FCk_GroundNav_LayerField&      InLayers,
        const FCk_GroundNav_ConnectionField& InConnections,
        const FCk_GroundNav_PlateField&      InPlates,
        const FCk_GroundNav_ClearanceField&  InClearance,
        FCk_GroundNav_PortalField&           OutPortals) -> FCk_GroundNav_BakeStageResult;
}

// --------------------------------------------------------------------------------------------------------------------
