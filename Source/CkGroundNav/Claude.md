# CkGroundNav

**Boundary — read this before anything else.** CkGroundNav owns *grounded* navigation: the walkable
surface of floors, ramps and stairs, represented as a layered ground field decomposed into merged
plates and joined by a portal graph. It is the ground member of a provider family — **`CkVoxelNav`**
answers volumetric free space for agents that are not stuck to a surface, **`CkPathNetwork`** answers
authored networks, and **`CkNavigation`** is the provider-neutral seam consumers speak to. A consumer
that reaches past that seam into this module has coupled itself to one provider.

All world geometry arrives through **`CkJolt`**'s public, JPH-free query surface. CkGroundNav includes
**no** Jolt headers — a hard invariant, not a preference (see Anti-patterns).

**Purpose:** bake a walkable ground field from world geometry and answer where an agent of a given
size can stand and cross.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkJolt`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`,
`CkShapes`, `CkThirdParty`.

---

## The shape of the thing

The bake is a pipeline of pure functions over value types. Every stage takes the previous stage's
output and a config, returns an `FCk_GroundNav_BakeStageResult`, and touches no world, no registry and
no physics — which is what lets the whole thing run headless against a hand-authored box list.

| Stage | Header | What it produces |
|---|---|---|
| Rasterize | `Bake/CkGroundNav_Rasterize.h` | `FCk_GroundNav_SpanField` — per-column ordered solid spans |
| Walkability | `Bake/CkGroundNav_Walkability.h` | span demotions + `FCk_GroundNav_ConnectionField` |
| Layers | `Bake/CkGroundNav_Layers.h` | `FCk_GroundNav_LayerField` — at most one span per column per layer |
| Clearance | `Bake/CkGroundNav_Clearance.h` | `FCk_GroundNav_ClearanceField` — how much room each cell has |
| Plates | `Bake/CkGroundNav_Plates.h` | `FCk_GroundNav_PlateField` — merged rectangles + cell→plate |
| Portals | `Bake/CkGroundNav_Portals.h` | `FCk_GroundNav_PortalField` — crossings + plate→portals |
| Tile | `Field/CkGroundNav_TileBake.h` | `FCk_GroundNav_Tile` — one published tile, halo cropped away |

Geometry reaches the pipeline through `ICk_GroundNav_GeometryBackend` (`Backend/`), which has a Jolt
implementation over the live static world and a stub over a box list. Tests use the stub; the debug
bake uses Jolt. Both drive the identical pipeline, so a bug reproduced headless is the same bug.

---

## The five ideas worth knowing before you change anything

**1. Clearance is why one bake serves every agent size.** The field is never eroded per radius.
Each cell carries the distance to the nearest cell an agent cannot stand on, and a query admits a
cell by testing its own radius against that number. Baking per radius is the thing this design
exists to avoid — do not reintroduce it.

**2. A plate's clearance cannot admit an agent; a portal's can.** Two rooms wide enough for anybody,
joined by a doorway narrow enough for nobody, both report generous clearance. The crossing carries
the number that decides passage. That number is the tightest point on the *widest* crossing the
portal offers — per cell pair the tighter of the two sides, maximised along the interval, because an
agent chooses where to cross. It is not a plain minimum over the interval; that would report every
doorway as the half-cell of room its own jamb has.

**3. The connection field is the only definition of adjacency.** `DoFilter_Walkability` builds it and
drops any edge the far span does not name back, so every surviving connection is mutual. Layer
extraction, the distance transform, the plate merge and portal extraction all read it and none of them
recompute "are these two cells neighbours". A second definition would drift from the first the moment a
filter changed — and a crossing that changes floor needs no special case precisely because a connection
names the neighbouring *span* rather than merely a direction.

**4. The field border counts as blocked, so a tile owes a halo.** Without a blocked border a fully
walkable field would have unbounded interior clearance and the number would mean nothing. The
consequence is that clearance reads short near a border, which for a tile would put a false pinch at
every seam. `DoBake_Tile` therefore rasterizes, filters, layers and measures clearance over a
halo-expanded lattice, then masks the halo away *before* plates are decomposed, so no plate or portal
straddles a tile boundary.

