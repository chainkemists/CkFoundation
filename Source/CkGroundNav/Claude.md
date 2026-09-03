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

## Failure is a status, never an empty field

`ECk_GroundNav_BakeStatus` and `ECk_GroundNav_BuildStatus` exist because a region with no floor and a
region whose bake could not run are identical in the data and could not be less alike to a path: the
first is a place with nowhere to walk, the second a place nothing is known about. A backend that
cannot answer yields `BackendUnavailable`, an exhausted budget yields `BudgetExhausted`, and neither
is ever published as a built field with no cells.

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
| `ck.GroundNav.FloodAt <X> <Y> <Z> <R>` | flood fill from a point out to a walked distance R: reached plates coloured by entry distance, settled crossings traced back to the source |

The query commands read the field the last `BakeFieldAt` kept; `Bake`/`BakeAt` produce a region snapshot
with no field to query. The body radius every query uses is `ck.GroundNav.Debug.AgentRadiusUu`.

Draw modes (`ck.GroundNav.Debug.Mode`): 0 plates, 1 clearance ramp, 2 layers, 3 the cells the filters
rejected, 4 the crossings between plates, 5 the tile lattice and the seams between tiles, 6 the plate
edges nothing crosses (rim runs in orange). Mode 3 is the only view that shows what a filter *costs* — a
ledge sensitivity tuned too tight and a world that genuinely has no floor produce an identical walkable
set and differ only in what was thrown away.

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
