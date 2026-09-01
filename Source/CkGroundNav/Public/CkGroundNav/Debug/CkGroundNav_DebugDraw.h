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
    /** What a drawn snapshot is coloured by. */
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
        Rejected
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
     * Draw a snapshot with the engine's persistent debug lines.
     *
     * Takes the snapshot BY VALUE-SEMANTIC REFERENCE and reads nothing else — it never reaches back
     * to whatever produced it, so a snapshot outliving its bake draws exactly as it was captured.
     */
    CKGROUNDNAV_API auto
    DoDraw_DebugSnapshot(
        UWorld*                            InWorld,
        const FCk_GroundNav_DebugSnapshot& InSnapshot,
        EDebugDrawMode                     InMode,
        FCk_Time                           InLifetime) -> void;

    /** One-line human summary of what a snapshot contains. Safe on every status. */
    CKGROUNDNAV_API auto
    Get_DebugSnapshotSummary(
        const FCk_GroundNav_DebugSnapshot& InSnapshot) -> FString;
}

// --------------------------------------------------------------------------------------------------------------------
