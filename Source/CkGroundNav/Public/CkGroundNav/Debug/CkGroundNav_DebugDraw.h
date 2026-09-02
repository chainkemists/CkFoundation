#pragma once

#include "CkCore/Time/CkTime.h"

#include "CkGroundNav/Bake/CkGroundNav_AgentProfile.h"
#include "CkGroundNav/Bake/CkGroundNav_BakeTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_Plates.h"
#include "CkGroundNav/Debug/CkGroundNav_DebugSnapshot.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * What a drawn snapshot is coloured by.
     *
     * Open Solid bodies are outside this choice entirely: they draw in RED in EVERY mode, and even for
     * a snapshot that is not drawable at all. The ground under an open body is not trustworthy, so
     * whether a developer sees it must not depend on which view they happen to have selected.
     */
    enum class EDebugDrawMode : uint8
    {
        // Merged plates as wireframe boxes, coloured per layer. The cheapest view and the one that
        // shows at a glance whether the decomposition understood the level.
        Plates,

        // One point per walkable cell, ramped by how much room an agent has there.
        Clearance,

        // One point per walkable cell, coloured by which floor it belongs to.
        Layers,

        // The cells the walkability filters threw away. Shown alongside what survived, because a
        // filter tuned too tight and a world that genuinely has no floor look identical otherwise.
        Rejected,

        // The crossings between plates, drawn on the boundary they occupy and coloured by how much
        // room they offer. This is the only view that shows why a body wide enough for both rooms
        // still cannot get from one to the other.
        Portals,

        // The tile lattice and the crossings between tiles. Empty for a single-region bake; for a
        // field bake it is the view that shows whether the seams agree — which is the one thing a
        // tiled bake gets wrong invisibly.
        Tiles
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Every input the debug bake takes.
     *
     * Grouped rather than passed loose so that adding a tunable later does not re-order an argument
     * list that console commands and callers both depend on positionally.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_DebugBakeParams
    {
    public:
        FVector _Centre = FVector::ZeroVector;
        FVector _Extent = FVector{1500.0, 1500.0, 500.0};

        FCk_GroundNav_BakeConfig _Config;
        FCk_GroundNav_AgentProfile _Profile;
        FCk_GroundNav_MergeTunables _MergeTunables;

        int32 _MaxCells = 20000;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Bake the region around a point out of the LIVE physics world and return a standalone snapshot.
     *
     * Every stage of the bake runs exactly as it does headless — this only supplies the geometry from
     * Jolt instead of from a hand-authored box list, which is the whole point of the backend seam.
     *
     * A world with no physics backend yields BackendUnavailable and an empty region yields
     * NoGeometryInRegion. Neither is silently an empty scene.
     *
     * Game thread only.
     */
    CKGROUNDNAV_API auto
    Make_DebugSnapshotFromWorld(
        const UObject*                           InWorldContextObject,
        const FCk_GroundNav_DebugBakeParams&     InParams) -> FCk_GroundNav_DebugSnapshot;

    /**
     * Bake a whole tiled FIELD around a point out of the live physics world.
     *
     * Same pipeline as the single-region bake, run per tile with the halo each tile needs, so this is
     * the only debug path that exercises tiling, seam portals and reachability against real geometry.
     *
     * Game thread only.
     */
    CKGROUNDNAV_API auto
    Make_FieldDebugSnapshotFromWorld(
        const UObject*                       InWorldContextObject,
        const FCk_GroundNav_DebugBakeParams& InParams) -> FCk_GroundNav_DebugSnapshot;

    /**
     * Draw a snapshot with the engine's persistent debug lines.
     *
     * Takes the snapshot BY VALUE-SEMANTIC REFERENCE and reads nothing else — it never reaches back
     * to whatever produced it, so a snapshot outliving its bake draws exactly as it was captured.
     *
     * Open Solid bodies draw first, in red, in every mode and whatever the snapshot's status.
     */
    CKGROUNDNAV_API auto
    DoDraw_DebugSnapshot(
        UWorld*                            InWorld,
        const FCk_GroundNav_DebugSnapshot& InSnapshot,
        EDebugDrawMode                     InMode,
        FCk_Time                           InLifetime) -> void;

    /**
     * Human summary of what a snapshot contains. Safe on every status.
     *
     * Open Solid bodies are named in a block directly under the status line, ahead of every number a
     * developer reads to judge a bake — because none of those numbers mean anything while a body
     * over them is open.
     */
    CKGROUNDNAV_API auto
    Get_DebugSnapshotSummary(
        const FCk_GroundNav_DebugSnapshot& InSnapshot) -> FString;
}

// --------------------------------------------------------------------------------------------------------------------
