#pragma once

#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Query/CkGroundNav_QueryTypes.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// THREAD CONTRACT: pure over the field handed in; writes only to the returned value and the caller's
// diagnostics. Callable from any thread by anybody holding the field.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * Walk a body along walkable ground from its start toward a target, as far as the ground allows.
     *
     * THIS IS THE SINGLE TRANSFORM WRITER FOR GROUNDED AGENTS: every grounded agent's final position
     * each frame passes through it. Its contract is therefore absolute — the result is ON the
     * walkable set, always: the answering cell is admitted (or is the start cell, which is admitted
     * unconditionally) and the location lies inside that cell at that cell's surface height. No clamp,
     * epsilon or re-projection produces that; the walk does, by never entering a cell it may not.
     *
     * The walk is a grid line traversal from the start cell toward the target. At each cell boundary
     * it steps across (Get_StepAcross); a refused step slides — the motion loses its component along
     * the blocked axis and continues along the other — and a second refusal on the other axis ends
     * the walk. A concave corner therefore terminates in two slides, never oscillates. A move whose
     * start and target both lie in one plate whose minimum clearance admits the body is answered
     * without stepping a single cell, which is the common case for a per-frame delta.
     *
     * Statuses: Blocked (radius over the ceiling), the start resolution's NoSurface / Unbuilt when the
     * start is not on the surface, Success otherwise — a walk that moved nowhere is still a Success
     * at its start.
     */
    CKGROUNDNAV_API auto
    Get_MoveAlongSurface(
        const FCk_GroundNav_Field&             InField,
        const FCk_GroundNav_SurfaceWalkQuery&  InQuery,
        FCk_GroundNav_SurfaceWalkDiagnostics&  OutDiagnostics) -> FCk_GroundNav_SurfaceWalkResult;

    /**
     * Whether a body can walk the straight segment from start to end without leaving walkable
     * ground: the same traversal, without the slide — the first refused step is the hit.
     *
     * A segment whose two ends lie in one plate whose minimum clearance admits the body is clear by
     * construction and costs one cell read. The result is symmetric in its ends when clear, and a hit
     * found from either end lies within one cell of the other's.
     */
    CKGROUNDNAV_API auto
    Get_SurfaceRaycast(
        const FCk_GroundNav_Field&        InField,
        const FCk_GroundNav_RaycastQuery& InQuery) -> FCk_GroundNav_RaycastResult;
}

// --------------------------------------------------------------------------------------------------------------------
