#pragma once

#include "CkGroundNav/Bake/CkGroundNav_GeometryBatch.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * Whether a triangle mesh is CLOSED: every edge shared by exactly two triangles, so the mesh bounds
     * a solid and has an inside.
     *
     * The bake sees faces and never an interior, so a solid resting on a floor is known to cover that
     * floor only through its underside — a body with no underside bakes as open ground and an agent
     * walks through it. Closure is the contract every Solid body owes the bake, and this is the check.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_ClosureReport
    {
    public:
        int32 _TriangleCount = 0;

        // Edges used by exactly one triangle. Zero means closed. An edge used by THREE or more is
        // non-manifold rather than open and is not counted here.
        int32 _OpenEdgeCount = 0;

        // World-space endpoints of the open edges, two per edge, capped by the caller's limit.
        TArray<FVector> _OpenEdgePoints;

    public:
        auto Get_IsClosed() const -> bool { return _OpenEdgeCount == 0; }
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Vertices closer than this are the same vertex. The batch is NOT welded, and a physics backend may
     * decode a shared vertex twice through different compression blocks, so exact comparison would
     * report a closed mesh as open. A gap wider than this is a real hole. Unreal units.
     */
    inline constexpr auto kMeshClosureWeldToleranceUu = 0.1;

    /**
     * Judge the triangles [InFirstTriangle, InFirstTriangle + InTriangleCount) of the batch as ONE mesh.
     *
     * Welds vertices within kMeshClosureWeldToleranceUu, then counts every undirected edge; an edge
     * seen once is open. Degenerate triangles (a welded edge of zero length) contribute no edge. Bills
     * one probe per triangle read. Records at most InMaxRecordedEdges open edges; _OpenEdgeCount is the
     * true total regardless.
     *
     * Pure: no world, no registry, no physics.
     */
    CKGROUNDNAV_API auto
    Get_MeshClosure(
        const FCk_GroundNav_GeometryBatch& InBatch,
        int32                              InFirstTriangle,
        int32                              InTriangleCount,
        int32                              InMaxRecordedEdges,
        int32&                             InOutProbes) -> FCk_GroundNav_ClosureReport;
}

// --------------------------------------------------------------------------------------------------------------------
