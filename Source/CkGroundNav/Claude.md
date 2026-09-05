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

*Profile variants, and what earns one.* A volume may carry `_ProfileVariants` — extra
`FCk_GroundNav_AgentProfile`s, each named by an `FGameplayTag`, each baked into its own field out of
the same single pass over the geometry. **Radius never justifies one**: it is answered at query time
by idea 1, and a variant for a wider agent is the per-radius bake this design refuses. What earns a
variant is a *walkable-set* change — a shorter step, a steeper slope limit, a lower standing volume —
because no query-time predicate can recover which ground a different profile could stand on from a
field baked under another. `_Profile` stays the **untagged default**: it is what a query carrying no
`_ProfileTag` is answered from, and it is never one of the variants. The selector is the tag, all the
way down — `FCk_NavSurface_*Query::_ProfileTag` and `FCk_Request_GroundNavPath_FindPath::_ProfileTag`
reach `world_fields::TryGet_Field(world, location, profileTag)`, which returns that profile's field or
**nothing**, never the default's: silently substituting it would walk an agent up a step it cannot
climb. A tag must be non-empty and unique per volume; both are refused where the params are judged.
The variants are published in the same call as the default — one `world_fields::Publish`, one write
lock — and re-derived beside it, each keeping its own epoch with the fragment's `_Epoch` as the
newest. An invalidator resolves a cached corridor's field through the corridor's OWN profile tag, so a
change that reached only a variant invalidates the routes planned over that variant and leaves the
routes planned over the default alone. **A repair on a volume that holds variants converts to a full
rebuild** — local repair is single-field, and repairing only the default would leave it describing the
world as it is and every variant as it was. A volume that repairs often and holds variants therefore
pays a whole-volume bake for every dirty region; a repair that runs over every profile is what removes
that cost.

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

## Nav links

The VOLUME owns the links authored on it, exactly as it owns the markup. `FFragment_GroundNavVolume_Links`
holds one `FCk_GroundNav_LinkEntry` per link — the link entity every request keys on, beside the
`FCk_GroundNav_LinkRecord` that is the pure value — and the record is two WORLD POINTS with no index in
it at all: a plate id is valid only against the field it was derived on, so a record carrying one would go
quietly wrong on the first rebuild that renumbered it. Ids are handed out monotonically per volume and
never reused, so a retired id never comes back meaning a different link and a field resolved against an
older link set can be diffed against a newer one. Every build, repair and derive copies the volume's
current records into `FCk_GroundNav_FieldParams::_Links`, for the same reason every build copies the
markup records: a plain `Request_Build` that nothing authored must not un-link the world any more than it
unpaints it.

What a record RESOLVES to is the field's separate answer. `FCk_GroundNav_Field::_ResolvedLinks` holds one
`FCk_GroundNav_ResolvedLink` per authored record, in authored-id order, carrying each end's surface, its
flat plate and its own `ECk_NavSurface_QueryStatus` beside the direction, the two multipliers, the
clearance and the tags the record named. The whole array is re-derived on every publish rather than
patched, exactly like the seam portals and the reachability labels, so a resolution can never outlive the
plate numbering it was answered under.

**A link change is a DERIVE.** Admission raises `FTag_GroundNavVolume_LinksDirty`;
`FProcessor_GroundNavVolume_LinkDerive` copies the published field, re-resolves every record in
`_Params._Links` through `Get_FieldWithLinks`, re-runs the reachability labelling and swaps the pointer
through the same publish the build uses. It re-bakes nothing and spends ZERO geometry probes — a link is
two world points, and finding what they stand on is a projection over cells that are already published, so
no span, no clearance and no plate of any tile can move under it. It is not the cost derive's pinned
zero-CELL-READ claim; it does read cells, a bounded number per end, and never reaches the backend. It runs
AFTER the cost derive by an explicit dependency edge and nothing runs after it: both are owed in the same
tick whenever one request changed a price and another changed a link, and a link derive that went first
would hand the restamp a field whose links it then re-resolved from the record list that change replaced.
A build or a repair that is RUNNING, and a repair that is armed, KEEP the tag: the derive runs again
in the tick their publish lands, after it. A volume with nothing published, or one a build is armed
for, clears the tag instead — that publish resolves the records itself, which is the same wait the cost
derive makes and the reason a link authored before the first bake is never lost.

**An end over unbaked ground is HELD, never dropped.** Its status reads `Unbuilt`, the link contributes no
crossing and no label, its record stays on the volume, and the next publish over that tile resolves it
with the author doing nothing. Only `NoSurface` and `Blocked` leave a link genuinely unresolved, and even
then the record stays: it is the field's graph that drops it, never the volume. A dropped link NEVER
warns. It is a status — `Get_UnresolvedLinkCount` on the field and on the volume, the per-end status in
the resolved entry, the snapshot's `_Links`, a red draw, `ck.GroundNav.LinksAt`, and `Get_IsLinkLive`
false — with one `Display` line per publish when the count is non-zero, because an end waiting on
geometry is the expected state of a link authored ahead of it and a warning there would be reporting the
schedule rather than a defect.