**5. Clearance saturates at a configured ceiling, and the halo is sized from it.** Halo width is
`ceil(cap / cellSize)` cells. Below the cap the field is exact; at or above it every cell reads the
cap. That is what makes a tile agree with a bake of a far wider region *exactly* rather than within a
tolerance — the alternative, truncating at the halo, makes a cell's value depend on how the world
happened to be tiled. A query for a radius above the cap cannot be answered on clearance alone, and
the tile carries its cap so such a query can refuse rather than silently mis-admit.

---

## The closed-collision contract

**Every Solid static body the bake reads must be a closed mesh** — every edge shared by exactly two
triangles. The bake sees faces and never a body's interior. A box resting on a floor is known to cover
that floor only through its underside, which lies flush on the floor's top face and wins the exact-height
tie in the rasterizer (non-walkable beats walkable on a tie). A wall with no underside, a fence modelled
as one quad, a building shell whose bottom was culled: none of these present a face in the columns
beneath them, and each bakes as open ground that an agent will path straight through. The rasterizer
does not rasterize vertical faces to compensate; that was ruled out in favour of this contract.

Heightfields are the exception. A terrain surface is `ECk_GroundNav_BodyKind::Surface`: open by
construction, with no interior to describe, its steep parts unwalkable on slope alone and a hole in it
a hole in the world. Everything else is `Solid` and owes closure.

The contract is enforced, not assumed. `DoCheck_GeometryClosure` fetches each body's WHOLE mesh once
per build (`Get_BodyTriangles`, unclipped, because a region-clipped mesh has cut edges that look like
holes), welds vertices within `kMeshClosureWeldToleranceUu`, and counts the edges only one triangle
uses. An open body does **not** fail the bake — the field publishes, one bad asset must not take a
level's navigation down — but it is recorded on `FCk_GroundNav_Field::_OpenBodies`, named in ONE warning
per build (which the AutoTest harness escalates to a failure), drawn in red in every debug mode, and
put on screen. The ground under an open body is not trustworthy until the asset is fixed: give it
simple/convex collision or a closed collision mesh.

---

## Area markup records

The VOLUME owns the area markup painted on it. `FFragment_GroundNavVolume_Markup` holds one
`FCk_GroundNav_MarkupRecord` per markup, and the identity a record is keyed on is the **markup
entity** — the same handle the neutral `FCk_Request_NavSurface_AreaMarkup` names on the other side of
the seam. A second request naming an entity the volume already holds updates that record in place and
re-stamps the epoch it was submitted against; disabling is one of those updates, never a delete. The
markup entity carries `FFragment_GroundNav_MarkupRef` — the volume and the record id, nothing else —
so a live probe reads the one array rather than a second copy that could drift from it.

Admission decides three things and nothing else: the entity is valid, the area tag has a policy
registered with `ck::nav_surface::TryGet_AreaPolicy` (that policy is where the record's kind and cost
multiplier come from — this module never decides what a tag means), and the record's world bounds are
a box. A record whose footprint misses every tile, or lands where nothing has baked, is still
admitted: the volume holds what was AUTHORED, and what a markup reaches is the bake's separate answer.

Admission then hands the change to the stage the record's kind owes, and the two hand-offs are
different SHAPES because the two stages are. A COST change raises
`FTag_GroundNavVolume_MarkupCostDirty` — a tag, because the derive restamps every tile from the whole
record list and has no use for a region. A WALKABILITY change owes an actual re-bake of the ground the
record covers, so it unions `Get_MarkupWorldBounds(record)` into the volume's `_PendingDirtyBounds` and
raises `FTag_GroundNavVolume_NeedsRepair` — a REGION, which is what the local repair takes.

An UPDATE marks the OLD record's bounds as well as the new one's, before the entry is overwritten:
ground a record moved off is decided by the new record only where the two overlap, and a footprint left
behind is ground nothing else will ever revisit. A RELEASE marks the released record's bounds for the
same reason painting them marked them. A walkability change on a volume with **nothing published** marks
no region and waits — there is no field for a repair to carry its untouched tiles over from, and the
record is already on the volume, so a build that STARTS after it bakes it in. That is the same wait the
cost derive makes, and it is why a paint before the first bake is never lost. A build already RUNNING is
the exception: it snapshotted its records when it started, so a record landing mid-build marks its
region and is repaired the moment that build publishes.

