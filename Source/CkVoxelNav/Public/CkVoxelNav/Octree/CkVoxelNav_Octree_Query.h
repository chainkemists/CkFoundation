#pragma once

#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Types.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Read-only queries over a built octree: position <-> address, cell extents, neighbour enumeration, and
// the kind-dispatching cell seam that search addresses.
// --------------------------------------------------------------------------------------------------------------------

enum class ECk_VoxelNav_SubNodePrecision : uint8
{
    NodeCenter,
    SubNodeCenter
};

// --------------------------------------------------------------------------------------------------------------------

enum class ECk_VoxelNav_AddressLookup : uint8
{
    Found,
    FreeSpaceAtLayer,
    OutOfBounds,
    NoLayers
};

/** Outcome of resolving a world position to a node.
 *
 *  `FreeSpaceAtLayer` is a first-class outcome, not a degenerate address: a position inside the volume
 *  but inside no node is genuinely open space, and its identity is the layer plus the FULL-WIDTH Morton
 *  code of the cell it falls in. Packing that code into an address field cannot work - the sub-node
 *  field is six bits wide and would truncate it. */
struct CKVOXELNAV_API FCk_VoxelNav_AddressLookupResult
{
    ECk_VoxelNav_AddressLookup _Outcome = ECk_VoxelNav_AddressLookup::OutOfBounds;

    // Valid only when _Outcome == Found.
    ck::voxelnav::FNodeAddress _Address;

    // Meaningful only when _Outcome == FreeSpaceAtLayer.
    ck::voxelnav::LayerIndex _Layer = 0;
    ck::voxelnav::MortonCode _Morton = 0;
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    CKVOXELNAV_API auto
    Get_LayerNodeSize(
        const FOctree& InOctree,
        LayerIndex InLayerIndex) -> float;

    CKVOXELNAV_API auto
    Get_LayerNodeExtent(
        const FOctree& InOctree,
        LayerIndex InLayerIndex) -> float;

    CKVOXELNAV_API auto
    Get_LayerRatio(
        const FOctree& InOctree,
        LayerIndex InLayerIndex) -> float;

    CKVOXELNAV_API auto
    Get_LayerInverseRatio(
        const FOctree& InOctree,
        LayerIndex InLayerIndex) -> float;

    // ----------------------------------------------------------------------------------------------------------------

    CKVOXELNAV_API auto
    Get_LeafNodePositionFromMorton(
        const FOctree& InOctree,
        MortonCode InMortonCode) -> FVector;

    CKVOXELNAV_API auto
    Get_NodePositionFromLayerAndMorton(
        const FOctree& InOctree,
        LayerIndex InLayerIndex,
        MortonCode InMortonCode) -> FVector;

    CKVOXELNAV_API auto
    Get_NodePositionFromAddress(
        const FOctree& InOctree,
        const FNodeAddress& InAddress,
        ECk_VoxelNav_SubNodePrecision InPrecision) -> FVector;

    CKVOXELNAV_API auto
    Get_NodeExtentFromAddress(
        const FOctree& InOctree,
        const FNodeAddress& InAddress) -> float;

    // ----------------------------------------------------------------------------------------------------------------

    /** Binary search over a layer's Morton-ordered nodes; INDEX_NONE when the code names no node, which
     *  is the octree's encoding of "this cell was never subdivided", i.e. free space. */
    CKVOXELNAV_API auto
    Get_NodeIndexFromMorton(
        const FOctree& InOctree,
        LayerIndex InLayerIndex,
        MortonCode InMortonCode) -> NodeIndex;

    CKVOXELNAV_API auto
    TryGet_NodeAddressFromMorton(
        const FOctree& InOctree,
        MortonCode InMortonCode,
        LayerIndex InLayerIndex) -> FNodeAddress;

    /** nullptr when the address names no node. Never hands back a shared placeholder node - a caller
     *  cannot tell one of those apart from a real node, and every read off it is silently wrong. */
    CKVOXELNAV_API auto
    TryGet_NodeFromAddress(
        const FOctree& InOctree,
        const FNodeAddress& InAddress) -> const FNode*;

    // ----------------------------------------------------------------------------------------------------------------

    CKVOXELNAV_API auto
    TryGet_NodeAddressFromPosition(
        const FOctree& InOctree,
        const FVector& InPosition,
        LayerIndex InMinLayerIndex) -> FCk_VoxelNav_AddressLookupResult;

    /** COMPLEXITY: O(layers x nodes x 64) - a linear scan of the whole octree, with no acceleration
     *  structure behind it. It sits on the endpoint-resolution path of every path request whose endpoint
     *  lands in a blocked cell, so treat a hot profile here as expected, not as a surprise. */
    CKVOXELNAV_API auto
    TryGet_NearestFreeNodeAddress(
        const FOctree& InOctree,
        const FVector& InPosition,
        LayerIndex InMinLayerIndex) -> FCk_VoxelNav_AddressLookupResult;