**Cost is a MULTIPLIER on the link's own span, and never below one.** `_CostMultiplierForward` and
`_CostMultiplierBackward` price the Euclidean distance between the two endpoints, and admission refuses
anything under 1.0. That is what keeps every edge in the graph costing at least its own length, which is
the property the search's Euclidean heuristic is admissible under at w = 1: a link cheaper than its own
straight line would make the heuristic optimistic and the answers stop being shortest. Admission is by the
record's authored `_ClearanceUu` through `Get_IsAdmitted`, defaulting to
`kAdmitsAnyAgentClearanceUu` so a link that names no width admits every body the plates at its ends
already admit — narrow it for a ladder or a crawl.

**In the search a link is an ordinary crossing with an index.** `FCk_GroundNav_Crossing` carries an
`int32 _LinkIndex` into `_ResolvedLinks`, `INDEX_NONE` for a lattice crossing, and `Make_CrossingKey`
hashes it — so a ladder beside a ramp between the same two plates is two nodes and not one, and the
cheaper of them is still expanded. A link crossing collapses `_Left` and `_Right` onto its ENTRY endpoint,
because an authored link joins two points and there is no interval to slide along, and its traverse is
charged on the INCOMING edge, so a link node's cost-so-far already holds it and the heuristic reads its
departure point. `DoBuild_Corridor` pushes TWO consecutive degenerate funnel portals, the entry and the
exit, so the ONE string-pull bends at both endpoints without a second implementation; `Get_CornerOffset`
leaves a waypoint exactly equal to one of those pinned points where it is, because an authored endpoint is
a place a body must actually pass through rather than a corner to be pushed off the wall it hugs.

**When a link is LIVE.** `UCk_Utils_GroundNavVolume_UE::Get_IsLinkLive` is derived at the read and nothing
stores it — the shape `Get_IsMarkupLive` has, narrower in exactly one way. The link must have RESOLVED,
because a markup that reaches nothing is admitted and simply decides nothing where a link that did not
resolve is a link that is not there. A record the author DISABLED reads false, as a markup's does: live
means in effect.
Otherwise both endpoint tiles must be `Built` and must carry an epoch STRICTLY past the record's
`_RequestedAtEpoch`, which is stamped with the epoch the field was already published at. A fixture that
disabled a link therefore waits on `Get_IsSettled`, never on liveness — and `Get_IsSettled` answers false
while `FTag_GroundNavVolume_LinksDirty` is up or the link request queue holds anything, so the provider
table's `_IsSurfaceSettled` fold inherits both.

**Runtime state is the DERIVE, and there is no second copy of it.** Switching a link on or off is a
`Request_Link` naming the same link entity with `_Enable` flipped: it updates the record in place, raises
`FTag_GroundNavVolume_LinksDirty`, and the derive republishes with the endpoint tiles' epoch bumped. There
is deliberately no per-world overlay saying which links are on — two sources of truth for "is this link in
effect" is the split-brain the module refuses for crossing keys and plate prices, and the labels the derive
recomputes would go stale against it the moment they disagreed. The epoch bump the toggle causes is not
incidental either: it is the channel every downstream reader — liveness, the path invalidator, a fixture
waiting on `Get_IsSettled` — actually hears the toggle through.

What runtime state adds around that mechanism is the API it was missing, and nothing else.
`Request_ReleaseLink_ById` drops the record carrying an id wherever the entity that authored it has got
to; `Request_ReleaseAllLinks` empties the volume without rewinding the id counter, so a field resolved
against the emptied list can still be diffed against an older one. `Request_LinkBatch` authors many links
under ONE admission and ONE completion, and it is ATOMIC: every entry is judged before any is applied, so
a batch carrying one refusal leaves the volume exactly as it found it and completes `Failed` with no ids
spent. The completion is the only thing the batch buys — the request drain takes the whole queue in a
single pass and the derive tag is idempotent, so N single requests landing in one tick already cost
exactly one derive. What the batch gives a caller is a moment at which "all of this holds", which two
completions cannot say.

**`Get_LinkResolution(volume, id)` is what one link RESOLVED to on the field currently published**, flat
and reflected: both end statuses, both flat plates, resolved yes/no and live yes/no in one read.
Every index in it — the plates above all — is valid only against that one publish, exactly like a
reachability label, so it is a snapshot of one call and never something to hold. An id the published field
carries no entry for reads as the default (no plates, `NoSurface` at both ends, neither resolved nor live),
and so does every id while nothing is published at all: a resolution is a property of a publish, and there
is no publish to have one against. The authored record is what survives a rebuild, and `TryGet_LinkRecord`
is where that is read.

**A route carries WHERE it crossed, as a parallel array.** `FCk_GroundNavPath_Result` gains
`TArray<FCk_GroundNavPath_LinkWaypoint> _LinkWaypoints`, keyed by index into `_Waypoints` — `{waypoint
index, link id, Entry|Exit, entry direction, distance from start}`. Parallel rather than a richer element
type for `_Waypoints` itself, so a consumer that reads only the locations reads exactly the array it read
before links existed and a route that crosses none carries no second array at all. The id is the STABLE
authored one and never the field-local index into `_ResolvedLinks`, because an installed path outlives the
field it was planned against and that array is re-derived wholesale on every publish. The two fields are
stamped on the internal `FCk_GroundNav_PathWaypoint` where the pinned point is recognised — the same
exact-equality rule `Get_CornerOffset` already uses — so they survive skip-first and the corner offset by
riding per waypoint, and they are collapsed into the array at the flatten in `DoPublish_Success`.
`_Waypoints` is byte-identical to what it was before any of this.

