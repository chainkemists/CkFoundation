#pragma once

#include "CkCore/Time/CkTime.h"

#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkGroundNav/Bake/CkGroundNav_AgentProfile.h"
#include "CkGroundNav/Bake/CkGroundNav_BakeTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_Plates.h"
#include "CkGroundNav/Debug/CkGroundNav_DebugSnapshot.h"
#include "CkGroundNav/Field/CkGroundNav_Field.h"

#include <CoreMinimal.h>

#include "CkGroundNav_DebugDraw.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Logs the outcome of a repair the debug console asked for.
 *
 * It exists only because FCk_Delegate_Request_OnCompleted is a DYNAMIC delegate: it binds to a
 * UFUNCTION on a UObject and to nothing else, and a console command has no object of its own. The
 * reporter roots itself when it takes the binding and unroots inside the handler, which the
 * request-completion contract guarantees runs exactly once — so it is alive for precisely as long as
 * the repair it is reporting on, and no static outlives the UObject system holding it.
 */
UCLASS()
class CKGROUNDNAV_API UCk_GroundNav_DebugRepairReporter_UE : public UObject
{
    GENERATED_BODY()

public:
    /** A completion delegate bound to a freshly rooted reporter. */
    static auto
    Make_CompletionDelegate() -> FCk_Delegate_Request_OnCompleted;

private:
    UFUNCTION()
    void
    DoOn_RepairCompleted(
        FCk_Handle InVolume,
        ECk_Request_OperationResult InResult);
};

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
        Tiles,

        // The plate edges nothing crosses — every wall, drop and hole rim the field knows about,
        // each with a tick showing which side of it is walkable. Runs on a TILE rim draw apart from
        // the rest, because those are walls only until the neighbour is baked and a viewer that read
        // them as permanent would call unbaked ground a wall.
        Boundary,

        // The links a published field resolved, over the plates they join. An authored link is the
        // one crossing in the field that no geometry accounts for, so it is the one thing no other
        // view can be read to infer.
        Links
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
     * The same bake, also handing back the field it produced.
     *
     * A snapshot is a drawable copy and carries nothing a query can run against, so a caller that
     * wants to PROBE the bake it just drew needs the field itself. The field is immutable and holds
     * no world reference, so it outlives the world it was baked from.
     *
     * OutField is left invalid on every failing status.
     *
     * Game thread only.
     */
    CKGROUNDNAV_API auto
    Make_FieldDebugSnapshotFromWorld(
        const UObject*                       InWorldContextObject,
        const FCk_GroundNav_DebugBakeParams& InParams,
        FCk_GroundNav_FieldPtr&              OutField) -> FCk_GroundNav_DebugSnapshot;

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
     * Every area markup the world's ground-nav volumes hold, as values.
     *
     * GAME THREAD ONLY: resolving a volume handle and asking the neutral facade whether one paint is
     * live both read the ECS registry. Both answers are captured HERE so the snapshot that carries
     * them stays drawable after the world is gone.
     */
    CKGROUNDNAV_API auto
    Make_DebugMarkupsFromWorld(
        UWorld* InWorld) -> TArray<FCk_GroundNav_DebugMarkup>;

    /**
     * Outline area markup: impassable in red, cost in amber labelled with its multiplier, and a
     * record the volume still holds but has disabled in dashed grey.
     *
     * A disabled record is drawn rather than omitted for the same reason an unbuilt tile is: nothing
     * drawn is indistinguishable from released, and those are the two a markup investigation is
     * trying to tell apart.
     */
    CKGROUNDNAV_API auto
    DoDraw_DebugMarkups(
        UWorld*                                    InWorld,
        TConstArrayView<FCk_GroundNav_DebugMarkup> InMarkups,
        FCk_Time                                   InLifetime) -> void;

    /**
     * Every navigation link the world's published fields resolved, as values.
     *
     * GAME THREAD ONLY: resolving a volume handle and asking whether one link is live both read the
     * ECS registry. Both answers are captured HERE so the snapshot that carries them stays drawable
     * after the world is gone.
     */
    CKGROUNDNAV_API auto
    Make_DebugLinksFromWorld(
        UWorld* InWorld) -> TArray<FCk_GroundNav_DebugLink>;

    /**
     * Draw resolved links: green where the link is traversable, grey where the author disabled it,
     * orange where an end stands over ground nobody has baked yet, red where an end found no ground
     * at all. Every allowed direction carries its own arrowhead, every resolved end a tick on the
     * surface it landed on, and the midpoint carries the link's id.
     *
     * A link that resolved to nothing is drawn rather than omitted, for the same reason a disabled
     * markup is: nothing drawn is indistinguishable from nothing authored, and those are the two an
     * investigation is trying to tell apart.
     */
    CKGROUNDNAV_API auto
    DoDraw_DebugLinks(
        UWorld*                                  InWorld,
        TConstArrayView<FCk_GroundNav_DebugLink> InLinks,
        FCk_Time                                 InLifetime) -> void;

    /**
     * Every ground-path corridor the world's agents hold, as values.
     *
     * GAME THREAD ONLY: it walks the ECS registry for the cached corridors and asks the world's
     * field registry which epoch each one is measured against. Both answers are captured HERE so
     * the snapshot that carries them stays drawable after the world is gone.
     */
    CKGROUNDNAV_API auto
    Make_DebugCorridorsFromWorld(
        UWorld* InWorld) -> TArray<FCk_GroundNav_DebugCorridor>;

    /**
     * The ground each of the world's published fields last reported its news about, reconstructed
     * from the tiles carrying the field's own epoch - which is exactly the box that publish handed
     * to the neutral rebuilt signal, derived rather than remembered.
     *
     * A field whose epoch no BUILT tile carries contributes nothing: bounds-unknown is what that
     * publish said, and a degenerate box would name ground it never produced.
     */
    CKGROUNDNAV_API auto
    Make_DebugChangedBoundsFromWorld(
        UWorld* InWorld) -> TArray<FBox>;

    /**
     * Outline cached corridors in cyan and last-published changed bounds in orange, each labelled
     * with the numbers the invalidation decision is made on.
     *
     * The changed-bounds boxes take a SHORT fixed lifetime of their own: one describes a single
     * publish, where a corridor stands until its agent replans, so drawing them alike would leave
     * a stale box per republish over a plate view that persists for a minute.
     */
    CKGROUNDNAV_API auto
    DoDraw_DebugInvalidation(
        UWorld*                                      InWorld,
        TConstArrayView<FCk_GroundNav_DebugCorridor> InCorridors,
        TConstArrayView<FBox>                        InChangedBounds,
        FCk_Time                                     InLifetime) -> void;

    /**
     * The local-repair state of the world's ground-nav volumes, written onto the snapshot as values.
     *
     * GAME THREAD ONLY: it resolves volume handles out of the world field registry. The repair's
     * tiles are placed here too, because the indices a repair carries address the volume's own
     * lattice and a snapshot's tiles come from a separate debug bake.
     *
     * ONE volume's open repair is reported, and the first pending dirty box found — which need not be
     * the same volume. Two volumes repairing at once are two separate pieces of news, and a union of
     * their boxes would name ground neither of them touched.
     */
    CKGROUNDNAV_API auto
    Do_StampRepairFromWorld(
        UWorld*                      InWorld,
        FCk_GroundNav_DebugSnapshot& InOutSnapshot) -> void;

    /**
     * Outline the dirty ground waiting on a repair (dashed) and the box an open repair is fixing,
     * and highlight the tiles that repair is re-baking.
     *
     * Both boxes take the short changed-bounds lifetime, and the tile highlight takes
     * ck.GroundNav.Debug.RepairHighlightSeconds, for the same reason a changed-bounds box does: a
     * repair is news about one publish and stops being true the moment the next slice lands.
     */
    CKGROUNDNAV_API auto
    DoDraw_DebugRepair(
        UWorld*                            InWorld,
        const FCk_GroundNav_DebugSnapshot& InSnapshot) -> void;

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