**Cost — `FProcessor_GroundNavVolume_MarkupCostDerive`.** Copies the published field,
restamps EVERY built tile's plates from the whole record list (`Get_FieldWithMarkupCost`), and swaps
the pointer through the same publish the build uses, in the same tick window. It reads no cell, no
span and no geometry, so it spends exactly zero probes. Every tile is restamped rather than only the
ones a listed record touches, because a record DELETED from the list names no tile and a touched-set
pass could never find the ground it used to price; stamping is per-plate work over rectangles the tile
already carries, so restamping a tile nothing reaches costs nothing. A tile's epoch — and the field's
— moves where the plates' policy fields actually changed, compared exactly, and also where an enabled
record the tile has not yet been published past reaches it, so a paint that changes nothing still
reads live; disabling a record and deleting it converge on the same field. A volume with
nothing published clears the tag and waits: the records are on the volume, and the next build prices
them through `FCk_GroundNav_FieldParams::_MarkupRecords`.

**Walkability — the local repair.** The dirty region a walkability change accumulates is answered by
`FProcessor_GroundNavVolume_StartRepair` / `_Repair` below, which re-bakes only the tiles the record's
halo-inflated footprint reaches. The markup path has no whole-volume rebuild and no walkability dirty tag: a walkability
change is a region, and the repair is what answers it.

What makes the scoped re-bake correct is that the repair is handed the volume's **current** records.
`StartRepair` passes `Get_MarkupRecordsOf(Get_MarkupRecords(volume))` to `Request_BeginRepair`, which
writes them into the repaired copy's `_Params._MarkupRecords` — the same records, obtained the same
way, that `StartBuild` hands to `Get_FieldParams`. Only the RECORDS are replaced: every other field of
the source params places the lattice the untouched tiles were produced on, and a repair handed fresh
params wholesale would publish two lattices in one field. Without that, a repair would bake its tiles
from the source field's records — whatever the last publish baked with — and answer the very markup
change that raised the region with the records that change replaced.

The tiles OUTSIDE the repaired set keep the stamps the last bake gave them, which is correct precisely
because the dirty box is the record's own footprint (old ∪ new): every tile the record reaches is in
the repaired set, and no tile outside it was ever decided by that record.

Every build reads the volume's current records into `FCk_GroundNav_FieldParams::_MarkupRecords`, so a
plain `Request_Build` that nothing painted still bakes against what the volume holds — a rebuild that
took no records would silently unpaint the world.

## Local repair

A **repair** re-bakes only the tiles a dirty world box reaches and republishes the whole field, where a
build re-bakes every tile. Two things raise one: a `Request_Repair` naming a `_DirtyBounds` box, and a
WALKABILITY markup change marking its record's footprint (above). For a body that MOVED the box is the
union of where it was and where it is: the new half closes the ground it arrived on, the old half
reopens the ground it left, and a request carrying only the new half leaves the old footprint blocked
for the life of the field.

**The tile set is halo-inflated and fixed at Begin.** `Get_RepairTileIndices` selects every tile the box
reaches after inflating it in XY by the field's own halo width, because a tile bakes against ground that
far outside itself — geometry that moved just past a tile's edge still moved that tile's clearance. The
set is then frozen, which is what makes a sliced repair produce the field a one-shot repair would. Tiles
outside it are carried across byte for byte with their epochs untouched, so `Get_ChangedTileBounds`
names exactly the ground the repair touched. The seam portals, the tile edge boundary, the reachability
labels and the open-body report are re-derived over the WHOLE tile set regardless, never patched.

**The published field is never touched** (Anti-patterns, below). The repair assembles a new field value
and the processor swaps the pointer, exactly as a build does.