Two free functions in `Path/CkGroundNavPath_Utils.h` read it, with `BlueprintPure` wrappers taking the
path handle: `Get_LinksOnPath` answers the entry/exit pairs in walk order, and `TryGet_NextLinkBeyond`
answers the first link stepped onto strictly BEYOND a distance already walked (an invalid id means none) —
strictly, so a body standing exactly on an entry is told about the link AFTER the one it is already on.
Both are pure over the result's own metadata: no field, no world, no registry, so a test asks them of a
value it wrote by hand. An entry with no exit after it answers as an OPEN span rather than being dropped,
which is what a partial route that stopped on a link produces. The distance both report is the carried
`_DistanceFromStartUu` and never a second integration of the polyline — the post-process already answered
that number, and integrating it again would be a second definition of it.

**The veto rides the QUERY, never the field.** `FCk_Request_GroundNavPath_FindPath` carries
`TSet<int32> _DeniedLinkIds`, `FGameplayTagContainer _DeniedLinkUserTypeTags` (matched with `HasTag`
against the link's authored `_UserTypeTag`, so a parent tag denies every link under it and one tag says
"this body cannot use ladders" without naming any id) and `TMap<int32, float> _LinkCostMultipliers`, which
REPLACES the authored multiplier for that id on this query alone. A denied link is SKIPPED in `Neighbors`,
where the crossing would have been admitted — no node is minted — so the answer routes around it or does
not exist, and it is never merely dearer; a corridor that came back can never be holding one. The rewrite
is applied at `Cost`'s single call site of `Get_LinkTraversalCost`. A multiplier below 1.0 is REFUSED at
the request boundary with `Failed_NotEnqueued` rather than clamped: every edge must cost at least the
distance it covers or the search's Euclidean heuristic stops being admissible, and clamping it somewhere
the caller cannot see would hide that. None of this touches the field — what a link joins, and every
reachability label that follows from it, is the same for every agent, which is exactly why the veto can be
per-body at all. The neutral `FCk_Nav_QueryFilterOverlay` is deliberately NOT extended: it is tag-keyed,
and an `int32` id has no place in it.

**Invalidation is exact for a link-only publish and bounds everywhere else.** A published plan caches
`_LastCorridorLinkIds` — the stable ids of the crossings it actually took, resolved against its own field —
beside `_LastCorridorKeys`. The link derive publishes a GroundNav-side note on the world-field registry
entry describing the RUN of link-only publishes since the last geometry publish:
`{_Epoch, _LastGeometryEpoch, _ChangedLinkIdsSinceGeometry}`. A build or a repair resets the accumulation
and stamps `_LastGeometryEpoch` with its own epoch; a link derive appends its changed ids to it.
`FProcessor_GroundNavPath_InvalidateOnRebuilt` narrows ONLY when the note accounts for everything the
corridor has missed — the note's epoch is the field's, and `_LastGeometryEpoch` is not newer than the
corridor's own epoch — and then flags iff the corridor's cached ids intersect the accumulated ones.
Anything else falls to the bounds floor the pass answered with before there were links at all. The run,
rather than "the newest publish was link-only", is what makes it sound: a repair and a derive publishing
in one tick leaves the repair's ground unaccounted for and correctly takes the floor, while two toggles
landing before an agent replans stay exact. The consequence worth stating plainly is the one the pin
rests on — an agent whose route crosses a link that was just disabled replans exactly once, and an agent
whose route crosses nothing does not replan at all even when it is walking inside the very tile the link's
end resolved into.

Both directions of that follow from the ids, not from geometry, and the second one bites: an agent whose
current route crosses NO link hears nothing when a link is re-enabled either. Its corridor names no id to
intersect, so a shortcut appearing under it is not news the invalidator delivers. A consumer that wants an
agent to reconsider when new ground opens up asks for the replan itself.

Nothing here added a console command; `ck.GroundNav.LinksAt` and draw mode 7 already read the published
field and answer every one of these states.

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

## The bake identity

`ck::groundnav::Get_ContentFingerprint` reduces everything one bake's output depends on to a single
64-bit value. Equal fingerprints mean a rebuild would reproduce the same field, so the previous result
may be reused; any difference forces a bake. The header carries the FROZEN ENUMERATION of what goes
in — geometry, region, bake config, agent profile, markup, links, the merge tunables, the clearance
cap and the profile variants — and it is frozen in the sense that matters: anything a bake reads and
the list omits is a latent staleness bug, because the field would silently keep a result computed
under inputs that have since moved. **A new bake input means a new item on that list and its
perturbation case in `Test_GroundNav_Fingerprint.cpp`, in the same change** — never one without the
other.

The enumeration is answered by TWO functions over one implementation. `Get_InputFingerprint` answers
items 2 through 9 — everything AUTHORED, with the geometry left out — and `Get_ContentFingerprint`
answers all nine by handing the geometry hash to the same chain as its seed, so the two cannot drift
as items are added. The split exists because a volume can always answer the authored half and can
only sometimes reach a physics world, and because "the records moved" and "the world moved" are two
questions that one number could not tell apart.

Order independence is a contract. Triangles combine commutatively (by ADDITION, not XOR — under XOR a
duplicated triangle would cancel and a doubled surface would fingerprint as no surface), markup hashes
in ascending id, links in the order the list already carries them, and tags hash through their NAME
rather than `GetTypeHash`: an `FName`'s hash is an index into a per-process table and is not the same
number next run, which is exactly the property a fingerprint compared across sessions cannot have.

