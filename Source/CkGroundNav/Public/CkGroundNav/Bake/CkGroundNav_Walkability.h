#pragma once

#include "CkGroundNav/Bake/CkGroundNav_AgentProfile.h"
#include "CkGroundNav/Bake/CkGroundNav_BakeTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_SpanField.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    // The four lattice directions, in a fixed order. Four and not eight: a diagonal step crosses a
    // corner an agent's volume cannot pass through, so admitting one would let paths cut through the
    // one place the clearance field says they must not.
    inline constexpr int32 kDirectionCount = 4;

    /** Lattice offset for one direction index. Direction d and direction d+2 are opposites. */
    CKGROUNDNAV_API auto
    Get_DirectionOffset(
        int32 InDirection) -> FIntPoint;

    CKGROUNDNAV_API auto
    Get_OppositeDirection(
        int32 InDirection) -> int32;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Which span in each neighbouring column one span connects to.
     *
     * The neighbour is stored as a span INDEX rather than a flag, because a column may hold several
     * spans and "there is a connection north" does not say which floor it leads to. Layer extraction
     * walks these indices directly.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_SpanConnections
    {
    public:
        static constexpr int32 kNoConnection = -1;

    public:
        int32 _Neighbours[kDirectionCount] = {kNoConnection, kNoConnection, kNoConnection, kNoConnection};

    public:
        auto Get_IsConnected(int32 InDirection) const -> bool
        {
            return _Neighbours[InDirection] != kNoConnection;
        }

        auto Get_ConnectionCount() const -> int32;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The adjacency of a span field, shaped exactly like it: one entry per span, same column order.
     *
     * This is the ONLY adjacency the distance transform and the plate merge are permitted to
     * consult. Recomputing "are these two neighbours" anywhere downstream would create a second
     * definition of walkable, and the two would drift.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_ConnectionField
    {
    public:
        int32 _SizeX = 0;
        int32 _SizeY = 0;

        TArray<TArray<FCk_GroundNav_SpanConnections>> _Columns;

    public:
        auto Get_IsValidColumn(int32 InX, int32 InY) const -> bool
        {
            return InX >= 0 && InY >= 0 && InX < _SizeX && InY < _SizeY;
        }

        auto Get_ColumnIndex(int32 InX, int32 InY) const -> int32 { return (InY * _SizeX) + InX; }

        auto Get_Column(int32 InX, int32 InY) const -> const TArray<FCk_GroundNav_SpanConnections>&
        {
            return _Columns[Get_ColumnIndex(InX, InY)];
        }

        auto Get_MutableColumn(int32 InX, int32 InY) -> TArray<FCk_GroundNav_SpanConnections>&
        {
            return _Columns[Get_ColumnIndex(InX, InY)];
        }

        auto Get_TotalConnectionCount() const -> int32;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Demote every walkable span whose headroom is under the profile's standing height.
     *
     * Headroom is measured to the bottom of the next span up in the same column; a span with nothing
     * above it has unbounded headroom. Returns how many spans were demoted.
     *
     * InOutProbes is ACCUMULATED into, never reset — one probe per span visited, in the unit
     * FCk_GroundNav_BakeStageResult defines — so one counter can be threaded through several stages.
     */
    CKGROUNDNAV_API auto
    DoFilter_LowClearance(
        const FCk_GroundNav_AgentProfile& InProfile,
        FCk_GroundNav_SpanField&          InOutSpans,
        int32&                            InOutProbes) -> int32;

    /**
     * Demote every walkable span that drops away on enough sides to be the lip of a fall rather than
     * standable ground. How many sides is "enough" comes from the profile's ledge sensitivity.
     *
     * A side drops when the neighbouring column is inside the lattice and holds nothing between one
     * step below this height and one standing height above it. Bounding it below rather than testing
     * the height DIFFERENCE is what keeps the ground at the foot of a wall standable, and a walkway
     * beside a rising ramp with it: an agent cannot fall upward. A neighbour OUTSIDE the lattice is
     * unknown, not a drop — counting it as one would demote the border of every tile and eat the
     * seams between them.
     *
     * Demotions are decided against the state on entry and applied together, so no span's verdict
     * depends on where in the scan its neighbour happened to be visited.
     *
     * Returns how many spans were demoted.
     *
     * InOutProbes is ACCUMULATED into, never reset — one probe per neighbouring span a side-support
     * test reads, in the unit FCk_GroundNav_BakeStageResult defines — so one counter can be threaded
     * through several stages.
     */
    CKGROUNDNAV_API auto
    DoFilter_Ledges(
        const FCk_GroundNav_AgentProfile& InProfile,
        FCk_GroundNav_SpanField&          InOutSpans,
        int32&                            InOutProbes) -> int32;

    /**
     * Record adjacency between walkable spans that an agent can actually step between.
     *
     * Two spans connect when the height delta is within the step height AND their surface normals
     * are within the profile's max slope change — unless the delta is within the rough-perch
     * tolerance, which waives the normal test so that rasterization roughness does not shatter one
     * surface into fragments. A zero tolerance waives nothing.
     *
     * Connections are symmetric by construction: an edge survives only if the far span records the
     * mirror edge back. Downstream may therefore treat this as an undirected graph.
     *
     * Returns the number of directed entries recorded.
     *
     * InOutProbes is ACCUMULATED into, never reset — one probe per connection candidate visited and
     * per far-column mirror read, in the unit FCk_GroundNav_BakeStageResult defines — so one counter
     * can be threaded through several stages.
     */
    CKGROUNDNAV_API auto
    DoBuild_Connections(
        const FCk_GroundNav_AgentProfile& InProfile,
        const FCk_GroundNav_SpanField&    InSpans,
        FCk_GroundNav_ConnectionField&    OutConnections,
        int32&                            InOutProbes) -> int32;

    /**
     * The three filters in their fixed order — low clearance, then ledges, then connectivity —
     * followed by the connection mask they produce.
     *
     * The order is load-bearing: clearance and ledge demotions must both be settled before adjacency
     * is recorded, or the mask would carry edges into spans a later pass removes.
     *
     * Pure: no world, no registry, no physics.
     */
    CKGROUNDNAV_API auto
    DoFilter_Walkability(
        const FCk_GroundNav_AgentProfile& InProfile,
        FCk_GroundNav_SpanField&          InOutSpans,
        FCk_GroundNav_ConnectionField&    OutConnections) -> FCk_GroundNav_BakeStageResult;
}

// --------------------------------------------------------------------------------------------------------------------
