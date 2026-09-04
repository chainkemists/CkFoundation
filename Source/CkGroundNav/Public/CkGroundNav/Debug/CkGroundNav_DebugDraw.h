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
     * Everything that decides what a view SHOWS, apart from the capture it is showing.
     *
     * Held together because it is what a retained draw is rebuilt on: a capture that did not move
     * still has to be re-emitted when the mode changes, and a viewer comparing only the capture's key
     * would miss that. The overlay flags are here for a second reason - they are what a capture is
     * COLLECTED under, so flipping one has to force a fresh capture; the build itself reads only what
     * the capture already carries and never the flags.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_DebugDrawSelection
    {
    public:
        EDebugDrawMode _Mode = EDebugDrawMode::Plates;

        bool _DrawMarkup = false;
        bool _DrawLinks = false;
        bool _DrawInvalidation = false;

    public:
        auto
        Get_IsEqual(
            const FCk_GroundNav_DebugDrawSelection& InOther) const -> bool;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Per-kind counts of what one build emitted.
     *
     * Counted rather than inferred because the whole claim a view makes is that it drew the capture:
     * a plate the decomposition found and the outline standing for it are two numbers, and a viewer
     * is only trustworthy while they are the same number.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_DebugDrawTally
    {
    public:
        // Always one, on EVERY status: a view that showed nothing for a failed bake would be
        // indistinguishable from one pointed at empty space.
        int32 _RegionBoxes = 0;

        int32 _OpenBodyBoxes = 0;
        int32 _OpenBodyEdgeSegments = 0;

        int32 _PlateBoxes = 0;

        int32 _PortalSegments = 0;
        int32 _PortalMastSegments = 0;

        int32 _TileBoxes = 0;
        int32 _SeamSegments = 0;

        int32 _BoundarySegments = 0;
        int32 _BoundaryTickSegments = 0;

        int32 _LinkSegments = 0;
        int32 _LinkTickSegments = 0;

        int32 _MarkupBoxes = 0;
        int32 _MarkupDashSegments = 0;

        int32 _CorridorBoxes = 0;
        int32 _ChangedBoundsBoxes = 0;
        int32 _RepairBoxes = 0;
        int32 _RepairDashSegments = 0;

        // What a query command's own overlay contributed: a corridor outline, a crossing, a leg of a
        // route. Kept apart from the field's counts because the two are replaced on different news.
        int32 _QuerySegments = 0;

    public:
        auto Get_Total() const -> int32;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** One world-space segment of a build, with the colour and weight it is drawn at. */
    struct CKGROUNDNAV_API FCk_GroundNav_DebugDrawLine
    {
    public:
        FVector _Start = FVector::ZeroVector;
        FVector _End = FVector::ZeroVector;

        FColor _Color = FColor::White;

        float _Thickness = 1.0f;
    };

    /** One axis-aligned world-space box of a build. Wireframe: PMG draws the edges, never a fill. */
    struct CKGROUNDNAV_API FCk_GroundNav_DebugDrawBox
    {
    public:
        FVector _Centre = FVector::ZeroVector;
        FVector _Extent = FVector::ZeroVector;

        FColor _Color = FColor::White;

        float _Thickness = 1.0f;
    };

    /**
     * The line geometry of one view, built as a VALUE before anything is emitted.
     *
     * Building and publishing are separate so that what a mode draws can be asserted without a world,
     * a physics backend or a renderer: a build is a pure function of the capture and the selection,
     * and the tally beside it is what a test compares against the capture's own counts.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_DebugDrawBuild
    {
    public:
        TArray<FCk_GroundNav_DebugDrawLine> _Lines;
        TArray<FCk_GroundNav_DebugDrawBox> _Boxes;

        FCk_GroundNav_DebugDrawTally _Tally;

    public:
        auto
        Add_Line(
            const FVector& InStart,
            const FVector& InEnd,
            FColor         InColor,
            float          InThickness) -> void;

        auto
        Add_Box(
            const FVector& InCentre,
            const FVector& InExtent,
            FColor         InColor,
            float          InThickness) -> void;

        auto Get_ElementCount() const -> int32;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Which retained set a build replaces.
     *
     * Two, because the two are news about different things: the FIELD group stands until the capture
     * or the selection moves, where a QUERY group is one command's answer and is replaced by the next
     * command. Publishing them into one set would make each command erase the field it was asked
     * about.
     */
    enum class EDebugDrawGroup : uint8
    {
        Field,
        Query
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Bake the region around a point out of the LIVE physics world and return a standalone snapshot.
     *
     * A thin caller: it resolves the Jolt geometry backend for the world and hands it to
     * Make_DebugSnapshotFromBackend, which is where every status is decided. A world with no physics
     * backend answers BackendUnavailable through that same derivation rather than through a check of
     * its own, so the live path and the stub-driven pins take one route and not two.
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
     * The line geometry one selection draws out of one capture.
     *
     * Takes the snapshot BY VALUE-SEMANTIC REFERENCE and reads nothing else — no world, no registry,
     * no backend — so a capture outliving its bake builds exactly as it was captured, and so what a
     * mode emits can be counted without a renderer.
     *
     * The region box is emitted on EVERY status, and open Solid bodies in every mode and whatever the
     * status: a view that showed nothing for a failed bake would read as one pointed at empty space,
     * and the ground under an open body is untrustworthy however the developer is colouring it.
     * A capture that is not drawable emits those two and nothing else.
     */
    CKGROUNDNAV_API auto
    Make_DebugSnapshotDrawBuild(
        const FCk_GroundNav_DebugSnapshot&      InSnapshot,
        const FCk_GroundNav_DebugDrawSelection& InSelection) -> FCk_GroundNav_DebugDrawBuild;

    /**
     * Draw a snapshot: its line geometry into the world's retained FIELD set, and the handful of
     * elements PMG's retained tier has no equivalent for immediately.
     *
     * The retained set is what makes a Test build draw at all — the engine's immediate helpers compile
     * out where ENABLE_DRAW_DEBUG is off, and a debug view that silently drew nothing in the very
     * configuration a packaged check runs would report a level as unbaked. The immediate remainder
     * (the per-cell points, every text label, the spheres and arrowheads) is what a viewer LOSES in
     * such a build, and it is deliberately the labelling rather than the geometry.
     *
     * An explicit console bake is a new capture by definition, so this rebuilds unconditionally;
     * Do_UpdateRetainedDebugDraw is the gated form a per-frame caller uses.
     */
    CKGROUNDNAV_API auto
    DoDraw_DebugSnapshot(
        UWorld*                                 InWorld,
        const FCk_GroundNav_DebugSnapshot&      InSnapshot,
        const FCk_GroundNav_DebugDrawSelection& InSelection,
        FCk_Time                                InLifetime) -> void;

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

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Replace one of the world's retained groups with a build, as one value.
     *
     * The previous sets are destroyed and new ones created, chunked, in the same idiom the crowd's
     * retained breadcrumb uses: a set is a child entity of the world's transient entity, so it dies
     * with the world whether or not anybody released it. Nothing is emitted while
     * ck.GroundNav.Debug.RetainedDraw is 0 — that is the switch that puts the geometry away.
     *
     * Game thread only: it creates and destroys entities.
     */
    CKGROUNDNAV_API auto
    Do_PublishRetainedDebugDraw(
        UWorld*                             InWorld,
        EDebugDrawGroup                     InGroup,
        const FCk_GroundNav_DebugDrawBuild& InBuild) -> void;

    /**
     * Add a build to a group without disturbing what it already holds.
     *
     * A query command's answer is assembled from several sources - the route it found, the markup it
     * was planned around, the links it could have used - and each is drawn by the same function that
     * draws it for a field view. Appending is what lets those share one group instead of each erasing
     * the last. The rebuild count is not moved: one command is one rebuild, however many pieces it
     * publishes.
     */
    CKGROUNDNAV_API auto
    Do_AppendRetainedDebugDraw(
        UWorld*                             InWorld,
        EDebugDrawGroup                     InGroup,
        const FCk_GroundNav_DebugDrawBuild& InBuild) -> void;

    /**
     * Emit a capture into the world's retained FIELD set, and do NOTHING when neither the capture nor
     * the selection has moved.
     *
     * This is the per-frame form. The gate is the capture's own cache key beside the selection, which
     * is exactly what a rebuild would be a function of: a caller handing the same field the same way
     * every frame pays for one rebuild and then nothing, and a caller whose field republished or whose
     * mode changed pays for exactly one more.
     */
    CKGROUNDNAV_API auto
    Do_UpdateRetainedDebugDraw(
        UWorld*                                    InWorld,
        const FCk_GroundNav_DebugSnapshot&         InSnapshot,
        const FCk_GroundNav_DebugSnapshotCacheKey& InKey,
        const FCk_GroundNav_DebugDrawSelection&    InSelection) -> void;

    /** Destroy every retained set the world holds, in both groups, and forget the key they were built
     *  under. What ck.GroundNav.Clear does, and what the world's own teardown does for itself. */
    CKGROUNDNAV_API auto
    Do_ReleaseRetainedDebugDraw(
        UWorld* InWorld) -> void;

    /**
     * The same release, for every world at once.
     *
     * What ck.GroundNav.Debug.RetainedDraw does on the way to 0. A retained set stands until
     * something takes it down, so the switch that says the views are off has to be the thing that
     * puts them away - stopping at the next build would leave the last one standing.
     *
     * Game thread only: it destroys entities.
     */
    CKGROUNDNAV_API auto
    Do_ReleaseAllRetainedDebugDraw() -> void;

    /** How many times a group has been rebuilt for this world. The number a caller asserts against
     *  when the claim is that a static field costs one rebuild and not one per frame. */
    CKGROUNDNAV_API auto
    Get_RetainedDebugDrawRebuildCount(
        UWorld*         InWorld,
        EDebugDrawGroup InGroup) -> int32;

    /** What the group's last rebuild emitted, per kind. */
    CKGROUNDNAV_API auto
    Get_RetainedDebugDrawTally(
        UWorld*         InWorld,
        EDebugDrawGroup InGroup) -> FCk_GroundNav_DebugDrawTally;

    /** How many PMG line sets the group is currently holding. Zero once released. */
    CKGROUNDNAV_API auto
    Get_RetainedDebugDrawSetCount(
        UWorld*         InWorld,
        EDebugDrawGroup InGroup) -> int32;
}

// --------------------------------------------------------------------------------------------------------------------