Geometry enters through a 64-bit value rather than a batch. `Get_GeometryHash` computes it from
triangles for a caller that has them; the overload taking the value directly is for one that never
holds a whole region's — which is every tiled build, since a slice collects one tile's halo and
nothing wider. What the volume's build stores there is the backend's world revision, the token
`ICk_GroundNav_GeometryBackend::Get_WorldRevision` already holds "the static world changed" against
and the same one a later slice fails closed on.

`FFragment_GroundNavVolume_BuiltField` carries the identity of the field it is PUBLISHING: the input
fingerprint that field went out with, beside the geometry revision it was produced against. A build
records both where it BEGINS — the finished field is a statement about the inputs the build
snapshotted, not about a record admitted while it ran — carries them on the build state, and hands
them over at the publish, so a build that FAILS leaves the standing field still naming what it was
actually produced from.

**Every publisher refreshes it**, through one helper the four of them share: the build, the local
repair, and the cost and link derives. The two derives have no backend and read no geometry — they
re-label ground that is already published — so they carry the revision FORWARD unchanged and refresh
only the authored half, which is exactly what moved. A derive that republished without restamping
would leave the volume claiming a record list it had itself already re-labelled past.

`Get_BuildFingerprint` reads the input half back. `Get_IsBuildCurrent` answers **has the world or the
authored records moved under the published field with no publish yet**: the stored revision against
the backend's current one, and the stored input fingerprint against the fingerprint of the volume's
current params and records. A volume with nothing published reads false, and so does one whose world
no backend can be made for — ground nothing can re-read has no honest answer but false. The revision
is monotonic, so ground that moved and moved back reads not-current: conservative on purpose, because
an unnecessary rebuild costs a bake where a field trusted past a change it never saw answers about
ground that is not there. Whether a paint is in EFFECT is `Get_IsMarkupLive`; whether anything is
still owed is `Get_IsSettled`.

---

## Failure is a status, never an empty field

`ECk_GroundNav_BakeStatus` and `ECk_GroundNav_BuildStatus` exist because a region with no floor and a
region whose bake could not run are identical in the data and could not be less alike to a path: the
first is a place with nowhere to walk, the second a place nothing is known about. A backend that
cannot answer yields `BackendUnavailable`, an exhausted budget yields `BudgetExhausted`, and neither
is ever published as a built field with no cells.

---

## Serialization

`Field/CkGroundNav_FieldSerialize.h` writes a published field to bytes and reads it back, at three
granularities: the whole field, one tile, and a spatial subset. The format is hand-rolled over an
`FArchive` — every scalar written out by name and by size — because the field types are plain C++ with
no `GENERATED_BODY`, and reflecting them would drag a `.generated.h` into the bake layer.

**What is persisted is the bake product and the params that produced it**: the tiles as the field
holds them — cells, clearance, plates, portals, boundary runs, seam stubs — plus `_Params` and the
open-body diagnostics. **Everything a composition DERIVES is re-derived on load**, through the same
pure derives a bake runs and in the same order: `DoDerive_SeamPortals`, then `DoResolve_Links`, then
`DoLabel_Reachability` (which numbers the plate offsets on its way). The seam portals, the tile edge
boundary, the resolved links, the plate offsets, the reachability labels and the per-component open
flags are therefore never read out of a blob, and a loaded field cannot carry a crossing or a label
the tiles beside it do not support. It keeps the format small as well: the derived arrays are the
large half of a field and none of them is an independent fact.

**Nothing process-relative is persisted.** An `FName` and an `FGameplayTag` are indices into a
per-process table, so a blob carries a NAME TABLE — every tag the field's plates and records name,
written once as a string and addressed everywhere else as an `int32` index into it. The table is
SORTED BY STRING rather than kept in encounter order, so two fields whose content matches produce the
same table whatever order their records were authored in. An absolute UTC cook date in the header is
admissible, and is the only run of bytes two writes of one field may differ in.
`TIsPersistableValue` (`Field/CkGroundNav_FieldSerializeTraits.h`) is the fence, and it is stricter
than the debug capture's beside it: it REJECTS `FName`, `FGameplayTag` and `FGameplayTagContainer`
where that one admits them, because a capture is read by the process that made it and a blob is not.
One `static_assert` per persisted type lists that type's members against the fence, naming a tag
member by the `int32` the blob carries in its place — a member added to any of those types belongs on
its list in the same change.

**Granularity is on the FIXED lattice.** The lattice rides the header, so a per-tile blob names the
lattice it belongs to and a read into a field divided differently answers `LatticeMismatch` rather
than laying cells over ground they were never baked from. A SUBSET is a whole-field blob whose absent
tiles are written PRESENT and Unbuilt, holding no cells and no plates: the loaded field is a valid
field of the same lattice with fewer built tiles, nothing is renumbered, and the crossings and links
that touched an absent tile are gone for free — the reader re-derives both, an unbuilt neighbour
yields no seam portal, and a link end over unbuilt ground resolves to nothing and is counted.