**What the volume does with a request.** `FProcessor_GroundNavVolume_HandleRepairRequests` unions the
box into `_PendingDirtyBounds`, parks the request's completion, and raises
`FTag_GroundNavVolume_NeedsRepair` — several bodies dirtying one volume in a frame cost one repair, not
one each, and a markup change unions into the same box. A volume with nothing published and no build
coming is told `Failed`: a repair carries the untouched tiles over from a previous bake and there is
none, and a build is the repair for that. `FProcessor_GroundNavVolume_StartRepair` SNAPSHOTS the
accumulated box and the parked requests together and resets both, so a box arriving mid-repair opens the
next one instead of corrupting this one. A region raised by markup carries no request: nothing is
waiting on it, and its outcome is read the way every markup outcome is — through
`Get_IsMarkupLive`.

**Supersede and cancel.**

| Event | What happens to the repair |
|---|---|
| A build STARTS | every parked repair request and the pending box are taken over by it — the build re-bakes that ground from live geometry under the records it snapshots, so they complete `Succeeded` when it publishes. A region or request raised AFTER the start is outside that snapshot: it stays pending across the publish and opens its repair against the field the build produced |
| A build is ARMED (the drain, restart or not) while a repair is slicing | the open repair is cancelled and its in-flight requests complete `Failed_Cancelled`; parked requests stay parked until the build starts and takes them over |
| A build FAILS | the requests that rode it complete `Failed` — a build is what bakes that ground, and their callers are told rather than left waiting on a publish that is not coming |
| A repair completes but the field it opened against is no longer the published one | the repaired field is DISCARDED — publishing it would silently undo the other publish everywhere the repair did not look — the in-flight requests complete `Failed`, and the region is raised again |
| The slice fails (`StaleGeometry`, backend gone) | in-flight requests complete `Failed` and the region is raised again, ONCE. A second failure running drops the region with an ensure: fail-closed with a bounded escape, because a region re-raised forever opens the same doomed repair every tick |
| The volume ends play | the queue, the parked list and the in-flight list all complete `Failed_Cancelled` |

A repair never OPENS while `NeedsBuild` or `BuildInProgress` is set, so the two never race for the
publish. A repair whose box reached no tile is not an error: it completes, moves no epoch, and publishes
nothing — the same answer the cost derive gives when nothing it restamped moved.

## When a markup is LIVE

`ck::groundnav::nav_surface_adapter::Get_IsMarkupLive` is the whole rule and it is DERIVED at the
read; nothing anywhere stores it. From the markup entity: no `FFragment_GroundNav_MarkupRef` (the
paint has not drained onto a volume yet) is false, a record the named volume no longer holds is false,
a volume with nothing published is false. Otherwise, over the published field, **every** tile whose
world bounds meet the record's world bounds must be `Built` and must carry an epoch STRICTLY PAST the
record's `_RequestedAtEpoch` — the stamp is the epoch the field was already published at, so an equal
epoch is the very publish that knew nothing of the record — every one, because a record reaching two
tiles is only as live as its
laggard. A record whose bounds meet no tile at all is NOT live: there is no ground for it to be live
on, and true would mean only that nothing contradicted it.

The provider-neutral entries this backs (`_ApplyAreaMarkup`, `_IsMarkupLive`, `_ReleaseAreaMarkup` on
`FCk_NavSurface_ProviderTable`) live in `Facade/CkGroundNav_NavSurfaceAdapter.cpp`. A paint is
enqueued on every volume whose bounds meet it, carrying the SAME markup entity — a paint straddling
two volumes is one markup held twice — and a paint meeting no volume is refused with an ensure, since
a volume is the only thing that holds a record. A release is enqueued on every volume holding an entry
for the entity, not only the one the back-pointer names, for the same straddling reason. Volumes enter
the world-field registry at Setup with no field yet, so a paint can find one before its first build.

`ck.GroundNav.Debug.MarkupLiveGate 0` (`Debug/CkGroundNav_DebugGates.h`) forces the entity-shaped
answer TRUE straight after its back-pointer guard, so a fixture that settles on liveness waits for
nothing. It is a fixture-proving tool, not a fallback: a paint-then-repath race pin that passes
under it has pinned nothing, and the default of 1 is the field's own answer.

