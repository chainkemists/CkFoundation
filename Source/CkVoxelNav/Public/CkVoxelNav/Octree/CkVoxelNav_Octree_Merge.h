#pragma once

#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Types.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// NODE MERGING: coalescing the free-cell set into a much smaller set of axis-aligned boxes, so a search
// expands tens of boxes where it used to expand thousands of cells.
//
// The merged table is a REPRESENTATION of free space, never a second source of truth about it. Occupancy,
// structure and every geometric answer still come from the octree; a merged cell is a box that the octree's
// own free cells tile exactly, and the cell -> merged lookup is what lets the kind-dispatching seam hand a
// search the coarse cell instead of the fine one.
//
// The pass is geometry-free (no probe, no backend) and runs in one slice at the end of a bake or a repair.
//
// Evidence for the shape: Massonnat & Verbrugge (McGill) 2024 measured merged octrees at a fraction of the
// plain octree's cell count and consistently sub-millisecond queries where the plain octree was multiple
// milliseconds. Their algorithm is not published in enough detail to port; this is greedy box growing over
// the same input, and cell-count reduction rather than optimality is what it is judged on.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    /** Every cell a search can stand in: the childless nodes of every layer, the fully-free leaves, and the
     *  free sub-nodes of every partly-occluded leaf. This is exactly the set the neighbour walk enumerates,
     *  which is what makes it the right input for merging - and it is also the PLAIN cell count an A/B
     *  benchmark compares against. */
    CKVOXELNAV_API auto
    Get_FreeCells(
        const FOctree& InOctree,
        TArray<FNodeAddress>& OutCells) -> void;

    /** The octree cell's world-space box. Cells are cubes, so this is centre +/- extent - but going through
     *  one function keeps the merge pass and the seam agreeing on cell geometry by construction. */
    CKVOXELNAV_API auto
    Get_NodeBoundsFromAddress(
        const FOctree& InOctree,
        const FNodeAddress& InAddress) -> FBox;

    // ----------------------------------------------------------------------------------------------------------------

    /** Replaces the octree's merged table with a merge of its whole free-cell set. */
    CKVOXELNAV_API auto
    Request_MergeCells(
        FOctree& InOutOctree) -> void;

    /** The LOCAL re-merge, mirroring the repair's own locality contract: every merged box of the published
     *  table that does not intersect the dirty bounds is carried over BIT FOR BIT, and only the boxes that
     *  did intersect it are re-derived from the repaired octree's cells.
     *
     *  This is sound because of what a repair guarantees: outside the dirty region the occupancy is carried
     *  over unchanged, so the structure derived from it is geometrically identical there and a carried-over
     *  box is still tiled exactly by cells of the repaired octree. Node INDICES do move, which is why the
     *  carry-over is keyed on lattice geometry rather than on a packed address.
     *
     *  Falls back to a full merge when the published octree carries no merged table or addresses a different
     *  lattice - there is then nothing to carry over. */
    CKVOXELNAV_API auto
    Request_MergeCellsLocally(
        FOctree& InOutOctree,
        const FOctree& InPublishedOctree,
        const FBox& InDirtyBounds) -> void;
}

// --------------------------------------------------------------------------------------------------------------------