**A blob a reader cannot use is refused with a status, never an ensure.**
`ECk_GroundNav_LoadStatus` answers `WrongMagic`, `WrongVersion`, `Truncated`, `UnknownTag`,
`LatticeMismatch` or `Corrupt`, and the caller's field is left untouched in every case but `Loaded`
— a fallback needs something to fall back TO. `Corrupt` is the one the reader DERIVES rather than
reads: every floating-point member is judged finite, and every rotation judged a unit quaternion,
before the value it belongs to is constructed, because a NaN that reached a bound or a transform
compares false against everything it is tested with and would stop the field deciding anything there
in silence. Counts are bounded the same way — by the bytes actually left divided by the smallest one
element can be — so a corrupt count asks for a reservation the size of the blob rather than the size
of the number its bytes happened to spell. A cook older than the code reading it is an ordinary state of a
shipped game and not a defect. `kFieldBlobFormatVersion` is BUMPED WHENEVER A BYTE MOVES: a blob
decoded past a version it does not match reads its members at the wrong offsets and produces a field
that is plausible and wrong.

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

The draw is RETAINED. `Make_DebugSnapshotDrawBuild` turns a capture and a selection (the mode plus the
three overlay toggles) into an `FCk_GroundNav_DebugDrawBuild` — world-space lines and boxes beside a
per-kind tally — and `Do_PublishRetainedDebugDraw` emits that build as CkPmg line sets parented to the
world's transient entity, chunked at 128 primitives per set. `Do_UpdateRetainedDebugDraw` is the
per-frame form: it rebuilds ONLY when the capture's `FCk_GroundNav_DebugSnapshotCacheKey` or the
selection changes, so a caller drawing a field that is standing still pays for one rebuild and then
nothing. `Do_ReleaseRetainedDebugDraw` — what `ck.GroundNav.Clear` and
`ck.GroundNav.Debug.RetainedDraw 0` reach — destroys the sets; a world's own teardown destroys them
with it. Query commands publish into their own group, so `PathAt` answering does not erase the field
view it was asked about.

Retained is what makes a Test build draw at all: the engine's immediate helpers compile out where
`ENABLE_DRAW_DEBUG` is off, and a view that silently drew nothing in exactly the configuration a
packaged check runs would report a baked level as unbaked. What stays immediate is what PMG's line
tier has no equivalent for — every text label, the per-cell points of the clearance, layer and
rejected views, the link and waypoint arrowheads, and the query commands' spheres — so a build
without them loses the labelling and keeps the geometry.

Because a build is a pure function of a capture and a selection, what a mode DRAWS is assertable
without a world: `FCk_GroundNav_DebugDrawTally` counts plate outlines, portal segments, boundary runs
and link spans separately, and those counts are the capture's own counts. A capture that is not
drawable emits its region box and its open bodies and nothing else, which is the same rule the status
vocabulary states: failure is a status, never an empty scene.

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
| `ck.GroundNav.LinksAt <X> <Y> <Z>` | every navigation link the world's volumes hold — id, endpoints, direction, both cost multipliers, clearance, tags, enabled, requested-at epoch, and live yes/no — then, per end, the status the last resolution gave it with the tile, layer and plate it landed on, and the count of links with an end that found no ground. Every volume is listed with where the point falls on it, because a link authored before anything baked lives on a volume that covers nothing yet. An end reading `Unbuilt` is HELD, not dropped: the next publish over that tile resolves it. Every resolved link also outlines in the world — green traversable, grey disabled, orange an end over unbaked ground, red an end with no ground at all. Reads the volumes' PUBLISHED fields, so `BakeFieldAt` is not needed |
| `ck.GroundNav.Invalidation` | every cached path corridor in the world and what a republish is measured against: the corridor box the invalidator intersects, what it was inflated by, the epoch the plan was made on against the epoch the field covering it has published, and whether the agent is already flagged for a repath. A corridor whose own epoch is not behind the field's is one no queued rebuild can be news to, which is the first thing the invalidator decides. Also reports the ground each field last published changed, the local repair a volume has open with its tile count, any dirty ground still waiting for one, how many rebuild boxes are pushed but not broadcast yet, and whether either gate below is bypassing the answer — a repair being the other thing a republish comes out of. Outlines every corridor in cyan, every changed-bounds box in orange, and the repair's ground in green. Reads the PUBLISHED fields, so `BakeFieldAt` is not needed |
| `ck.GroundNav.RepairAt <X> <Y> <Z> <HALF>` | declare a box of ground no longer trustworthy and ask every volume it reaches for a LOCAL repair of exactly that ground. Per volume it reports the tiles the box would select (through the same pure `Get_RepairTileIndices` the repair itself uses), the dirty ground already pending on it, and whether a repair is already in flight — then logs that volume's own outcome when the repair ENDS, which is ticks away and never the moment the box is accepted. A volume whose published field does not reach the box is listed and left alone. Reads the volumes' PUBLISHED fields, so `BakeFieldAt` is not needed |
| `ck.GroundNav.Debug.DrawMarkup` / `.MarkupLiveGate` | `DrawMarkup` (default 1) outlines markup in the plate view and in `PathAt`/`FloodAt`: impassable red, cost amber with its multiplier, disabled dashed grey. `MarkupLiveGate` (default 1) at 0 forces GroundNav's `Get_IsMarkupLive` true — the bypass a paint-then-repath race pin must FAIL under to be evidence |
| `ck.GroundNav.Debug.DrawLinks` | (default 1) draws the links the world's published fields resolved, in EVERY draw mode and in `PathAt`/`FloodAt`: green traversable, grey a record the author disabled, orange an end over ground nobody has baked yet, red an end that found no ground at all — an arrowhead per direction the link may be walked, a tick at each resolved end, and the id at the midpoint. It gates COLLECTION, so a view drawn with it off carries no links rather than hiding ones it holds |
| `ck.GroundNav.Debug.DrawInvalidation` | (default 0) draws the invalidation state in the plate view: every cached corridor box in cyan, thicker where it is flagged for a repath and labelled with its two epochs and its inflation, and the ground each published field last reported changed in orange. The orange box carries a short lifetime of its own because it describes ONE publish. Off by default — a corridor is per AGENT, so a crowd draws one box each |
| `ck.GroundNav.Debug.RepairHighlightSeconds` | (default 2.0) how long the LABEL on an open local repair stays up, beside the green tiles it is re-baking, its dirty box and the dashed box of ground still waiting for one — all drawn under `DrawInvalidation`. A short lifetime of its own because a repair's tile set describes ONE publish and stops being true the moment the next slice lands, and a repair that finishes inside a frame would otherwise leave nothing to catch. The green geometry is retained rather than timed, so it stands until the next rebuild replaces it |
| `ck.GroundNav.Debug.RetainedDraw` | (default 1) holds the LINE geometry of every view — the region box, the plates, the crossings, the tile lattice and its seams, the boundary runs, the links, the markup and invalidation outlines, and the `PathAt`/`ReachAt` overlays — as CkPmg retained line sets instead of immediate debug lines. At 0 every set the world holds is released and nothing further is emitted: a retained view stands until something releases it, so switching it off is how it is put away, and `ck.GroundNav.Clear` releases both tiers at once |
| `ck.GroundNav.Debug.RepathOnRebuild` | (default 1) flags an agent for a repath when a published rebuild meets its cached corridor, which is the shipping behaviour. At 0 the invalidator flags nobody however much ground moved — the bypass a rebuild-then-repath pin must FAIL under to be evidence |