**Settled** is the other named condition, and it is about the VOLUME rather than about one paint.
`UCk_Utils_GroundNavVolume_UE::Get_IsSettled` is true when a field is published and no stage still owes
this volume a publish: not building (`NeedsBuild`/`BuildInProgress`), no repair open or armed
(`RepairInProgress`/`NeedsRepair`), no cost re-derive owed (`MarkupCostDirty`), and none of the three
request queues holding anything — emptiness, never presence, because the drains reset a queue's array
in place and leave the fragment on the volume. The provider table's `_IsSurfaceSettled` folds it over
every volume in the world and answers false for a world holding none. It is what a fixture waits on
after a paint, a release, or a rebuild kick, instead of a hop count. Note that the rebuild hook stays a
KICK: `_RequestSurfaceRebuild` enqueues a build per volume and returns the moment they are enqueued, so
the request landing is never the answer — settling is.

## What a plate's price reaches

`Get_SurfaceAttributes` answers `_CostMultiplier` and `_AreaTags` from the resolved PLATE's own label
— policy is stored per plate precisely so a query never pays per cell for what does not vary per cell.
`Get_AreaMultiplier` prices a leg at the GREATER of that plate multiplier and whatever the per-query
cost table names for the same flat plate: a query's table overrides upward only and can never talk a
marked plate back down to bare ground. That is the same "greater wins" rule overlapping markup already
merges under, so a plate priced by two sources has one answer whether they met in the bake or at the
query. `Get_MaxMerged` stays the pure table helper it always was.

---

## Failure is a status, never an empty field

`ECk_GroundNav_BakeStatus` and `ECk_GroundNav_BuildStatus` exist because a region with no floor and a
region whose bake could not run are identical in the data and could not be less alike to a path: the
first is a place with nowhere to walk, the second a place nothing is known about. A backend that
cannot answer yields `BackendUnavailable`, an exhausted budget yields `BudgetExhausted`, and neither
is ever published as a built field with no cells.

---

## Nothing outlives its world

Every field, markup record, revision, repair and queued request lives on an entity in ONE world's
registry, and the four things that are not entities are keyed by the world and dropped at
`FWorldDelegates::OnWorldCleanup`: the world-field registry (`Facade/CkGroundNav_WorldFieldRegistry`),
the debug field the query commands read (`Debug/CkGroundNav_DebugDraw`), and CkNavigation's two
per-world mirrors (which provider a world chose, which shadow mode). Nor does a field outlive its
VOLUME: a volume leaves the registry at end-play, so a destroyed volume answers no query on its world
from then on, and its tile epochs are carried as retired revision so the world's surface revision
never falls - a consumer holding the old number sees ground that went away as a change, not a rewind. What is process-wide is CODE — a
provider table per provider, the area-policy registry and the Recast adapter's tag tables, all seeded
once at module load and never written by a world. Two PIE worlds therefore never see each other's
paint or each other's rebuilds, and ending one leaves the other's queries correct; the multi-world pins
(`MultiWorld.*`, `Ck_AutoTest_Net_GroundNav_TwoWorldsDoNotShareFields`) hold that line. Before adding a
`static` that holds a handle, a field pointer or a `UWorld`, key it by the world and clear it in the
same cleanup hook, or put it on an entity.

"Settled" is a question, never a wait. `UCk_Utils_GroundNavVolume_UE::Get_IsSettled` is true when a
volume has a published field, is not building, has no repair open or pending, no cost derive owed and
nothing queued that no stage has picked up yet;
the world answers `Get_IsSurfaceSettled` through the neutral facade only when every volume is settled
AND no markup request or release is still riding the pipeline — a release is work the provider has
not seen yet, so the world is not settled the frame it is requested. Tests kick a rebuild with
`Request_SurfaceRebuild_ForTesting` and wait on that named condition; a hop count is not evidence.

---

## Debugging it

`Debug/CkGroundNav_DebugDraw.h` bakes the live world around a point and draws the result.

