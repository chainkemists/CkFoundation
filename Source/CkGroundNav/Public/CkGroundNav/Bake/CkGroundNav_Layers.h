#pragma once

#include "CkGroundNav/Bake/CkGroundNav_BakeTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_SpanField.h"
#include "CkGroundNav/Bake/CkGroundNav_Walkability.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /** Where one span lives: its column, and its index within that column. */
    struct CKGROUNDNAV_API FCk_GroundNav_SpanAddress
    {
    public:
        int32 _X = 0;
        int32 _Y = 0;
        int32 _SpanIndex = 0;

    public:
        auto operator==(const FCk_GroundNav_SpanAddress&) const -> bool = default;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * A maximal set of walkable spans mutually reachable across the connection mask.
     *
     * A component may cover the same column more than once — a spiral ramp that climbs over its own
     * lower run is one continuous walk and one component, but two floors. That is why the footprint
     * is tracked separately from the span list.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_Component
    {
    public:
        TArray<FCk_GroundNav_SpanAddress> _Spans;

        // One bit per column, set where this component has at least one span.
        TBitArray<> _Footprint;

        // True when some column carries more than one of this component's spans, so no single layer
        // can hold it whole.
        bool _OverlapsItself = false;

    public:
        auto Get_SpanCount() const -> int32 { return _Spans.Num(); }
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Which layer each span belongs to, shaped like the span field it came from.
     *
     * Within one layer no column carries more than one span, which is what lets a query treat a layer
     * as a plain 2D grid. Non-walkable spans carry kNoLayer.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_LayerField
    {
    public:
        static constexpr int32 kNoLayer = -1;

    public:
        int32 _SizeX = 0;
        int32 _SizeY = 0;
        int32 _LayerCount = 0;

        TArray<TArray<int32>> _Columns;

    public:
        auto Get_ColumnIndex(int32 InX, int32 InY) const -> int32 { return (InY * _SizeX) + InX; }

        auto Get_Column(int32 InX, int32 InY) const -> const TArray<int32>&
        {
            return _Columns[Get_ColumnIndex(InX, InY)];
        }

        /** How many spans in the given column were assigned to the given layer. Never above one. */
        auto Get_OccupancyAt(int32 InX, int32 InY, int32 InLayer) const -> int32;

        auto Get_AssignedSpanCount() const -> int32;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Group walkable spans into connected components over the connection mask.
     *
     * Seeded in column-major scan order so component indices are reproducible from the same input.
     * Returns the number of components found.
     *
     * InOutProbes is ACCUMULATED into, never reset — one probe per seed candidacy test and per
     * neighbour the flood fill visits, in the unit FCk_GroundNav_BakeStageResult defines — so one
     * counter can be threaded through several stages.
     */
    CKGROUNDNAV_API auto
    DoFind_ConnectedComponents(
        const FCk_GroundNav_SpanField&       InSpans,
        const FCk_GroundNav_ConnectionField& InConnections,
        TArray<FCk_GroundNav_Component>&     OutComponents,
        int32&                               InOutProbes) -> int32;

    /**
     * Assign every walkable span to a layer such that no column carries two spans of one layer.
     *
     * A component is placed WHOLE into the lowest layer whose footprint it does not intersect, so
     * spans that are mutually reachable stay together — the property multi-storey queries rely on. A
     * component that overlaps ITSELF cannot satisfy that for any layer, so it alone is split span by
     * span; a spiral ramp therefore keeps its single component while its runs land on separate
     * layers.
     *
     * A span that fits nowhere OPENS A NEW LAYER. It is never dropped: an extra layer costs an index,
     * and a lost floor costs an agent.
     *
     * Pure: no world, no registry, no physics.
     */
    CKGROUNDNAV_API auto
    DoExtract_Layers(
        const FCk_GroundNav_SpanField&       InSpans,
        const FCk_GroundNav_ConnectionField& InConnections,
        FCk_GroundNav_LayerField&            OutLayers) -> FCk_GroundNav_BakeStageResult;
}

// --------------------------------------------------------------------------------------------------------------------
