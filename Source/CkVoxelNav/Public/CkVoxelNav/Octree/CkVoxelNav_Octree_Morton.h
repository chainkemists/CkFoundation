#pragma once

#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Types.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Morton (Z-order) addressing for the octree lattice. A cell's Morton code interleaves its lattice
// coordinates bit by bit, which makes a node's parent a 3-bit right shift and its eight children a
// contiguous run starting at a 3-bit left shift - that is the whole reason the SVO stores codes rather
// than coordinates.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    // A 64-bit Morton code interleaves 21 bits per axis.
    inline constexpr int32 GMaxMortonCoord = 0x1FFFFF;

    /** Interleaves non-negative lattice coordinates into a Morton code. The encoder's domain is
     *  [0, GMaxMortonCoord] per axis and inputs are CLAMPED into it, so a caller that must REJECT an
     *  out-of-lattice coordinate (the neighbour walk does) bounds-checks against the layer's edge node
     *  count first - a clamped coordinate silently names a different cell. */
    CKVOXELNAV_API auto
    Get_MortonFromCoords(
        const FIntVector& InCoords) -> MortonCode;

    CKVOXELNAV_API auto
    Get_CoordsFromMorton(
        MortonCode InMortonCode) -> FIntVector;

    CKVOXELNAV_API auto
    Get_VectorFromMorton(
        MortonCode InMortonCode) -> FVector;

    CKVOXELNAV_API auto
    Get_ParentMorton(
        MortonCode InChildMortonCode) -> MortonCode;

    CKVOXELNAV_API auto
    Get_FirstChildMorton(
        MortonCode InParentMortonCode) -> MortonCode;

    /** Walks a code from its own layer up to a COARSER one. Layer indices grow coarser, so the target
     *  layer must be at or above the child's; a target at or below it returns the code unchanged. */
    CKVOXELNAV_API auto
    Get_ParentMortonAtLayer(
        MortonCode InChildMortonCode,
        LayerIndex InTargetLayerIndex,
        LayerIndex InChildLayerIndex) -> MortonCode;
}

// --------------------------------------------------------------------------------------------------------------------