The query commands read the field the last `BakeFieldAt` kept for THIS world (each world keeps its own, dropped with it); `Bake`/`BakeAt` produce a region snapshot
with no field to query. The body radius every query uses is `ck.GroundNav.Debug.AgentRadiusUu`.

Draw modes (`ck.GroundNav.Debug.Mode`): 0 plates, 1 clearance ramp, 2 layers, 3 the cells the filters
rejected, 4 the crossings between plates, 5 the tile lattice and the seams between tiles, 6 the plate
edges nothing crosses (rim runs in orange), 7 the links the published fields resolved over dimmed plates.
Mode 7 draws no link the other modes do not already carry — links are overlaid in every mode while
`ck.GroundNav.Debug.DrawLinks` is on — what it adds is the absence of everything else, which is the only
way to read an authored crossing against the two pieces of ground it joins. In mode 5 the tiles
carrying the field's newest epoch tint green and label it, which is exactly the set the last publish re-baked — a field whose built tiles all
share one epoch is all of its own news and tints none of them. Mode 3 is the only view that shows what
a filter *costs* — a ledge sensitivity tuned too tight and a world that genuinely has no floor produce
an identical walkable set and differ only in what was thrown away.

**Read the `region` and `lattice` lines of the summary before forming any hypothesis about a bake.**
Every per-cell count is bounded by the lattice the summary prints, and halving the cell size quadruples
that lattice; a count read against the wrong cell size has already sent one investigation down a hole.

`FCk_GroundNav_DebugSnapshot` is the copy boundary: a value-only structure holding no world, no actor,
no handle and no span field, so a viewer can draw it a frame later or after its world is gone. Value-only
is a compile-time claim rather than a convention — `TIsDebugSnapshotValue` says what a member may BE, and
one `static_assert` per captured type lists that type's members against it, so a `TObjectPtr`, a
`TWeakObjectPtr`, a `TSharedPtr`, an `FCk_Handle` or a raw pointer named on any of those lists fails the build
instead of failing in a torn-down world. The lists are what make the claim, so a member added to a captured
type belongs on its list in the same change.

A capture reports itself through five statuses and no others — `NeverBuilt`, `BackendUnavailable`,
`NoGeometryInRegion`, `Failed`, `Current` — and only `Current` is drawable. That is the vocabulary the
draw modes and query commands above are read under: an empty scene is never the answer, so a viewer
holding no cells is holding a status that says why.

Four of those five are decided in ONE place: `Make_DebugSnapshotFromBackend`
(`Debug/CkGroundNav_DebugSnapshot.h`), which takes an `ICk_GroundNav_GeometryBackend&` and nothing
else — no world, no registry, no actor. `Make_DebugSnapshotFromWorld` is a thin caller that resolves
the Jolt backend and delegates, so a backend that cannot reach a physics world answers
`BackendUnavailable` through the same derivation a stub does rather than through a check of its own.
That is what lets each status be PINNED: the stub backend drives an empty region to
`NoGeometryInRegion` and a refused config to `Failed` headless, against the identical code the live
bake runs. `NeverBuilt` needs no producer — it is what a capture nobody filled in already reads as.

`FCk_GroundNav_DebugSnapshotCache` keeps the last capture beside the key it was taken under: the world's
name, the volume entity's number, that number's version, the newest tile epoch and the world's surface
revision, all values, so a key outlives the volume it names and can still be compared after it is gone.
The number carries its version because a number on its own is a slot the next volume inherits, and it
carries the world's name because two PIE worlds can hold the same entity number at the same epoch. A reader checks the key first and enumerates
only what `TryGet_Current` hands back for it; `Replace` swaps the key and the whole capture together,
exactly as a publish swaps a whole field, because a partial update is the corruption both shapes exist to
make unrepresentable.