    CKVOXELNAV_API auto
    Get_MinLayerIndexForAgentRadius(
        const FOctree& InOctree,
        float InAgentRadiusUu) -> LayerIndex;

    CKVOXELNAV_API auto
    Get_IsNodeInBounds(
        const FVector& InNodePosition,
        float InNodeExtent,
        const FBox& InBounds) -> bool;

    /** Is the finest cell containing this position navigable? EXACT, and deliberately distinct from
     *  TryGet_NodeAddressFromPosition, which SNAPS an occluded position to the nearest free sub-node of
     *  the same leaf and therefore answers `Found` for a point buried inside geometry - correct for
     *  resolving a path endpoint, wrong for asking whether a point is free.
     *
     *  A position outside the octree's navigation bounds is NOT free: the bake makes no statement about
     *  space it never rasterized, and reporting free there would let a path leave the volume. */
    CKVOXELNAV_API auto
    Get_IsPositionFree(
        const FOctree& InOctree,
        const FVector& InPosition) -> bool;

    // ----------------------------------------------------------------------------------------------------------------

    CKVOXELNAV_API auto
    Get_NodeNeighbours(
        const FOctree& InOctree,
        const FNodeAddress& InAddress,
        TArray<FNodeAddress>& OutNeighbours) -> void;

    CKVOXELNAV_API auto
    Get_LeafNeighbours(
        const FOctree& InOctree,
        const FNodeAddress& InLeafAddress,
        TArray<FNodeAddress>& OutNeighbours) -> void;

    CKVOXELNAV_API auto
    Get_FreeNodesUnderAddress(
        const FOctree& InOctree,
        const FNodeAddress& InAddress,
        TArray<FNodeAddress>& OutFreeNodes) -> void;

    CKVOXELNAV_API auto
    TryGet_RandomFreePoint(
        const FOctree& InOctree,
        FRandomStream& InOutStream) -> TOptional<FVector>;

    // ----------------------------------------------------------------------------------------------------------------

    /** The cell seam. Search-facing code asks the octree about a cell through these and never unpacks an
     *  FCellId itself, so a coarser cell representation is introduced by extending the kind dispatch here
     *  and nowhere else.
     *
     *  When the octree carries a merged table, MERGED cells are the currency: an endpoint resolves to one,
     *  neighbour enumeration hands them back for octree-node cells too, and a route is a chain of them.
     *  Node cells stay addressable throughout - a portal address baked before merging still answers for its
     *  centre and extent - but nothing a search walks addresses one. */
    CKVOXELNAV_API auto
    Get_CellCenter(
        const FOctree& InOctree,
        const FCellId& InCell) -> FVector;

    CKVOXELNAV_API auto
    Get_CellBounds(
        const FOctree& InOctree,
        const FCellId& InCell) -> FBox;

    CKVOXELNAV_API auto
    Get_CellExtent(
        const FOctree& InOctree,
        const FCellId& InCell) -> float;

    CKVOXELNAV_API auto
    Get_CellIsFree(
        const FOctree& InOctree,
        const FCellId& InCell) -> bool;

    CKVOXELNAV_API auto
    Get_CellNeighbors(
        const FOctree& InOctree,
        const FCellId& InCell,
        TArray<FCellId>& OutNeighbors) -> void;

    /** Resolves a world position to the cell a search should stand in - the merged cell when the octree
     *  carries a merged table, the octree-node cell otherwise. An invalid cell means the position resolves
     *  to no free cell at all, which is what an out-of-bounds or unbaked endpoint answers. */
    CKVOXELNAV_API auto
    TryGet_CellAtPosition(
        const FOctree& InOctree,
        FVolumeId InVolume,
        const FVector& InPosition,
        LayerIndex InMinLayerIndex) -> FCellId;

    /** The points a route has to pass through between two consecutive cells, appended in order.
     *
     *  Two adjacent OCTREE cells need none: the segment between their centres provably stays inside their
     *  union, because a finer cell on a coarser cell's face lies within that face's span and the lateral
     *  drift from the finer cell's centre to the shared plane is therefore less than its own half-extent.
     *  Two MERGED boxes carry no such property - they can share a small patch of a large face - so the
     *  centre of the shared face is emitted, and the route becomes centre -> face -> centre, each leg
     *  inside one convex box. */
    CKVOXELNAV_API auto
    Get_CellTransitionPoints(
        const FOctree& InOctree,
        const FCellId& InFrom,
        const FCellId& InTo,
        TArray<FVector>& OutPoints) -> void;
}

// --------------------------------------------------------------------------------------------------------------------