| Command | Behaviour |
|---|---|
| `ck.GroundNav.Bake` | bake around the player pawn |
| `ck.GroundNav.BakeAt <X> <Y> <Z>` | bake around an explicit point — use this with a flying pawn |
| `ck.GroundNav.BakeFieldAt <X> <Y> <Z>` | bake a TILED field around a point and keep it for the query commands below |
| `ck.GroundNav.Clear` | flush the drawn field and drop the kept one |
| `ck.GroundNav.Print` | dump every tunable |
| `ck.GroundNav.Probe` / `ProbeAt <X> <Y> <Z>` | project the pawn (or a point) onto the kept field and show the surface, its plate and clearance (`ck.GroundNav.Debug.Probe*` cvars shape the search box) |
| `ck.GroundNav.WalkAt <X> <Y> <Z> <TX> <TY>` | constrained surface walk from a point toward a target, drawn with every slide |
| `ck.GroundNav.RayAt <X> <Y> <Z> <TX> <TY>` | walkability raycast, drawn to its hit |
| `ck.GroundNav.EdgesAt <X> <Y> <Z> <R>` | the boundary runs within R of a point, and the nearest one in magenta |
| `ck.GroundNav.ReachAt <X> <Y> <Z> <TX> <TY> <TZ>` | reachability by component label between two points, coloured by verdict, with the expansion count |
| `ck.GroundNav.PathAt <X> <Y> <Z> <TX> <TY> <TZ>` | a path for the debug body between two points: the plate corridor outlined, every crossing with the point its leg is priced through, and the string-pulled route in magenta with its length |
| `ck.GroundNav.FloodAt <X> <Y> <Z> <R>` | flood fill from a point out to a walked distance R: reached plates coloured by entry distance, settled crossings traced back to the source |
| `ck.GroundNav.PointsAt <X> <Y> <Z> <R> <N>` | N random points on walkable ground within a horizontal radius R, uniform by area over every storey the disc touches, spheres coloured per layer |
| `ck.GroundNav.FarPointsAt <X> <Y> <Z> <MIN> <MAX> <N>` | N random points whose WALKED distance from the point lies in [MIN, MAX], drawn from the plates a flood fill reaches; the label reports how many draws it spent |
| `ck.GroundNav.GridAt <X> <Y> <Z> <HALF> <SPACING>` | a lattice of points at SPACING over walkable ground inside the box of half-extent HALF, one per storey per position, phased to the field origin |
| `ck.GroundNav.MarkupAt <X> <Y> <Z>` | every area markup the world's volumes hold — id, kind, tag, multiplier, enabled, world bounds, requested-at epoch, and live yes/no through the neutral facade — plus the plate under the point with its policy index, tags and multiplier. Every volume is listed with where the point falls on it, because a record painted before anything baked lives on a volume that covers nothing yet. Reads the volumes' PUBLISHED fields, so `BakeFieldAt` is not needed |
| `ck.GroundNav.Invalidation` | every cached path corridor in the world and what a republish is measured against: the corridor box the invalidator intersects, what it was inflated by, the epoch the plan was made on against the epoch the field covering it has published, and whether the agent is already flagged for a repath. A corridor whose own epoch is not behind the field's is one no queued rebuild can be news to, which is the first thing the invalidator decides. Also reports the ground each field last published changed, the local repair a volume has open with its tile count, any dirty ground still waiting for one, how many rebuild boxes are pushed but not broadcast yet, and whether either gate below is bypassing the answer — a repair being the other thing a republish comes out of. Outlines every corridor in cyan, every changed-bounds box in orange, and the repair's ground in green. Reads the PUBLISHED fields, so `BakeFieldAt` is not needed |
| `ck.GroundNav.RepairAt <X> <Y> <Z> <HALF>` | declare a box of ground no longer trustworthy and ask every volume it reaches for a LOCAL repair of exactly that ground. Per volume it reports the tiles the box would select (through the same pure `Get_RepairTileIndices` the repair itself uses), the dirty ground already pending on it, and whether a repair is already in flight — then logs that volume's own outcome when the repair ENDS, which is ticks away and never the moment the box is accepted. A volume whose published field does not reach the box is listed and left alone. Reads the volumes' PUBLISHED fields, so `BakeFieldAt` is not needed |
| `ck.GroundNav.Debug.DrawMarkup` / `.MarkupLiveGate` | `DrawMarkup` (default 1) outlines markup in the plate view and in `PathAt`/`FloodAt`: impassable red, cost amber with its multiplier, disabled dashed grey. `MarkupLiveGate` (default 1) at 0 forces GroundNav's `Get_IsMarkupLive` true — the bypass a paint-then-repath race pin must FAIL under to be evidence |
| `ck.GroundNav.Debug.DrawInvalidation` | (default 0) draws the invalidation state in the plate view: every cached corridor box in cyan, thicker where it is flagged for a repath and labelled with its two epochs and its inflation, and the ground each published field last reported changed in orange. The orange box carries a short lifetime of its own because it describes ONE publish. Off by default — a corridor is per AGENT, so a crowd draws one box each |
| `ck.GroundNav.Debug.RepairHighlightSeconds` | (default 2.0) how long the tiles an open local repair is re-baking stay highlighted in green, alongside that repair's dirty box and the dashed box of ground still waiting for one — all drawn under `DrawInvalidation`. A short lifetime of its own for the same reason the changed-bounds box has one: a repair's tile set describes ONE publish and stops being true the moment the next slice lands, and a repair that finishes inside a frame would otherwise leave nothing to catch |
| `ck.GroundNav.Debug.RepathOnRebuild` | (default 1) flags an agent for a repath when a published rebuild meets its cached corridor, which is the shipping behaviour. At 0 the invalidator flags nobody however much ground moved — the bypass a rebuild-then-repath pin must FAIL under to be evidence |