`UCk_Utils_GroundNavPath_UE::Get_Diagnostics` is the same boundary one agent wide. It hands back
`FFragment_GroundNavPath_Diagnostics`, which a small pass in `Path/` copies onto every entity holding
the path feature once a tick: which provider answers that agent's world, the profile its cached
corridor was planned for, the verdict of the last finished episode carried across the search that
follows it, how many waypoints the slot is publishing, the authored link ids that corridor crosses
with the epoch it was found on, whether the agent stands flagged for a repath, and the `FCk_Time` the
plan was dated at by the world it was planned in. All values, so a viewer reads one after the agent is gone. Where the body IS
along that route is deliberately absent: the cursor belongs to the crowd agent walking it, and this
carries what GroundNav owns.

---

## Cooked assets

The cooked form of a baked field is data assets, one per built tile. A
`UCk_GroundNav_CookedTile_UE` (`Cook/CkGroundNav_CookedTile.h`) carries the tile's serialized blob,
the blob format version it was written under, its coord in the lattice, the content fingerprint of
the bake it came out of, and the lattice itself — origin, divisions, tile size, cell size, cell height
— as an `FCk_GroundNav_CookedLatticeKey` of plain values. The lattice rides every tile rather than
only the collection it belongs to because a tile asset is loadable on its own: read into a field
divided differently it would lay its cells over ground it was never baked from, and every index it
carries would name something else. `Get_IsCompatibleWith` answers the version question by exact
equality — a format the reader does not speak is refused, never reinterpreted.

The properties are `VisibleAnywhere` rather than `EditAnywhere` throughout — read-only in the details
panel, not read-only on disk. The cook writes every one of them through the setters; what the
specifier buys is that a human cannot edit a coord, a lattice or a fingerprint into disagreeing with
the bytes it sits next to, which nothing downstream could tell. The writing side lives in
`CkGroundNavEditor`, an UncookedOnly module; the asset types stay in the runtime module, because the
game loads them and only the editor writes them.

**A volume's cook identity is a NAME it authors.** `FCk_Fragment_GroundNavVolume_ParamsData::_CookKey`
is that name, and **None means runtime-only**: no cooked field is ever written for such a volume and
none is ever looked up for it, which is the honest answer for every gym, test and prototype volume
rather than a key invented on their behalf. Nothing else about a volume is stable enough to key on —
the params carry bounds, config and profile, the world-field registry keys on a runtime handle, and
there is no volume actor. Two volumes in one world carrying the SAME key is an admission failure,
refused where the params are judged and again where a build is asked for: a duplicate would have the
two of them writing over each other's tiles and reading back whichever landed last. None is exempt,
because it is not a key.

**A key and a profile variant cannot both stand**, refused at those same two sites. An index names ONE
field for a volume, so a volume reading its ground from a cook has no field under any variant's tag —
and a query naming one is answered from nothing rather than from the default's ground, which would
walk an agent up a step its own profile cannot climb. Refusing the two together is what tells an
author which of them to give up; dropping the variants quietly at the load would not.

`UCk_GroundNav_CookedFieldIndex_UE` (`Cook/CkGroundNav_CookedFieldIndex.h`) is one volume's cooked
field: the level package the cook ran over, the volume's cook key, the INPUT fingerprint of the bake,
the blob format version, the lattice, and the tiles as soft references in the lattice's own tile-index
order. Soft, because hard ones would load the whole field with the index, which is the cost the cooked
form exists to avoid. The geometry half of the bake identity is deliberately absent for now — it
belongs beside the fingerprint the day a cook can record the world revision it ran against.

Nothing hard-references a cooked asset, so the PATH is the reference. `Get_CookedIndexAssetPath` and
`Get_CookedTileAssetPath` are the convention, mirroring CkJolt's cooked world index: the level
package's path under the content root, then `GroundNavIndex_<CookKey>` or
`GroundNavTile_<CookKey>_<X>_<Y>`. The key is in the asset NAME rather than in the directory, so one
level's volumes sit beside each other. `Get_PackageLookupKey` normalises a package name into the form
the cook recorded — PIE renames every level package, the cook only ever runs on non-PIE worlds, and a
raw PIE name would match nothing and degrade silently to a runtime bake. The content root is a
constant in that header today: CkGroundNav has no project-settings object to hang one on, and it
belongs beside CkJolt's own `_CookedDataRootPath` the moment the module has one.

**`ECk_GroundNav_CookStatus` is the vocabulary, and it is four values wide.** `RuntimeOnly` — the
volume authored no key, so nothing was ever looked up and nothing is ever written. `MissingCook` — a
key is authored and no index exists at the convention path for {this level package, that key};
absence is LEGAL, a level opts into cooked ground. `StaleCook` — an index exists and cannot be used:
its fingerprint names inputs that have since moved, its format version is one the reader does not
speak, its lattice is not this volume's, or a tile it names would not load. `Cooked` — the published
field came out of the cook. `UCk_Utils_GroundNavVolume_UE::Get_CookStatus` reads it back in all three
environments.

