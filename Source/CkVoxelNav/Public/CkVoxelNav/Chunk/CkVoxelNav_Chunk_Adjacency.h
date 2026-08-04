#pragma once

#include "CkVoxelNav/Chunk/CkVoxelNav_Chunk_Types.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Types.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Baking the adjacency table: which chunks share a face, and which of their boundary cells a route can
// actually cross between.
//
// The shape is upstream's - AABB prefilter, per-axis face-sharing test, then boundary-cell matching across
// the shared plane - with the identity problem fixed (chunk ids, never actor pointers) and the pairing
// arithmetic expressed against this octree's cell seam rather than its own leaf/node split.
//
// COMPLEXITY: the chunk-pair loop is O(chunks^2) and the cell pairing inside a sharing pair is O(cells on
// A's face x cells on B's face). Both are bake-time, once per volume aggregation, over the chunk counts a
// partitioned volume actually produces - the prefilter is what keeps the first from mattering and the
// per-face restriction is what keeps the second from mattering.
//
// Derived from Nav3D 2.0, (c) 2025 Darby Costello, MIT.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    /** The six faces of a chunk's authored box, as a bit per face. The order matches
     *  GNeighbourDirections, so a face and the octree's neighbour direction that leaves through it share an
     *  index. */
    enum class EChunkFace : uint8
    {
        MaxX = 1 << 0,
        MinX = 1 << 1,
        MaxY = 1 << 2,
        MinY = 1 << 3,
        MaxZ = 1 << 4,
        MinZ = 1 << 5
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** A free, addressable cell of one chunk's octree that reaches at least one of the chunk's own faces.
     *  Carries its geometry so the pairing pass never has to re-resolve an address. */
    struct CKVOXELNAV_API FChunkBoundaryCell
    {
        FNodeAddress _Address;
        FVector _Center = FVector::ZeroVector;
        float _Extent = 0.0f;
        uint8 _FaceMask = 0;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** One chunk as the adjacency bake sees it: a stable id, the AUTHORED sub-box the partition gave it,
     *  and a borrowed pointer to its published octree. The pointer is borrowed for the duration of the bake
     *  only - it never reaches the table. */
    struct CKVOXELNAV_API FChunkAdjacencyInput
    {
        FChunkId _ChunkId;
        FBox _Bounds = FBox{ForceInit};
        const FOctree* _Octree = nullptr;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** Every free cell of the octree that reaches a face of InChunkBounds.
     *
     *  A cell here is what the search's cell seam addresses: a node at any layer above 0 that was never
     *  subdivided, a fully-free leaf as one whole cell, and every free sub-node of a partly-occluded leaf.
     *  The bounds tested against are the chunk's AUTHORED box, not the octree's navigation cube - the cube
     *  is snapped up to a power of two and its faces are nowhere near the chunk's. */
    CKVOXELNAV_API auto
    Get_ChunkBoundaryCells(
        const FOctree& InOctree,
        const FBox& InChunkBounds,
        TArray<FChunkBoundaryCell>& OutCells) -> void;

    /** The face of A (as an EChunkFace bit) whose plane the two boxes share, or 0 when they share none.
     *  Tolerance is 1.5 leaf-node sizes, upstream's figure: partitioned chunks abut exactly, but a caller
     *  may hand in chunks authored with a gap and a route still has to cross it. */
    CKVOXELNAV_API auto
    Get_SharedFace(
        const FBox& InBoundsA,
        const FBox& InBoundsB,
        float InLeafNodeSizeUu) -> uint8;

    CKVOXELNAV_API auto
    Get_FaceAxis(
        uint8 InFace) -> int32;

    CKVOXELNAV_API auto
    Get_FacePlaneValue(
        const FBox& InBounds,
        uint8 InFace) -> double;

    CKVOXELNAV_API auto
    Get_OppositeFace(
        uint8 InFace) -> uint8;

    /** Do two cells on opposite sides of a plane actually meet? Both must reach within InMaxGapUu of the
     *  plane AND their projections onto it must overlap - a cell that merely sits near the plane somewhere
     *  else on the face is not a crossing. */
    CKVOXELNAV_API auto
    Get_CellsConnectAcrossPlane(
        const FChunkBoundaryCell& InCellA,
        const FChunkBoundaryCell& InCellB,
        double InPlaneValue,
        int32 InPlaneAxis,
        double InMaxGapUu) -> bool;

    // ----------------------------------------------------------------------------------------------------------------

    /** Bakes the whole table for one volume's chunks. Chunks with no published octree contribute nothing
     *  and are not an error: a volume aggregates only once every chunk has published, so an unpublished
     *  chunk here means the caller asked early. */
    CKVOXELNAV_API auto
    Bake_ChunkAdjacency(
        const TArray<FChunkAdjacencyInput>& InChunks) -> FChunkAdjacencyTable;
}

// --------------------------------------------------------------------------------------------------------------------