The query commands read the field the last `BakeFieldAt` kept for THIS world (each world keeps its own, dropped with it); `Bake`/`BakeAt` produce a region snapshot
with no field to query. The body radius every query uses is `ck.GroundNav.Debug.AgentRadiusUu`.

Draw modes (`ck.GroundNav.Debug.Mode`): 0 plates, 1 clearance ramp, 2 layers, 3 the cells the filters
rejected, 4 the crossings between plates, 5 the tile lattice and the seams between tiles, 6 the plate
edges nothing crosses (rim runs in orange). In mode 5 the tiles carrying the field's newest epoch tint
green and label it, which is exactly the set the last publish re-baked — a field whose built tiles all
share one epoch is all of its own news and tints none of them. Mode 3 is the only view that shows what
a filter *costs* — a ledge sensitivity tuned too tight and a world that genuinely has no floor produce
an identical walkable set and differ only in what was thrown away.

**Read the `region` and `lattice` lines of the summary before forming any hypothesis about a bake.**
Every per-cell count is bounded by the lattice the summary prints, and halving the cell size quadruples
that lattice; a count read against the wrong cell size has already sent one investigation down a hole.

`FCk_GroundNav_DebugSnapshot` is the copy boundary: a value-only structure holding no world, no actor,
no handle and no span field, so a viewer can draw it a frame later or after its world is gone.

---

## Anti-patterns

- **Do not include a Jolt header here.** Geometry comes from CkJolt's JPH-free surface. The bake math
  is free of engine types entirely — no `UWorld`, no `FCk_Handle`, no `UObject` under `Bake/`.
- **Do not store a pointer or an engine-object reference inside a field value type.** Stable integer
  ids only: tile coord, layer index, plate index, portal index. This is also what makes serialization
  possible at all.
- **Do not patch a published field.** A rebuild derives a new tile and swaps it. Patching makes
  corruption representable, which is the property this representation was chosen to avoid.
- **Do not add a fourth plate-merge criterion.** There are exactly three — plane-fit tolerance, normal
  cone, policy equality — and only the first two are tunables. Widening the plane-fit tolerance past
  the shallowest riser in the content merges the treads either side of it and the step stops existing
  for everything downstream; watch the worst height spread, not the residual, because a plane fits two
  treads perfectly well by tilting through them.
- **Do not derive adjacency from plate rectangles.** See idea 3.
- **Do not compare a bake against a tolerance where it should be exact.** The distance transform, the
  decomposition and the portal extraction are all deterministic; a test that accepts "close enough"
  is a test that will pass through a propagation bug.