**The fallback is STATE-SELECTED, not compiled** — the shape `CkJoltStaticWorld_Subsystem`'s
`Get_UsesCookedData()` branch has, with no `#if` anywhere in it. `FProcessor_GroundNavVolume_Setup`,
after the volume is admitted, walks the states in the order they rule one another out: no key →
`RuntimeOnly`, arm the build as before; no index → `MissingCook`, arm the build as before;
`Try_LoadCookedField` refuses → `StaleCook`, arm the build as before; the load holds → `Cooked`,
publish that field exactly as the build's publish does and arm nothing. `_AutoBuildOnSetup` is moot
on the last of those: it says whether the volume bakes itself unasked, and the ground is already
published. The variant map goes out empty there, and by ADMISSION rather than by a decision at the
publish: a volume carrying a cook key cannot carry a profile variant. A cooked field participates in
repair, in the cost derive and in the link derive like any other published field — it is a field, and
nothing downstream of the publish knows where it came from.

**The cooked publish RESTAMPS every tile's epoch** to the one it is going out under, exactly as a bake
stamps every tile it builds. The loaded tiles carry the COOK's epochs, which count a run of publishes
this volume never made, so without the restamp `Get_ChangedTileBounds` would find no tile carrying the
new epoch and the `Request_NotifySurfaceRebuilt` bounds would be the empty box. With it they are the
union of every tile the cook held — a fresh publish of everything, which is what a cooked publish is.

The load itself is PURE and lives apart from the world for that reason.
`Cook/CkGroundNav_CookedFieldLoad.h` splits it in two: `Find_CookedFieldIndex(world, cookKey)`
resolves the persistent level's package through `Get_LevelPackageKey` — the one derivation of it,
`Get_PackageLookupKey` over the persistent level — and the path convention, and loads the asset; null
is `MissingCook`. `Try_LoadCookedField(index, levelPackage, cookKey, params, inputFingerprint,
outField)` reaches no world at all. Nothing in it ensures, and `OutField` is untouched unless the whole
load held — a cook older than the code reading it is an ordinary state of a shipped game whose answer
is to bake at runtime, and a caller falling back needs something to fall back TO. All of which is what
makes every refusal assertable headless against assets built in a transient package.

**What it judges, in the order that pays least.** First the index's own IDENTITY — the level package
it records and the cook key it was written for, against the ones the caller asked for. A cooked asset
is reached by PATH and by nothing else, so an index moved, renamed or copied in from another level
would answer the lookup while being internally consistent about a volume nobody asked about. Then the
index's format version, its fingerprint and its lattice, all before a single tile is resolved. Then,
per tile, what the ASSET claims about the blob it carries: its format version, its lattice against the
index's, its coord against the slot it is listed for (`_Tiles` is in the lattice's flat tile-index
order), and its fingerprint against the index's. The blob's own header answers for the bytes below it
— magic, version, truncation, tags, lattice — and the serializer is what asks those; these four are
the claims sitting BESIDE the bytes, and until they are compared a tile from another bake, another
lattice or another slot loads perfectly and lands over ground it was never baked from.

**The composition runs ONCE.** `Read_TileInto` takes an `ECk_GroundNav_ComposeOnLoad` and the loader
passes `Deferred` for every tile, then calls `Compose_LoadedField` after the last one. The derives are
whole-field — the seam portals, the resolved links and the reachability labels are re-derived over
every tile — so composing per tile would run them once per tile and keep only the last run's answer.
A `Deferred` reader OWES that call: a field left uncomposed carries tiles no crossing, no resolved
link and no label supports.

**A cooked field carries NO open body.** The per-tile form holds no closure report, and the check
belongs to the run that read the meshes — which was the cook's, and is the cooker's to report.
`Try_LoadCookedField` leaves `_OpenBodies` empty rather than reconstructing a list it cannot know is
still true.

`FFragment_GroundNavVolume_BuiltField::_CookStatus` carries the answer. SETUP answers it — the cook is
resolved there and nowhere else. A runtime bake that followed a `MissingCook` or a `StaleCook` KEEPS
that status: it still names why the ground standing there is not the cook's. A runtime build that
publishes over a `Cooked` field DEMOTES it to `StaleCook`, because the ground standing there stopped
being the cook's the moment that field was replaced, and only a fresh Setup reads one again. A repair
or a derive keeps whatever stood, for the reason they carry the geometry revision forward — they
re-label ground somebody else published.

**Health is not provenance.** `Do_ProviderHealth` in `Facade/CkGroundNav_NavSurfaceAdapter.cpp` is
UNCHANGED by any of this and must stay so. It answers whether a world has usable ground; where that
ground came from does not bear on the question, and a level that bakes at runtime because nobody
cooked it is `Ready` in exactly the way a cooked one is. Folding the two would make a missing cook
read as a broken nav surface, which is a different and much louder claim.

---

## Anti-patterns

- **Do not include a Jolt header here.** Geometry comes from CkJolt's JPH-free surface. The bake math
  is free of engine types entirely — no `UWorld`, no `FCk_Handle`, no `UObject` under `Bake/`. Includes
  run one way for the same reason: no `Field/` HEADER includes `Query/`, and
  `Field/CkGroundNav_FieldLinks.cpp` is the one implementation file under `Field/` that does, because
  resolving an authored link endpoint IS a projection query over the field being composed.
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
- **Do not resolve a link against a field other than the one it is published in.** A `_ResolvedLinks`
  entry, and the `_LinkIndex` a crossing carries, address the plate numbering of exactly one field;
  carried across a publish they name a different link or none. The array is re-derived wholesale on every
  publish, for the same reason the reachability labels are.
- **Do not compare a bake against a tolerance where it should be exact.** The distance transform, the
  decomposition and the portal extraction are all deterministic; a test that accepts "close enough"
  is a test that will pass through a propagation bug.
