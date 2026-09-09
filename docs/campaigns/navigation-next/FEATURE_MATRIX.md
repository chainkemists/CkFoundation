# navigation-next — FEATURE MATRIX (capability freeze)

> **Status:** P1 deliverable, planning session 1 (2026-08-31). This is the campaign's **capability
> freeze**: the complete enumeration of what a finished CkGroundNav provider plus the
> provider-neutral ground-query facade must do in order to (a) become the default path provider for
> grounded agents and then (b) let Unreal Navigation/Recast be retired from the Ck ecosystem
> entirely, per [NN-D1].
>
> **Current execution status (2026-09-08):** GroundNav is the default runtime provider while Recast remains selectable for A/B and shadow comparison. The current working tree implements and locally verifies 7C streaming composition, 7D invokers, 7E selector/chunk-asset support, and 7F aligned transformed-field merge. The final GroundNav-default suite and Recast-default Nav/Crowd/Queue lanes have been run and compared by name; exact results are in `PROGRESS.md` Gate81–89. Durable World Partition source manifests/runtime lifecycle binding, editor/U5 checks, and packaged Development/Test/Shipping evidence remain open. Source is uncommitted; continue from `CONTINUATION_PROMPT_GroundNavStreamingAndAcceptance.md`.
>
> **Representation is already decided** ([NN-D7], `REPRESENTATION.md`): layered ground field →
> merged plates → portal graph, chunked in tiles, per-cell clearance from a distance transform,
> immutable publish + epochs, local repair, CkAStar-driven search with portal funnelling. Every
> "What WE build" cell below is grounded in that choice and in the in-house assets inventoried in
> `research/R4-ck-native-assets.md`. **The seam is already decided** ([NN-D8],
> `MIGRATION_SEAM.md`). Nothing in this file reopens either. If a feature seems to require a
> different representation or a different seam, that is a fork for the orchestrator, not a local
> decision.

## How to read this file

Every feature carries five cells:

- **What it is** — the behavioral spec: inputs, outputs, failure modes, threading contract where
  one exists. Precise enough that a later agent can plan the feature from this file alone.
- **Why we need it** — the actual Ck consumer, with `file:line` from
  `research/R3-coupling-inventory.md` where available (line numbers are a snapshot @ CkFoundation
  `a25ec9539`; re-verify before executing).
- **What WE build** — the design sketch in terms of the chosen representation and our own assets
  (CkAStar, CkShapes, CkPmg, CkJolt's geometry-backend seam, immutable publish, epochs, plates,
  portals, clearance field, value-only snapshots).
- **Acceptance criteria** — measurable, naming the test layer per `R4 §7` (Layer 1 = hermetic C++
  automation under `CkTests/Private/UnitTests/<Module>/`; Layer 2 = PIE AngelScript autotests;
  Layer 3 = gym station). `[EDITOR-VERIFY]` marks anything only a human can confirm.
- **Provenance** — the public algorithm(s) / open-source project the implementation is built from.
  **Provenance policy ([NN-D4]): every algorithm in this campaign cites public literature or
  appropriately licensed open source.** The open-source `recastnavigation` project (zlib licence —
  the same code Unreal itself embeds) is a legitimate, citable source of technique; so is published
  academic and conference literature. A capability being *desirable* is never on its own a
  justification for a structural design — the structure comes from the cited public technique
  composed onto our own architecture.

### Tiers

| Tier | Meaning | Gate it serves |
|---|---|---|
| **Tier 1** | Required before CkGroundNav can be **promoted to default provider** for grounded agents. Without any one of these, promotion is blocked. | `MIGRATION_SEAM.md` §4 "Promotion to default provider requires all of" |
| **Tier 2** | Required before **Recast can be retired** (deps removed from `.Build.cs`, `CkNavmeshDebugDraw` deleted, adapter sites deleted). Not required for promotion — during Tier 2 the Recast adapter is still present as fallback and A/B oracle. | `MIGRATION_SEAM.md` §4 "Recast retirement requires additionally" |
| **Tier 3** | **Post-retirement backlog.** Genuine capability, not needed for parity with what Unreal Navigation gives us today. Each is planned, none is scheduled by this campaign. | — |

Feature ids are stable (`F<tier>.<n>`); PHASE docs and VALIDATION.md reference them.

### Phase mapping ([NN-D9])

| Phase | Features |
|---|---|
| P0 — contract neutralization + adapter | F1.27, F1.38, F1.39, F1.42 |
| P1 — bake core | F1.1–F1.12, F1.43 |
| P2 — query surface | F1.13–F1.22 |
| P3 — search + install + shadow | F1.23–F1.30, F1.40, F1.41 |
| P4 — dynamics / markup / repair | F1.31–F1.37 |
| P5 — links + PathNetwork migration + authoring | F2.1–F2.8 |
| P6 — debugger + gym | F2.9–F2.13 |
| P7 — persistence / cook / streaming | F2.14–F2.19 |
| P8 — parity / promotion / retirement | F2.20–F2.23 |

---

# TIER 1 — required for promotion to default provider

## Part A — Build pipeline

### F1.1 — Geometry collection through a physics-backend seam

**What it is.** Given a build region (a tile's bounds inflated by a border margin large enough that
every filter with a neighbourhood radius — clearance, ledge, step — is correct at the tile edge),
produce the set of static collision surfaces intersecting it, expressed as triangles or as analytic
shapes, in world space. Inputs: bounds, a collision-domain selector, an optional exclusion set.
Outputs: a geometry batch plus a canonical-order content fingerprint of that batch (F1.11).
Failure modes: backend unavailable → the tile bakes as *unbuilt*, never as *empty* (a build failure
must be a status, never a silently empty field). Threading: collection runs off the game thread and
must not observe live physics mutation — it is scheduled in the one window provably outside the
async physics step, exactly as the volumetric builder is.

**Why we need it.** Every consumer downstream of the bake. Today the equivalent work is inside
Unreal's navigation system and is invisible to us; the only Ck-side hook is
`Utils/CkNav_Utils.cpp:261-263` actor nav-registration, kept through migration by [NN-D8c].

**What WE build.** A 3-function JPH-free geometry backend header mirroring
`Backend/CkVoxelNav_GeometryBackend.h` + `_Jolt.h` / `_Stub.h`: enumerate-static-bodies-in-bounds,
fetch-shape-geometry, and a cheap bounds prefilter. The `_Stub` backend takes a hand-authored box
list so the whole bake is hermetically testable with no world, no ECS and no physics
(`R4 §1`, `§7`). Generation-1 domain is **static bodies only** — kinematic bodies are invisible to
the bake and are handled by dynamic markup (F1.31) instead, which is the same boundary CkVoxelNav
draws and test-pins. Instanced/foliage sources are collected once per source shape with a per-
instance transform, so an instanced forest costs one geometry fetch.

**Acceptance criteria.** Layer 1: a bake over a hand-authored box list produces a bit-identical
field across 100 runs (determinism); a body straddling a tile border produces identical cells in
both tiles' overlap region; a stub backend returning "unavailable" yields `Unbuilt`, and no tile is
ever `Built` with zero plates over non-empty geometry. Layer 1: instanced source of N instances
issues exactly one shape fetch.

**Provenance.** Standard broadphase-query-then-export geometry collection; no algorithmic novelty
claimed. Border-margin sizing follows the published navmesh-tiling practice used in the
open-source `recastnavigation` project (zlib).

---

### F1.2 — Span rasterization into a layered ground field

**What it is.** Rasterize collected geometry into per-column solid **spans** at a configured cell
size (XY) and cell height (Z). Triangles whose normal is within the profile's max-slope cone mark
walkable spans; steeper ones mark solid-but-not-walkable. Adjacent spans within the profile's climb
threshold merge. Output per column: an ordered, non-overlapping list of spans, each with a top
height, a walkable flag, and a quantized surface normal. Failure modes: degenerate/NaN triangles
are dropped with a counted diagnostic, never rasterized; a column count exceeding the tile budget
fails the tile with a status. **Input contract ([NN-D38]):** every Solid body must be a CLOSED mesh —
the rasterizer sees faces only and does not rasterize vertical faces, so a solid covers the floor
beneath it solely through its flush underside. Heightfields are Surface bodies and exempt. The
contract is enforced by a per-body closure check whose violations are recorded on the field, named in
one warning per build, and drawn in red by the debugger; they never fail the bake.

**Why we need it.** This is the substrate for every other Tier-1 feature: projection, containment,
raycast, boundary segments, clearance, plates, portals. Nothing above it works without it.

**What WE build.** A span rasterizer in the CkGroundNav math core — pure functions over a geometry
batch and a `FCk_GroundNav_BakeConfig`, no ECS, no `UWorld`, following the CkVoxelNav precedent
that "the voxelizer is usable with no ECS against hand-authored box lists" (`R4 §1`, `§7`). Spans
are stored per tile in flat value arrays indexed by stable integer ids (`FCk_GroundNav_TileId`,
column index, span index) — never pointers ([NN-D7] fences).

**Acceptance criteria.** Layer 1: a single 1000×1000×10 uu box at Z=0 rasterizes to exactly one
walkable span per covered column at the expected height (±half a cell height); a 45° ramp produces
a monotone height progression and a normal within tolerance of the analytic normal; two floors
1000 uu apart produce exactly two spans per column; a 30 uu step between adjacent columns merges
under a 40 uu climb threshold and does not merge under a 20 uu one.

**Provenance.** Span-based heightfield rasterization with walkable-slope marking and climb-
threshold span merging — the technique published and implemented in the open-source
`recastnavigation` project (zlib licence; Mononen's published talks and the surrounding navigation-
mesh literature).

---

### F1.3 — Walkability filtering

**What it is.** Post-rasterization passes that demote walkable spans that an agent cannot actually
stand on or leave:
- **Low-clearance filter** — a span whose vertical gap to the next span above is under the
  profile's height clearance is demoted.
- **Ledge filter** — a span whose neighbours differ in height by more than the climb threshold on
  enough sides is demoted (it is the lip of a drop, not standable ground).
- **Step / climb consistency** — connectivity between neighbouring spans is recorded only where the
  height delta is within the climb threshold and the slope-change between the two surface normals
  is within the profile's max slope-change.
- **Rough-perch tolerance** — an optional dilation that keeps a surface navigable across small
  rasterization-induced roughness rather than shattering it into disconnected fragments.
Output: per-span walkable flag plus a 4- (or 8-) neighbour connection mask.

**Why we need it.** Without ledge and clearance filtering, agents path onto surfaces they fall off
or into gaps they cannot fit through; `CkCrowdAgent_ConstrainToNavmesh_Processor` (R3, `108-143`,
`241`) is the single Transform writer for grounded agents and depends on the field's definition of
"standable" being conservative.

**What WE build.** Three ordered pure-function passes over the span field, each independently
testable, each configured only through the frozen `FCk_GroundNav_AgentProfile` tunables (F1.12).
The connection mask computed here is the *only* adjacency the plate merge (F1.6) and the distance
transform (F1.5) are permitted to consult, so "walkable" has exactly one definition in the codebase.

**Acceptance criteria.** Layer 1: a 100 uu-wide slot under a 150 uu ceiling is walkable for a 140 uu
profile and demoted for a 160 uu one; the top row of columns along a 500 uu cliff edge is demoted by
the ledge filter and the row behind it is not; a 5 uu sawtooth surface stays one connected region
under the rough-perch tolerance and shatters with the tolerance at zero.

**Provenance.** Low-clearance and ledge filtering over a span heightfield, and neighbour-
connectivity by climb threshold — the open-source `recastnavigation` project (zlib) and the
published navmesh-generation literature.

---

### F1.4 — Vertical layer extraction (overlapping floors)

**What it is.** Group the walkable spans of a tile into **layers** such that within one layer no
column carries more than one span, and spans that are mutually reachable across the connection mask
land in the same layer. Multi-storey buildings, bridges over walkways, and mezzanines therefore
resolve into distinct 2.5D sheets. Output: a layer index per walkable span, and per layer a
2D footprint. Failure mode: a span that cannot be assigned (a cycle in the footprint constraint) is
split into a new layer rather than dropped — a layer count is cheap, a lost floor is not.

**Why we need it.** [NN-D7] requirement 6: overlapping floors are a hard requirement of our
projects; a purely 2D ground field cannot serve a multi-storey building and CkCrowd's projection
recovery (R3, ConstrainToNavmesh `108-143`) would snap agents through floors.

**What WE build.** Flood-fill connected components over the span connection mask, then assign
components to layers by testing 2D-footprint overlap: a component joins the lowest-indexed layer
whose footprint bitset does not intersect it, otherwise it opens a new layer. Layer index is part of
the stable id triple (tile, layer, cell) — never a pointer, per the fences.

**Acceptance criteria.** Layer 1: a two-storey box building yields exactly 2 layers with disjoint
per-column occupancy; a spiral ramp that passes over its own lower run yields ≥2 layers and the ramp
remains one connected component across the layer split; a flat plane yields exactly 1 layer.

**Provenance.** Connected-component labelling (flood fill) plus footprint-bitset layer assignment —
the layered-heightfield approach documented in the open-source `recastnavigation` project (its
layer/tile-cache path, zlib) and in published navigation-mesh literature.

---

### F1.5 — Per-cell clearance field (distance transform)

**What it is.** For every walkable cell, the distance (in world units) to the nearest non-walkable
cell in the same layer — i.e. the largest circular agent radius that fits standing on that cell.
Computed by a two-pass chamfer distance transform over the layer's walkable mask, then scaled to
world units. Output: a clearance value per cell, and a **minimum clearance across each portal**
(F1.7). Failure mode: none — a fully enclosed layer yields large interior values, an isolated single
cell yields exactly one cell-size of clearance.

**Why we need it.** [NN-D7] chose clearance over per-radius erosion deliberately: **one bake serves
every agent radius**. It gives (a) radius filtering at query time (`clearance >= R`), (b)
wall-distance costing for free (F1.26), and (c) closes the transition-clearance hole that pure
cell-size filters have — the portal carries the min clearance across the crossing, so a wide plate
joined to another wide plate through a narrow doorway is correctly impassable for a fat agent.
CkCrowd agents already come in multiple radii and today share one Recast bake with per-agent
containment fudge (ConstrainToNavmesh's 4× radius recovery, R3 `108-143`).

**What WE build.** A chamfer distance transform over each layer's walkable mask, stored as a flat
per-cell array in the tile's finest tier (per `REPRESENTATION.md`'s memory mitigation: per-cell
arrays exist only for height and clearance; everything else lives at plate level). Portal min
clearance is derived during portal extraction, never stored redundantly.

**Acceptance criteria.** Layer 1: on a 1000×1000 uu open square with cell size 25 uu, the centre
cell's clearance equals 500 uu ± one cell diagonal; a 90 uu-wide corridor yields max clearance 45 uu
± one cell along its spine; the min clearance of the portal joining two rooms through a 60 uu door
is ≤30 uu regardless of the rooms' size. Layer 1: `clearance >= R` admits/rejects the exact set of
cells a brute-force O(n²) reference computes on a 64×64 fixture.

**Provenance.** Chamfer distance transform (Borgefors 1986, "Distance transformations in digital
images"); the same two-pass approach used for the distance field in the open-source
`recastnavigation` project (zlib).

---

### F1.6 — Merged-plate decomposition

**What it is.** Collapse the per-cell layer field into a small number of **plates**: maximal
axis-aligned rectangular groups of cells that are near-coplanar (their heights fit a plane within a
frozen tolerance and their normals lie within a frozen cone) and share policy (same area tag set,
same walkable status). Output per plate: a stable `FCk_GroundNav_PlateId`, an XY rectangle, a fitted
height plane, a representative normal, an area-tag/policy key, and min/max clearance. Failure mode:
a region that cannot be merged degenerates gracefully to one plate per cell — correct, just larger.

**Why we need it.** This is the collapse that makes search cheap: the same merge our volumetric
field already demonstrates (91,752 cells → 359 boxes on a 6400 uu scene, `REPRESENTATION.md`). The
A* in F1.23 runs over plates, not cells, which is what puts a multi-hundred-agent crowd inside the
per-frame query budget (`Get_MaxPathQueriesPerFrame` default 8, R3 §B).

**What WE build.** Greedy rectangle decomposition over each layer, seeded in a deterministic scan
order so the output is reproducible. **The merge criteria — plane-fit tolerance, normal cone,
policy equality — are frozen in PHASE_1 per [NN-D7]'s fences**; executors implement them and expose
the named tunables, they do not redefine "mergeable". Ramps and stairs are the interesting case: the
plane-fit tolerance is what keeps a staircase from either shattering (one plate per tread, funnel
still correct but search fatter) or over-merging (a plate whose interior height is wrong, funnel
incorrect). Both directions are test-pinned.

**Acceptance criteria.** Layer 1: a flat 100×100 cell plane merges to exactly 1 plate; a plane with
one hole merges to ≤5 plates; a 12-tread staircase merges to a bounded plate count (asserted range,
not a magic number) and every plate's max height error against the true surface is within the frozen
tolerance; the decomposition is identical across 100 runs and independent of geometry submission
order. Layer 1: a measured collapse ratio on a representative fixture is recorded in VALIDATION.md
as a tracked number (regression tripwire, not a pass/fail).

**Provenance.** Greedy rectangular decomposition of binary/attribute grids (greedy meshing as
documented in the public voxel-meshing literature; minimal rectangular partition literature, e.g.
Ferrari, Sankar & Sklansky 1984); least-squares plane fitting (standard linear algebra); our own
in-house merged-cell decomposition precedent (see `research/R4-ck-native-assets.md` §1).

---

### F1.7 — Portal extraction and the plate adjacency graph

**What it is.** For every pair of adjacent plates (within a layer, across a layer boundary where a
ramp changes layer, or across a tile boundary), emit a **portal**: the shared edge interval, its two
endpoints, the owning plate ids, the **minimum clearance across the crossing**, and a traversal
policy key. Output: a portal array per tile plus an adjacency table plate → portals. Failure modes:
a portal whose interval is shorter than one cell is still emitted (search may still need it) but
carries its true, tiny clearance; tile-boundary portals are only emitted once both tiles are built —
until then the boundary is a hard edge and paths across it fail as *unbuilt*, never as *blocked*.

**Why we need it.** The portal graph is the search space (F1.23) and the funnel input (F1.24). The
per-portal min clearance is the mechanism that makes one bake serve all radii (F1.5).

**What WE build.** Portal extraction as a derived pass — like everything else in the field, portals
are re-derived on repair, never patched ([NN-D7] fence). Cross-tile portals follow the chunk-portal
+ adjacency-table pattern already proven in `Chunk/CkVoxelNav_Chunk_Types.h:131,156` (`R4 §1`),
including the rule that partitioning is decided at composition, not inside a processor.

**Acceptance criteria.** Layer 1: two abutting rooms joined by one door yield exactly one portal
with min clearance equal to the door width/2 ± one cell; a plate fully enclosed by non-walkable
cells yields zero portals and forms its own reachability component (F1.9); a wall between two tiles
produces zero cross-tile portals and a floor across the same boundary produces a portal whose
interval matches the floor width; removing and re-adding a tile reproduces byte-identical portals.

**Provenance.** Portal graphs / portal-based spatial connectivity (long-standing public technique in
both rendering and path planning literature); shared-edge adjacency extraction from a cell
decomposition.

---

### F1.8 — Tiling, immutable publish, and epochs

**What it is.** The field is chunked into fixed-size tiles with stable integer ids. A completed tile
(or whole field) is published as an immutable value; readers take a shared reference and are
guaranteed a self-consistent structure for as long as they hold it. Each completed (re)build bumps
an **epoch**; staleness is *derived at the read boundary*, never stored. A chunked field's
fingerprint is the sum of its tiles' epochs, giving one monotone number to compare. Failure mode:
publishing never partially mutates a live structure — a failed build leaves the previous published
field intact and records a status.

**Why we need it.** (1) It is what makes off-game-thread queries (F1.20, the avoidance sampler at
R3 `CkCrowdAgent_AvoidanceSample_Processor.cpp:47,171-189`) safe without a lock discipline —
`REPRESENTATION.md` calls this out as requirement 4 falling out of the design. (2) It is what makes
repair (F1.35) unable to corrupt anything. (3) It gives path invalidation (F1.34) a cheap trigger.

**What WE build.** `TSharedPtr<const FCk_GroundNav_Field>` in a fragment slot, mirroring
`FFragment_VoxelNavVolume_BuiltOctree::_Octree` (`Volume/CkVoxelNavVolume_Fragment.h:64`) and the
epoch discipline at `Fragment.h:44-46,66` + `_AggregatedChunkEpochSum` at `:178` (`R4 §1`) — all of
it already test-pinned in the volumetric case. Illustrative shape, house style:

```cpp
namespace ck
{
    struct CKGROUNDNAV_API FFragment_GroundNavVolume_BuiltField
    {
        CK_GENERATED_BODY(FFragment_GroundNavVolume_BuiltField);

    private:
        TSharedPtr<const FCk_GroundNav_Field> _Field;
        FCk_GroundNav_Epoch                   _Epoch;

    public:
        CK_PROPERTY_GET(_Field);
        CK_PROPERTY_GET(_Epoch);
    };
}
```

**Acceptance criteria.** Layer 1: a reader holding a shared field across a full rebuild continues to
observe the old field's exact contents; the epoch after a rebuild is strictly greater; the aggregated
chunk-epoch sum is monotone across any interleaving of per-tile rebuilds; a build that fails leaves
the previously published field pointer unchanged and sets a failure status. Layer 1: no published
field contains a raw pointer or `UObject` reference (a compile-time or reflection-driven assertion).

**Provenance.** Immutable publish / copy-on-write snapshot publication and monotone version
counters — standard concurrent-data-structure practice; our own proven in-house pattern (`R4 §1`).

---

### F1.9 — Reachability components

**What it is.** A connected-component label per plate, computed over the portal graph subject to a
*profile-invariant* connectivity (i.e. ignoring per-query clearance/cost filters). Two points are
**definitely unreachable** if their plates carry different labels; equal labels mean *possibly*
reachable and a search is required. Output: a component id per plate, and a component count.
Failure mode: labels are only valid within one published field epoch and must be re-derived on
rebuild, never carried across.

**Why we need it.** Near-O(1) rejection of impossible goals. Today CkCrowd discovers unreachable
goals only by burning a full failed search — the `Stall_UnreachableGoalFailsBounded` autotest
(R3 §CkTests) exists precisely because that cost is visible. It also backs the facade's
`Get_IsReachable(from, to, agent)` ([NN-D8] Step 2).

**What WE build.** Union-find over portals during field assembly (cheap — plates are hundreds, not
tens of thousands), stored as a flat per-plate `int32` array in the published field. Note the
honest limitation, which must be documented in the API contract: components ignore per-agent
clearance, so a *different* label proves unreachability while the *same* label does not prove
reachability for a fat agent. A clearance-thresholded component set is a Tier-3 refinement (F3.14).

**Acceptance criteria.** Layer 1: two rooms with no connecting door carry different labels; opening a
door (rebuild) merges the labels; label assignment is deterministic and independent of tile build
order. Layer 2: `Stall_UnreachableGoalFailsBounded` fails in strictly fewer expanded nodes on
CkGroundNav than the recorded Recast baseline.

**Provenance.** Union-find / connected components (Tarjan; standard graph algorithms literature).

---

### F1.10 — Deterministic build budgeting

**What it is.** The bake is sliceable and budgeted by a **deterministic primary counter — geometry
probe count** — with wall-clock only as a secondary guard. A slice consumes at most N probes,
records where it stopped, and resumes with identical results to an unsliced bake. Output: a build
status and consumed-probe count per slice. Failure mode: exhausting a slice never publishes a
partial field.

**Why we need it.** Determinism is what makes bakes assertable in Layer 1 tests, and what stops the
"it passed on the build machine" class of flake. It is also the frame-time contract: a rebuild
triggered by a moved obstacle must not spike the game thread.

**What WE build.** Exactly the CkVoxelNav discipline: "probe COUNT is the primary budget
(deterministic, test-assertable); wall-clock only a guard" (`CkVoxelNav/CLAUDE.md:214-217`,
`R4 §1`), with the build processor scheduled in the window provably outside the async physics step.
Budget expressed with domain types (`FCk_Time` for the wall-clock guard, never a bare float).

**Acceptance criteria.** Layer 1: `SlicedBakeMatchesOneShotBake` over a fixture with ≥5 forced
slices; probe count for a given fixture + config is identical across runs and across slice sizes;
no publish occurs while a slice is outstanding.

**Provenance.** Time-sliced / budgeted incremental computation — standard; our own probe-budget
discipline (`R4 §1`) supplies the determinism contract.

---

### F1.11 — Content-hash dirty check (incremental rebuild admission)

**What it is.** Before rebuilding a tile, hash its input set in canonical order — collected geometry
identities and transforms, the tile's markup records, and the agent-profile config version. If the
hash matches the built tile's stored hash, skip the rebuild entirely and keep the epoch. Output: a
`Skipped` vs `Rebuilt` decision plus the new hash. Failure mode: hash collisions are addressed by
using a wide hash; a *conservative* mistake (rebuilding when nothing changed) is acceptable, a
*permissive* one (skipping a real change) is not — so anything the hash does not cover must force a
rebuild.

**Why we need it.** Level streaming and editor iteration re-trigger builds constantly; without an
admission check every stream-in pays a full bake. It is also the mechanism that makes the
"rebuild is idempotent" test assertion cheap.

**What WE build.** A canonical-order fingerprint computed during F1.1 and stored per tile in the
published field. The hash input list is a frozen, documented enumeration — adding a bake input
without adding it to the hash is the exact bug class this feature invites, so the PHASE doc requires
a Layer-1 test per hash input.

**Acceptance criteria.** Layer 1: rebuilding an unchanged tile skips and preserves the epoch; each
enumerated hash input, when perturbed, forces a rebuild (one assertion per input); changing
geometry submission order does not change the hash.

**Provenance.** Content-addressed / hash-based incremental invalidation (standard build-system and
caching technique).

---

### F1.12 — Agent profiles

**What it is.** The named parameter set that defines what "walkable" means: height clearance, step
(climb) height, max slope, max slope-change between neighbouring cells, ledge sensitivity, and the
rough-perch tolerance. **Radius is deliberately NOT a profile parameter** — it is a query-time
predicate against the clearance field (F1.5). A profile whose parameters genuinely change
walkability (a much shorter or much more agile agent) may justify a **second baked profile layer**;
that is a CTO decision, not an executor's ([NN-D7] fences). Output: a config value type consumed by
F1.2–F1.6. Failure mode: an invalid profile (negative clearance, slope > 90°, step > clearance) is
rejected at admission by `CK_ENSURE_IF_NOT` — never silently clamped.

**Why we need it.** CkCrowd runs agents of differing sizes today and pays for it in containment
fudge; the queue/formation and PathNetwork consumers both need "is this walkable *for this agent*".

**What WE build.** `FCk_GroundNav_AgentProfile` as a reflected params struct in the house shape
(private `_Members`, `CK_PROPERTY_GET` on essentials, `CK_DEFINE_CONSTRUCTORS` on the essential set),
with authored extents expressed through `CkShapes` types rather than bare floats (`R4 §3`), and
durations/budgets as `FCk_Time`. Derived cell-space values (clearance in cells, step in cells) are
computed once at bake admission and carried in a private derived struct, never recomputed per cell.

**Acceptance criteria.** Layer 1: the same geometry baked with two profiles differing only in
clearance yields the expected walkable-set difference; an invalid profile is rejected with no
partial state and no published field; profile config version participates in the content hash
(F1.11). Layer 3: the gym exposes a profile switch and the field visibly changes.

**Provenance.** Agent-parameterised navmesh generation (slope/step/clearance/ledge) as documented in
the open-source `recastnavigation` project (zlib) and the navmesh-generation literature.

---

## Part B — Query surface

> All Tier-1 queries operate against a **published immutable field snapshot** (F1.8) and are
> therefore callable off the game thread unless a cell says otherwise. All of them report **status
> enums, not bools** — `Unbuilt`, `NoSurface`, `Blocked`, `Success` are distinguishable, because
> "no path" and "the region isn't built yet" demand different consumer behaviour (Recast's
> conflation of these is visible today in CkCrowd's 5s deferral watchdog, R3 §B).

### F1.13 — Point projection

**What it is.** Given a world position, an asymmetric search box (separate XY and ±Z extents), and
an agent profile + radius, return the nearest walkable surface position, its plate id, its surface
normal, and its area tag. Modes: **closest**, **down-only**, **up-only**. Tie-break is
horizontal-first (a point 10 uu sideways beats one 50 uu below), matching the behaviour crowd
containment depends on. Batched variant takes a span of positions and fills a span of results.
Failure modes: `NoSurface` when nothing in the box qualifies; `Unbuilt` when the box intersects only
unbuilt tiles — these must not be conflated.

**Why we need it.** The most widespread capability in the codebase (R3 §C.1): `CkNav_Algorithm.cpp:149-177`,
`CkNav_Processor.cpp:226,373`, `CkNav_Utils.cpp:227-241` (`Try_ProjectOntoNavmesh`, the public
UFUNCTION), `CkCrowdAgent_ConstrainToNavmesh_Processor.cpp:108-143` (including its 4× radius
recovery), `CkEqs/Query/CkEqs_Algorithm.cpp:266-280`, `CkQueue/Queue/CkQueue_Formation_Processor.cpp:159-209`,
`CkPathNetwork_EditorUtils.cpp:27-35,176-186,356-366`, `CkPerfLab_WorldSurvey_Builder.cpp:57-250`.

**What WE build.** A tile/plate lookup over the field: quantize the query to tile + cell, walk the
candidate layers' spans within the Z extent, then expand outward in rings within the XY extent,
rejecting cells whose clearance is below the agent radius. Half-extents come from a
**provider-neutral setting** ([NN-D8e]). The batch variant amortizes tile lookup across sorted
queries. Illustrative facade shape:

```cpp
UFUNCTION(BlueprintCallable,
          Category = "Ck|Utils|NavSurface",
          DisplayName="[Ck][NavSurface] Try Project Point")
static FCk_NavSurface_ProjectionResult
Try_ProjectPoint(
    const UObject* InWorldContext,
    const FCk_NavSurface_ProjectionQuery& InQuery);
```

**Acceptance criteria.** Layer 1: a point 200 uu above a floor projects to the floor with the
correct normal; a point inside a wall with floors on both sides projects to the horizontally nearer
one; a point over a hole with the search box smaller than the hole returns `NoSurface`, and larger
returns the hole's rim; a query over an unbuilt tile returns `Unbuilt` and never `NoSurface`; the
batch variant's results are element-wise identical to N single calls. Layer 2: the existing crowd
containment autotests pass on the CkGroundNav provider.

**Provenance.** Nearest-surface search over a spatially indexed height field — standard; the
asymmetric-extent + tie-break contract is a restatement of the behaviour our consumers already
depend on.

---

### F1.14 — Is-navigable test (single and batch)

**What it is.** A cheaper predicate than projection: "is this exact position on walkable ground for
this profile and radius, within a small tolerance" → status. Batch variant over a span.

**Why we need it.** EQS-style filtering (`CkEqs_Algorithm.cpp:266-280` currently uses projection for
what is really a test), queue slot validation (`CkQueue_Formation_Processor.cpp:159-209`), and any
consumer that only needs a yes/no and is currently paying for a full projection.

**What WE build.** The tile→layer→cell lookup from F1.13 without the ring expansion, plus the
clearance predicate. Batch form sorts by tile to amortize lookups.

**Acceptance criteria.** Layer 1: agreement with projection-at-zero-extent on 10k random points over
a fixture; measured cost strictly below projection on the same fixture (recorded in VALIDATION.md).

**Provenance.** Direct index lookup; no algorithmic claim.

---

### F1.15 — Constrained surface walk (containment)

**What it is.** Given a start position known to be on the surface and a desired target position,
return the furthest position reachable by sliding along walkable ground without leaving it, plus
whether the full move succeeded and the plate the walk ended on. It must **never** return a position
off the walkable set, must handle the walk crossing plate and tile boundaries, and must terminate
(no infinite slide) on a concave corner.

**Why we need it.** `CkCrowdAgent_ConstrainToNavmesh_Processor.cpp:241` is **the single Transform
writer for grounded agents** (R3 §C.3) — every grounded agent's final position each frame passes
through that one call, and it is the **only** consumer of this capability. This is the
highest-stakes Tier-1 query: a defect here is agents walking through walls or falling out of the
world.

**What WE build.** A DDA walk over the cell grid from start toward target, clipping at the first
cell that fails the walkable + clearance predicate, then projecting the residual motion onto the
blocking edge (slide) and continuing with a bounded iteration count. Plate/portal structure gives an
early-out: a move that stays inside one plate's rectangle and above its clearance is admitted
without stepping cells at all — the common case for a crowd agent's per-frame delta.

**Acceptance criteria.** Layer 1: 10k randomized moves over a fixture with walls, holes, and a
multi-storey overlap, asserting **zero** end positions off the walkable set (this is a hard
zero — a containment escape is a promotion blocker per `MIGRATION_SEAM.md` §4 (promotion item 1)); a move into a
concave corner terminates within the iteration bound; a move fully inside one plate takes the
early-out path (assert on an instrumented counter). Layer 2: the crowd containment and
corner-retirement autotests green. Layer 3: gym station with a wall-hugging agent, visually
verified `[EDITOR-VERIFY]`.

**Provenance.** DDA / Amanatides-Woo grid traversal (Amanatides & Woo 1987) with sliding-collision
response; the constrained-surface-walk contract mirrors the well-known "move along surface" query
published in the open-source `recastnavigation` project's detour layer (zlib).

---

### F1.16 — Walkability raycast (line-of-sight along the surface)

**What it is.** Straight-line "can I walk from A to B without leaving walkable ground" → clear /
blocked + hit position + hit normal. Optional early termination on accumulated cost exceeding a cap.
Callable off the game thread.

**Why we need it.** Five sites retire waypoints and validate corridors on this today (R3 §C.4):
`CkCrowdAgent_Steering_Processor.cpp:93-120,185`, `CkCrowdAgent_OnPathResolved_Processor.cpp:294`,
`CkCrowdAgent_OnRouteResolved_Processor.cpp:276`, plus `CkQueue_Formation_Processor.cpp:159-209`
and PathNetwork's `Is_NavmeshSegmentDirectlyWalkable` (`CkPathNetwork_Processor.cpp:358-375`).

**What WE build.** The same DDA walk as F1.15 without the slide — first failing cell is the hit. The
plate rectangle early-out applies again: a segment contained in one plate is clear by construction.
The cost cap accumulates per-cell area cost multipliers as it walks, giving PathNetwork's "is this
ribbon segment cheap enough as well as walkable" question a direct answer.

**Acceptance criteria.** Layer 1: a segment across an open plane is clear; a segment through a wall
reports blocked with the hit within one cell of the analytic wall plane; a segment along a corridor
narrower than the agent radius is blocked; results are symmetric in A/B for the clear case and hit
positions agree within a cell for the blocked case. Layer 2: steering corner-retirement autotests
green.

**Provenance.** DDA grid ray walk (Amanatides & Woo 1987); the surface-raycast query contract as
published in the open-source `recastnavigation` project's detour layer (zlib).

---

### F1.17 — Boundary segment query (off-thread)

**What it is.** Given a centre position and a radius, return the walkable/non-walkable **boundary
segments** within that radius as flat endpoint pairs with consistent winding (interior on a known
side), capped at a caller-supplied maximum. **This query must be callable from a parallel processor
off the game thread**, against an immutable field snapshot, with no locking and no allocation from a
shared pool.

**Why we need it.** `CkCrowdAgent_AvoidanceSample_Processor.cpp:47,171-189` calls Recast's
`FindEdges` from a `TParallelProcessor`, and the thread-safety contract is documented at that
header's `:28` (R3 §C.5). This is the single hardest requirement to retrofit onto a mutable
structure — and the reason `REPRESENTATION.md` counts it as falling out of the immutable-publish
design rather than needing a lock discipline.

**What WE build.** Boundary segments are **precomputed at bake time** as part of plate extraction:
every plate edge that is not covered by a portal is a boundary edge, stored per tile in a flat
segment array with a coarse spatial index. The query is then a bounded index scan against a
`TSharedPtr<const>` field with results written into caller-provided storage — no locks, no
allocation, trivially parallel. Consistent winding is established once at bake, not per query.

**Acceptance criteria.** Layer 1: segments around a point in an open room match the room's walls
within a cell; winding is consistent (interior test on every returned segment); the cap truncates
deterministically (nearest-first). Layer 1 (concurrency stress): 8 worker tasks querying against
captured field snapshots while the game thread publishes and repairs the field 1000 times — every
result self-consistent with the epoch its snapshot carries, zero torn reads, zero crashes, zero
ensures; the Layer-1 thread-contract test green alongside it. Layer 2: crowd avoidance autotests green with
the parallel processor unchanged.

**Provenance.** Local-boundary extraction for reciprocal collision avoidance — the `dtLocalBoundary`
concept published in the open-source `recastnavigation`/Detour crowd layer (zlib); consumed by our
existing ORCA-family avoidance (van den Berg et al., *Reciprocal n-body Collision Avoidance*).

---

### F1.18 — Closest boundary edge / closest point on boundary

**What it is.** Given a position, return the single nearest boundary segment, the closest point on
it, and the distance. Distinct from F1.17 (which returns a set) because the common consumer wants
one answer and the set query's cap semantics are the wrong shape for it.

**Why we need it.** Wall-distance costing (F1.26), avoidance-volume edge confirmation, and the
"push me back onto the surface" recovery in containment (`ConstrainToNavmesh` 4× radius recovery,
R3 `108-143`) all want the nearest wall, not all walls.

**What WE build.** Same precomputed boundary segment array + spatial index as F1.17, with an
expanding-ring nearest search. Falls out of F1.17's data; no new bake stage.

**Acceptance criteria.** Layer 1: agreement with a brute-force nearest-segment reference on 10k
random points over a fixture; the closest point always lies on the returned segment.

**Provenance.** Nearest-segment search over a spatially indexed segment set; standard computational
geometry.

---

### F1.19 — Reachability query (near-O(1) reject + exact flood fill)

**What it is.** Two related queries. (a) `Get_IsReachable(from, to, profile)` — a component-label
comparison (F1.9), near-O(1), answering *definitely unreachable* vs *possibly reachable*. (b) A
**flood fill** over the portal graph from a source, producing per-plate true shortest path distances
(not centre-to-centre approximations), with a pluggable early-exit predicate; one-to-many variants
answer "which of these 50 destinations are reachable and how far".

**Why we need it.** Facade capability ([NN-D8] Step 2); unreachable-goal rejection in CkCrowd; and
flood fill is the shared engine behind point generation (F1.21) and reachability-filtered EQS tests.

**What WE build.** (a) is an array compare. (b) is a Dijkstra-style priority-queue expansion over
the plate-portal graph where each expanded plate carries its funnel state, so the recovered distance
is the true string-pulled distance rather than a portal-centre sum. The funnel machinery is shared
verbatim with F1.24 — one implementation, two consumers.

**Acceptance criteria.** Layer 1: on a fixture with two disconnected islands, cross-island
reachability is false in O(1) (assert on an expansion counter of zero); flood-fill distances agree
with independently computed A* path lengths within a tight tolerance on 100 sampled destinations;
the early-exit predicate provably bounds expansion.

**Provenance.** Dijkstra (1959) single-source shortest paths; funnel-based exact path-distance
recovery over a portal sequence (funnel algorithm — "simple stupid funnel", Mononen/Demyen public
write-ups; Demyen & Buro, *Efficient Triangulation-Based Pathfinding*).

---

### F1.20 — Surface attributes at a point (single and batch)

**What it is.** Given a position (or a span of positions), return the surface normal, the area tag
set, the cost multiplier, and the owning plate/layer id. Batched.

**Why we need it.** Markup ground-truth (F1.32) is a special case of it; debug draw and the crowd's
"what am I standing on" checks want it directly; EQS scoring wants it batched.

**What WE build.** A projection-free direct lookup (the caller already has an on-surface position),
reading plate-level attributes — which is exactly why the plate decomposition stores policy at plate
level rather than per cell.

**Acceptance criteria.** Layer 1: attributes at a point inside a painted region match the paint;
attributes one cell outside do not; batch equals N singles.

**Provenance.** Direct index lookup; no algorithmic claim.

---

### F1.21 — Point generation (random and grid, radius- and distance-bounded)

**What it is.** Three generators. (a) **Random points in radius** — uniformly distributed over
walkable area within a radius of an origin, optionally seeded for determinism. (b) **Random points
by path-distance range** — points whose true path distance from an origin falls in [min, max],
which is the query EQS actually wants and that a radius cannot express (a point 200 uu away through
a wall is 3000 uu of walking). (c) **Grid of points in radius/bounds** — regular spacing with an
optional grid-phase alignment flag so successive queries agree on lattice positions, each point
carrying its area tags.

**Why we need it.** `CkEqs` generation and scoring (R3, `CkEqs_Algorithm.cpp:266-280` is the
projection half of this today), and `CkPerfLab_WorldSurvey_Builder.cpp:57-250` which seeds spawn
positions from the navmesh.

**What WE build.** (a) and (c) sample plate rectangles weighted by walkable area, rejecting cells
under the clearance threshold — the plate decomposition makes area-weighted sampling exact and
cheap. (b) runs the F1.19 flood fill with the distance range as its early-exit predicate, then
samples within admitted plates. RNG is caller-seeded and results are reproducible for a given seed +
field epoch.

**Acceptance criteria.** Layer 1: 100k random points in a fixture are all on walkable ground and
their spatial distribution is uniform within a chi-square tolerance across plates; the same seed +
epoch reproduces the same set; grid-phase alignment produces identical lattice positions for two
overlapping query bounds; distance-range points all verify against an independent path query.

**Provenance.** Area-weighted rejection sampling; Dijkstra flood fill (as F1.19); regular lattice
generation.

---

### F1.22 — Build-status query

**What it is.** `Get_IsBuildInProgress(world)`, `Get_IsBuilt(point)`, `Get_IsBuilt(bounds)`, and
`Get_SurfaceBounds(world)`. Lets a consumer avoid querying an unbuilt region, and lets the debugger
and tests gate on a settled field.

**Why we need it.** `CkCrowdAgent_PathRefresh_Processor.cpp:140-152` gates on
`IsNavigationBuildInProgress` today; the CkCrowdDebugger status panel and viewport fit need bounds
and health (`CkCrowdDebugger_DataCollector.cpp:169-482`, `Types.h:133`); the facade lists both
([NN-D8] Step 2).

**What WE build.** Per-world state derived from the tile status map in the published field plus the
build scheduler's pending set. `Get_ProviderHealth(world)` returns a status enum, never a bool, and
**failure is a status, never an empty answer** — the same rule the volumetric debug snapshot already
enforces (`R4 §1`: `MissingCook/StaleCook/Building/Current/Failed/RuntimeOnly`).

**Acceptance criteria.** Layer 1: status transitions Unbuilt → Building → Built exactly once per
tile per build; querying a point in an unbuilt tile reports `Unbuilt` consistently across F1.13–F1.21.
Layer 2: a PIE autotest waits on the named condition "surface built" rather than a hop count
(`R4 §7` settling discipline).

**Provenance.** No algorithmic claim.

---

## Part C — Path search and installation

### F1.23 — A* over the plate-portal graph

**What it is.** Given start and goal positions, an agent profile + radius, and a compiled filter,
produce an ordered corridor of plates and the portals crossed. Bounded by max expanded nodes and max
path length. Time-sliceable: a search that exceeds its slice budget preserves state and continues
next tick. Failure modes, all distinguishable: `Unbuilt`, `NoStartSurface`, `NoGoalSurface`,
`Unreachable` (component reject), `BudgetExceeded`, `Blocked`.

**Why we need it.** It is the provider. `CkCrowdAgent_HandleRequests_Processor.cpp:488-557`
(`RequestPathForActiveGoal`) gains a CkGroundNav branch beside VoxelNav → PathNetwork →
CkNavigation ([NN-D8] Step 3).

**What WE build.** A `TGraph` model satisfying the existing `AStarGraph` concept
(`CkAStar_GraphConcept.h:16-31` — exactly `Neighbors`, `Cost`, `Heuristic`, `IsGoal`, nothing
geometric) over plate ids, driven by `TSearchState::ContinueSearch(FSearchParams)`
(`CkAStar_Search.h:54`, budget = iterations AND microseconds) — the same infrastructure CkGoap,
CkVoxelNav (over a merged-BOX graph) and CkPathNetwork (over an edge graph) already run on
(`R4 §2`). Per-query immutable seed data follows `FPathGraphSharedData`'s precedent
(`CkVoxelNav_Path_Graph.h:33-47`), including its rule that **agent fit is expressed as a clearance
minimum, not as a layer/plate index — "search must stay blind to how a cell is addressed"**. The one
genuinely new piece R4 flags: a portal graph needs its own transition-point math (the analogue of
`Get_CellTransitionPoints`) — the point on a portal interval used for `Cost` and `Heuristic`.

**Heuristic contract.** Baseline heuristic is **Euclidean distance** — admissible, inadmissibility
bound **0**. An optional greedy weight `w >= 1.0` is a tunable defaulting to **1.0** (admissible at
the default); the bound at a given `w` is `(w - 1)`. Any `w > 1.0` must be recorded together with its
measured path-quality impact.

**Acceptance criteria.** Layer 1: optimal path length on a fixture with a known analytic optimum,
within the heuristic's stated inadmissibility bound; a search across two disconnected components
rejects via F1.9 with zero expansions; a sliced search matches a one-shot search exactly; node cap
and length cap each terminate with the correct distinguishable status. Layer 1: expansion counts
recorded in VALIDATION.md as tracked regression numbers.

**Provenance.** A* (Hart, Nilsson & Raphael 1968, *A Formal Basis for the Heuristic Determination of
Minimum Cost Paths*); weighted/greedy A* with a bounded-inadmissible heuristic (Pohl 1970; Pearl,
*Heuristics*). Search infrastructure is our own `CkAStar` (`R4 §2`). **Design origin:** independently
designed here against the existing `astar::AStarGraph` concept, instantiated over **crossing-arrival
nodes** — a node is an arrival at a portal crossing, not a plate — with a Euclidean heuristic and an
optional greedy weight `w >= 1.0`. Slicing rides `TSearchState::ContinueSearch` with each slice
clamped to the *remaining* expansion budget, which is what makes "sliced == one-shot" an identity
rather than an approximation. The distinguishable statuses (`Unbuilt`, `NoStartSurface`,
`NoGoalSurface`, `Unreachable`, `BudgetExceeded`, `Blocked`) are our own contract. No external code
was copied, translated, or structurally mirrored; the works cited are published literature.
**Landed** on `feature/ck-navigation` in CkFoundation `342db65da` —
`Source/CkGroundNav/Public/CkGroundNav/Search/CkGroundNav_SearchTypes.h`,
`CkGroundNav_PlatePortalGraph.{h,cpp}`, `CkGroundNav_PathSearch.{h,cpp}`, the `ck.GroundNav.PathAt`
debug command in `Debug/CkGroundNav_DebugDraw.cpp`, and the `CkAStar` module dependency; tests in
CkTests `2ec88245`. **Evidence:** `Saved/Logs/P3-3A-Gate2c.log` — 164/164.

---

### F1.24 — Funnel string-pulling over portals

**What it is.** Convert a plate corridor + portal sequence into a minimal ordered waypoint list — the
shortest path within the corridor. Must handle: portals of zero-length degenerate intervals, corridor
segments where the apex must switch sides, and **agent radius inset** (waypoints pulled off the
portal endpoints by the agent radius so the string-pulled path does not graze corners).

**Why we need it.** Waypoint lists are the neutral output contract (`FCk_Nav_PathResult::_Waypoints`,
R3 §B). Without string-pulling, paths follow plate centres and agents visibly stair-step —
`REPRESENTATION.md` names funnel-over-portals as the mitigation for cell-resolution boundaries.

**What WE build.** The funnel algorithm defined over a portal interval sequence — it is defined for
any convex-region/portal chain, not only triangles, which is precisely why candidate B is viable
([NN-D7]). Shared implementation with F1.19's flood-fill funnel. Radius inset is applied to portal
intervals before funnelling (clamping to the portal's midpoint when the interval is narrower than
2R, which cannot happen for an admitted path since the portal's min clearance already gated it).

**Acceptance criteria.** Layer 1: on an L-shaped corridor the funnel emits exactly 3 waypoints
(start, inner corner, goal); on a straight corridor exactly 2; string-pulled length ≤ corridor
centre-line length on every fixture; every waypoint is ≥ R from the nearest boundary (F1.18) for the
requested radius; degenerate portals do not produce duplicate or NaN waypoints.

**Provenance.** Funnel algorithm / string pulling — "simple stupid funnel algorithm" (Mononen's
public write-up) and Demyen & Buro, *Efficient Triangulation-Based Pathfinding*; the same technique
in the open-source `recastnavigation` project's detour layer (zlib). **Design origin:** independently
designed and implemented here, and the P3 corridor consumer adds **no second funnel** — it calls the
one shared `Get_StringPull` (the radius inset is applied inside it), so the flood fill and the path
search string-pull through identical code. No external code was copied, translated, or structurally
mirrored; the works cited are published references. **Landed** on `feature/ck-navigation` in
CkFoundation `f019236b1` (corridor → waypoints via `Search/CkGroundNav_PathPostProcess.{h,cpp}` and
the `Path/` GroundNavPath feature quartet); tests in CkTests `a48412bb` measure the acceptance
criteria directly — an L corridor emits 3 waypoints, a straight corridor 2, and every waypoint is
≥ R from the boundary (`Path.LCorridor_ThreeWaypointsAllClearOfBoundary` and siblings).
**Evidence:** `Saved/Logs/P3-3B-Gate3c.log` — 175/175.

---

### F1.25 — Partial paths and search bounds

**What it is.** When `_AllowPartialPath` is set and the goal is unreachable or the search exhausts
its node cap, return the path to the best node reached, flagged `Partial`. The result must
distinguish `Partial` from `Ready` and from `Failed`. Bounds: max path length and max expanded
nodes, with the documented interaction that under partials the length limit prunes differently and
the node cap is the real bound.

**Why we need it.** `_AllowPartialPath = true` is the request default (R3 §B) and **CkCrowd's strict
phase treats a short partial as a verdict that triggers permissive re-dispatch** (R3 §C.8) — the
partial/complete distinction is load-bearing gameplay logic, not a nicety.

**What WE build.** Best-node tracking inside the CkAStar search state (lowest h-value expanded),
returned on cap/exhaustion when the flag is set; funnel then runs over the truncated corridor
normally. The status mapping into the existing `_Status` ∈ {None, Pending, Ready, Failed, Partial}
enum is byte-compatible with today's contract.

**Acceptance criteria.** Layer 1: a goal inside a sealed room with partials enabled returns
`Partial` with a path ending adjacent to the seal; with partials disabled returns `Failed` with no
waypoints; the node cap produces `Partial` (flag on) or `BudgetExceeded` (flag off), never a silent
truncation reported as `Ready`. Layer 2: the crowd strict→permissive re-dispatch autotests green.

**Provenance.** Best-first search with best-node fallback — standard A*/anytime-search practice
(Likhachev et al., ARA* family, for the published treatment of bounded-suboptimal returns).
**Design origin:** independently designed here. The best node is the **lowest-heuristic node actually
expanded**, memoised inside `IsGoal` — the one place the search already visits every expanded node,
so the tracking is free. `Partial` is returned on exhaustion or the expansion cap **only when the
flag is set**; without it those same conditions report `Unreachable` / `BudgetExceeded`. The
corridor-length cap always reports `BudgetExceeded` and never a partial, and a best node equal to the
source is never reported as `Partial` (a zero-progress partial is a failure, not a short path).
Through the seam a partial installs as `ECk_Nav_PathStatus::Partial`, so CkCrowd's strict→permissive
re-dispatch reads it unchanged. No external code was copied, translated, or structurally mirrored.
**Landed** on `feature/ck-navigation` in CkFoundation `f019236b1` (partial paths and the cost
parameters on the query), with the install mapping in `df65f4988`; tests in CkTests `a48412bb`.
**Evidence:** `Saved/Logs/P3-3B-Gate3c.log` — 175/175; `P3-3C-Gate4d.log` — 191/192.

---

### F1.26 — Cost model

**What it is.** The scalar cost of traversing from plate A to plate B: base Euclidean distance
through the portal transition point, multiplied by (a) the destination plate's **area cost
multiplier** (max-wins when multiple painted regions overlap), (b) a **slope/gradient penalty**
scaling with height change over horizontal distance, and (c) a **clearance bias** that mildly
penalises hugging walls so agents route down corridor centres. Per-query filter overrides may
rewrite a plate's multiplier. Accumulation mode is documented and fixed (integrate over the segment,
not max-of-cells).

**Why we need it.** Cost areas are how the game steers crowds: `UCk_NavArea_CrowdAgent` marks
stationary agents expensive; the three avoidance-volume area classes make volumes expensive rather
than impassable (R3, `CkCrowdAvoidanceVolume_NavArea.h/.cpp`); PathNetwork threads a cost policy
through its route compiler — `Try_ProjectPathOntoNavmesh` (~457), `Try_ResolvePathOntoNavmesh`
(~547), `Try_ResolvePathOntoNavmeshWithRibbonConstraints` (~602) and `Apply_NavmeshClearance` (~791)
in `CkPathNetwork_Processor.cpp` each thread the query-filter class through.

**What WE build.** Cost lives entirely in the CkAStar graph model's `Cost(A,B)` — the field itself
stores an area-tag key and a multiplier per plate, and the per-query compiled filter (F1.27) supplies
the tag→multiplier table. Clearance bias is free because clearance is already per cell/per portal
(F1.5) — this is one of the concrete wins `REPRESENTATION.md` cites for the chosen representation.
The frozen accumulation semantics live in the PHASE doc; executors do not choose them.

**Frozen formula shapes** (PHASE_3 §5; constants are measured at phase entry, not invented):
- slope penalty — `cost * (1 + k_slope * rise / run)`; `k_slope` tunable, default
  `[MEASURE at phase entry]` on the ramp-vs-level fixture.
- clearance bias — `cost * (1 + k_wall / (clearance_cells + 1))`; `k_wall` tunable, default
  `[MEASURE at phase entry]` on the corridor fixture.
- corner offset distance — `agent_radius * k_corner`; `k_corner` default **1.0**,
  `[MEASURE at phase entry]` on the corner fixture.
- crossover tolerance — **one finest cell**.

Changing a formula shape is an orchestrator decision; changing a constant is a measurement.

**Acceptance criteria.** Layer 1: a path around a 3× cost region is chosen when the detour is <3×
longer and through it when longer, at the exact analytic crossover ± one finest cell; two overlapping
painted regions yield the max multiplier, not the product or sum; the slope penalty makes a ramp
route lose to a level route at the configured threshold; clearance bias shifts a corridor path
toward the centre line measurably without changing its plate corridor.

**Provenance.** Weighted-graph edge costing; terrain/area cost multipliers as in the open-source
`recastnavigation` project's area-cost model (zlib); clearance-biased costing from the
roadmap/clearance literature (medial-axis and clearance-based path planning). **Design origin:**
independently designed here as a single leg-cost function, `Get_LegCost` = Euclidean distance
× plate multiplier × `(1 + k_slope * rise / run)` × `(1 + k_wall / (clearance_cells + 1))`. The
multiplier comes from the per-query tag→multiplier table, and overlapping paints resolve **max-wins**
through `Get_MaxMerged`, matching the frozen semantics above. No external code was copied,
translated, or structurally mirrored. **Constants are measured, not invented** (they replace the
`[MEASURE at phase entry]` placeholders above): `k_slope` and `k_wall` both ship at **0**, because
each was measured against Recast and any non-zero value breaks A/B parity; the analytic detour
crossover measured `m* = 14.56` at `ρ = 3.02` on the cost-region fixture. Corner offset ships at the frozen bullet's
value — `FCk_GroundNav_PathCostParams::_CornerOffsetK = 1.0f`
(`Search/CkGroundNav_SearchTypes.h`), one agent radius, so the bullet above stands as written.
**Parity observations (not defaults):** Recast's own corner offset is **0** as we drive it
(`CkNav_Processor.cpp:446` passes `InCornerOffsetDistance = 0.0f`) and its raw inside-corner distance
measures **1.55 R** — inputs for a later reference-number decision, not a change to the shipped
default. **Landed** on `feature/ck-navigation` in CkFoundation `f019236b1`; tests in CkTests
`a48412bb`. **Evidence:** `Saved/Logs/P3-3B-Gate3c.log` — 175/175.

---

### F1.27 — Provider-neutral filters and area tags

**What it is.** A query filter is a **neutral value**: a set of required area tags, a set of excluded
area tags, and a tag→cost-multiplier table. Filters are authored as data assets, selected per query
by `FGameplayTag` through the project settings table, and may be further narrowed per query by a
value-only **exclusion overlay**. A malformed filter **fails closed** (the query fails with a
distinguishable reason) — never silently degrades to "no filter".

**Why we need it.** This is Step 1 of the migration seam ([NN-D8]) and removes the only two
Unreal-typed leaks in the public contract:
`FCk_Nav_QueryFilterOverlay::_ExcludedAreaClasses : TArray<TSubclassOf<UNavArea>>`
(`CkNav_Fragment_Data.h:183`) → `TArray<FGameplayTag>`, and
`FCk_Request_Nav_FindPath::_QueryFilterClassOverride : TSubclassOf<UNavigationQueryFilter>`
(`:215`) → `FGameplayTag _QueryFilterOverride`. `UCk_Nav_ProjectSettings_UE::_QueryFilters`
(`CkNav_ProjectSettings.h:43`) becomes tag → neutral filter definition. CkCrowd's strict/permissive
filter classes (`CkCrowdAgent_HandleRequests_Processor.cpp:117-134`,
`CkCrowdAgent_NavQueryFilter.h:17`) become two filter-definition assets.

**What WE build.** A `UCk_NavFilterDefinition_DataAsset` (working name) holding the neutral value;
each provider adapter compiles it into its native form — the Recast adapter into a
`UNavigationQueryFilter` instance seeded from the existing framework area classes, CkGroundNav into
a plate-policy mask + cost table resolved once per query into an immutable seed. **Per CkFoundation
doctrine there are no back-compat shims** — call sites migrate in the same change.

**Acceptance criteria.** Layer 1: a filter excluding tag X makes every plate carrying X impassable
and no others; a malformed/unresolvable filter fails the query closed with the documented reason and
mutates nothing; the compiled-filter cache produces identical results to an uncached compile.
Layer 2: crowd strict and permissive phases behave identically to the recorded Recast baseline in
shadow mode. Layer 1 (contract): no Unreal-Navigation type appears in any public CkNavigation
header (a grep-based assertion in the gate).

**Provenance.** Attribute/flag-based query filtering with per-area cost multipliers — the filter
model published in the open-source `recastnavigation` project (zlib), re-expressed over
`FGameplayTag`.

---

### F1.28 — Path result shape and post-processing

**What it is.** The neutral result: an ordered waypoint list, a destination, a status, and
diagnostics. Post-processing stages, in order: (1) funnel (F1.24); (2) **corner offset** — waypoints
pushed off inside corners by a configured distance so an agent with volume does not clip them;
(3) the existing Ck skip-first-waypoint pass; (4) fill per-waypoint data (direction to next,
integrated distance, integrated cost, surface normal, area tags at the waypoint).

**Why we need it.** `FNavMeshPath::OffsetFromCorners` (`CkNav_Algorithm.cpp:234-239`) plus the Ck
skip pass is what today's consumers receive; `ExtractWaypoints` (`:250-313`) is the Recast→neutral
conversion that disappears when CkGroundNav produces the neutral form natively. The result struct
`FCk_Nav_PathResult` stays byte-compatible ([NN-D8] ground rules), including the rule that
**waypoints are preserved on failure** so consumers keep walking the old path, and that
`_PendingSinceSeconds` is process-relative and **must not be persisted or replicated**.

**What WE build.** The post-processing chain as pure functions over a waypoint array + the field, so
each stage is Layer-1 testable in isolation. Corner-offset distance is a provider-neutral setting.

**Acceptance criteria.** Layer 1: corner offset moves exactly the inside-corner waypoints and by the
configured distance; every offset waypoint remains on walkable ground with clearance ≥ R (offsetting
must never push a waypoint off the surface — a real failure mode); integrated distance equals the
polyline length; a failed query leaves the previous `_Waypoints` intact.

**Provenance.** Corner offsetting / path smoothing over a string-pulled polyline — standard
post-processing, as in the open-source `recastnavigation` project (zlib). **Design origin:**
independently designed here as a pure chain over the waypoint array: (1) funnel (F1.24); (2) **corner
offset** — the waypoint is pushed *away from the vertex*, and the offset is accepted only if the
offset point is navigable and still ≥ R from the boundary, halving the offset up to four times before
giving up, which is what makes "offsetting never pushes a waypoint off the surface" structural rather
than hoped-for; (3) the Ck skip-first-waypoint pass, mirrored from our own
`CkNav_Algorithm.cpp:193-207` so the neutral result keeps today's semantics; (4) per-waypoint fill —
direction, XY-integrated distance, integrated cost, surface normal, and area tags — into
`FCk_GroundNav_PathPlan`. No external code was copied, translated, or structurally mirrored; the
mirrored pass is our own CkNavigation code, cited for behavioural parity. **Landed** on
`feature/ck-navigation` in CkFoundation `f019236b1` — `Search/CkGroundNav_PathPostProcess.{h,cpp}`
plus the `Path/` GroundNavPath feature quartet; tests in CkTests `a48412bb`. **Evidence:**
`Saved/Logs/P3-3B-Gate3c.log` — 175/175.

---

### F1.29 — Async dispatch, per-frame budget, deferral, and revisions

**What it is.** Path requests are queued, drained under a per-frame budget, and carry a caller-owned
opaque revision with latest-wins semantics over a ring on [1, MAX_int32] (shorter forward distance =
newer). A request whose start is not yet projectable is **parked** and re-probed per tick, then
force-failed after a bounded deferral. Cancellation is explicit and immediate. The whole mechanism
must be **per-world**.

**Why we need it.** The existing contract, exactly (R3 §B): `Get_MaxPathQueriesPerFrame()` default 8;
revision enforced at `AddDeferredLatest`, batch pre-drain, and in-drain; `Request_AbandonPath`
purges in-flight and deferred entries **before** completing any, because completion delegates are
caller AngelScript that may re-enter abandon. [NN-D8b] rules that the deferred queue
(`GDeferredNavRequests`, `CkNav_Processor.cpp:33-122` — explicitly *not* world-keyed today, a
documented multi-PIE defect) becomes **per-world state** when the drain moves behind the facade,
preserving the 5s watchdog and latest-wins semantics.

**What WE build.** The existing processor's mechanism, moved behind the facade and keyed per world.
CkGroundNav's own search is time-sliced (F1.23), so a long search consumes budget across frames
rather than blocking; deferral remains because both providers build asynchronously. Every `Pending`
write restamps the pending clock (`:379-385` semantics preserved).

**Acceptance criteria.** Layer 1: revision ring comparisons correct across the wrap boundary
(exhaustive over representative pairs); an older-revision arrival is cancelled and a newer one evicts
the older with `Failed_Cancelled`; abandon-during-completion-callback re-entrancy does not double-fire
or leak an entry. Layer 2 (multi-PIE): two PIE worlds each running crowds show zero cross-world
request leakage — the defect the current global queue has. Layer 1: budget of N drains at most N per
tick and the remainder survive.

**Provenance.** No algorithmic claim — this is our existing contract, preserved. Ring comparison is
standard sequence-number arithmetic (as in TCP sequence-space comparison, RFC 1982 serial-number
arithmetic). **Design origin:** independently designed here as the GroundNavPath feature — requests
live on the agent, are drained by `FProcessor_GroundNavPath_HandleRequests` and advanced by
`FProcessor_GroundNavPath_Slice`, under the per-frame budget CVars `ck.GroundNav.MaxSearchesPerFrame`
(8), `ck.GroundNav.SliceBudgetMs` (1.0), `ck.GroundNav.MaxIterationsPerSlice` (0) and
`ck.GroundNav.MaxDeferralSeconds` (5.0). An `Unbuilt` start defers and then fails once the deferral
bound is spent; the caller's revision is carried on both the request and the result; abandon is
enqueued rather than executed inline. No external code was copied, translated, or structurally
mirrored. **Finding — corrects the "Why we need it" cell above, which reads "The existing contract,
exactly (R3 §B): `Get_MaxPathQueriesPerFrame()` default 8":** in CkNavigation that accessor **bounds
nothing**. Its value is assigned at `CkNav_Processor.cpp:175` and never read, so the documented
per-frame cap was never actually enforced on the Recast path. `ck.GroundNav.MaxSearchesPerFrame` is
therefore a *new* bound, not a preserved one; the prior text is left standing above and this is the
correction beside it. **Landed** on `feature/ck-navigation` in CkFoundation `f019236b1` (the `Path/`
GroundNavPath feature quartet); tests in CkTests `a48412bb`. **Evidence:**
`Saved/Logs/P3-3B-Gate3c.log` — 175/175.

---

### F1.30 — Plan repair / warm-started replanning

**What it is.** Given an existing path and a field change, validate it and — where it is repairable —
re-search only from the first invalidated step rather than from scratch. Output: a repaired path or
a full-replan verdict.

**Why we need it.** `CkCrowdAgent_PathRefresh_Processor.cpp` does sync replans today
(`461-486`, `799-825`: escape path and corridor splice) at full cost. Repair is how a crowd survives
a moving obstacle without a replan storm.

**What WE build.** The seam already exists: `CkAStar_Search.h`'s **warm-start constructor**
`(Graph, Start, Goal, ExistingPath, WarmStartFromIndex, Capacity)` (`:37-44`) and the free
`ValidateExistingPath(Graph, Path) -> int32` returning the first disconnected step (`:15-20`) —
R4 spot-checked both. CkGroundNav supplies the graph; validation runs against the *current* published
field epoch, so the check is a pure function of two values.

**Acceptance criteria.** Layer 1: a path invalidated at step k re-searches with expansions bounded
well below a cold search (tracked number); a path invalidated at step 0 correctly reports full
replan; a still-valid path validates in O(path length) with zero expansions.

**Provenance.** Incremental replanning / path repair (D*-family and anytime-repairing literature —
Stentz 1994; Koenig & Likhachev, D* Lite) — here in its simplest published form, warm-started A*
from the first invalid step. Infrastructure is our own `CkAStar`. **Design origin:** independently
designed here on the CkAStar repair seam. The corridor is keyed by `Make_CrossingKey`, and
`TryGet_NodeForKey` re-resolves a key against the current field by **door geometry, not plate
index** — a rebake renumbers plates, so geometric matching is what lets a key survive an epoch
change. `astar::ValidateExistingPath` finds the first disconnected step and the warm-start
constructor re-searches from it. Measured: an unchanged epoch validates `StillValid` with **zero**
expansions; a warm repair took **5** expansions against **11** for the equivalent cold search. Change
*detection* is deliberately not in this row — that is F1.34, PHASE_4 work; this row is the repair
mechanism only. No external code was copied, translated, or structurally mirrored; the works cited
are published literature. **Landed** on `feature/ck-navigation` in CkFoundation `f019236b1` (the
`TryGet_NodeForKey` / `Request_BeginRepair` seams); tests in CkTests `e9edfcd1`. **Evidence:**
`Saved/Logs/P3-3B-Gate3c.log` — 175/175; `P3-3C-Gate4d.log` — 191/192.

---

## Part D — Dynamics, markup, and observability

### F1.31 — Dynamic area markup (neutral)

**What it is.** `Request_AreaMarkup(shape, area-tag, enable)` — paint an area tag (and thereby a cost
multiplier or an impassable policy) over a box/sphere/cylinder/convex region at runtime, without a
full rebuild, returning a markup handle. Removal by handle. Overlapping markup: **usage tags OR
together, cost multipliers take the max**. Failure modes: a markup whose bounds lie in an unbuilt
region is accepted and applied when that region builds (recorded, not dropped); an invalid shape is
rejected at admission with no partial state.

**Why we need it.** `UCk_NavAreaMarkup_UE : UObject, INavRelevantInterface`
(`NavAreaMarkup/CkNavAreaMarkup_Utils.h:21,41`, the sole `INavRelevantInterface` site) is the
actor-free dynamic painter that **CkCrowd depends on** (R3 §C.6): stationary-agent cost markup
(`UCk_NavArea_CrowdAgent`) and avoidance volumes
(`CkCrowdAvoidanceVolume_Processor.cpp:35,95,148`). CkQueue is **not** a markup consumer — it
consumes projection and raycast only.

**What WE build.** Markup is an ECS-native record: a request enqueued on the ground-nav volume
entity, drained by a `HandleRequests` processor into a markup fragment holding value-only records
(shape as **`FCk_AnyShape`** — box, sphere, capsule, or cylinder — rasterized to cells at
markup-apply time, plus area tag, enabled state, stable id). The bake reads markup records as
an input (so markup participates in the content hash, F1.11) and stamps plate policy during
decomposition. **Markup that only changes cost — not walkability — updates plate policy without
re-deriving geometry**; markup that changes walkability triggers a local repair (F1.35). That split
is a documented, tested distinction, because it is the difference between a free update and a bake.

**Acceptance criteria.** Layer 1: painting an impassable box makes exactly the covered plates
impassable and no others; two overlapping cost regions yield the max; disabling restores prior
policy exactly (byte-compare against a field baked without the markup); a cost-only markup performs
zero geometry probes (assert on the probe counter). Layer 2: avoidance-volume autotests green.

**Provenance.** Volume-stamped area modifiers over a navigation field — the modifier/area-volume
model published in the open-source `recastnavigation` project (zlib) and Unreal's own nav modifiers.
**Design origin:** independently designed here as a *provider-dispatched* markup contract over a
neutral tag→policy registry. The nav-surface provider table gains three entries — `_ApplyAreaMarkup`,
`_IsMarkupLive`, `_ReleaseAreaMarkup` (`NavSurface/CkNavSurface_ProviderTable.h:60,63,69`) — and the
Recast adapter is filled with the utils' former bodies, so the neutral API no longer *is* the Recast
path. What an area tag means without a `UNavArea` is answered by `ck::nav_surface::Register_AreaPolicy`
(`NavSurface/CkNavSurface_AreaPolicy.{h,cpp}`): the impassable tag is a **walkability** policy, every
other registered tag a **cost multiplier** read off its `UNavArea` CDO. On CkGroundNav a markup is a
value record on the **volume** entity keyed by the markup entity
(`Volume/CkGroundNavVolume_{Fragment,Fragment_Data,Processor,Utils}`), its shape rasterized to cells
at bake time by `Bake/CkGroundNav_MarkupMask` (closed-square rule, per-span surface Z, analytic for
box/sphere/capsule/cylinder). Walkability records reject the covered spans inside
`DoFilter_Walkability` (`Bake/CkGroundNav_Walkability.{h,cpp}`) and re-bake the whole volume; cost
records are stamped onto plates after decomposition by `Stamp_PlateCostPolicies`
(`Bake/CkGroundNav_Plates.{h,cpp}` — tags OR, multipliers MAX, any-overlap-wins, **no plate split**,
the policy interned per plate field as `_AreaPolicyIndex`), which is also what let the plate merge's
third criterion — policy equality — finally be implemented. The cost-only path republishes through
the zero-probe derive `Field/CkGroundNav_FieldMarkupCost`, which copies the field and restamps every
tile without spending a geometry probe. Markup joins the frozen fingerprint enumeration
(`Bake/CkGroundNav_Fingerprint.{h,cpp}`); the admission-skip that fingerprint would enable is **not**
wired yet. No external code was copied, translated, or structurally mirrored — the modifier/area-volume
model is the `recastnavigation` lineage this cell already names. **Landed** on `feature/ck-navigation`
in CkFoundation `05a627f39`, tests in CkTests `b88bdbe2`, superproject pointer bump `8654219`
(2026-09-03). **Evidence:** `Saved/Logs/P4-4A-Final-GroundNav.log` — 226/226, zero ensures (the suite
went 198 → 226 with this slice); hermetic pins `CkTests.UnitTests.CkGroundNav.Bake.Markup_*` (7,
`Test_GroundNav_Markup.cpp` — impassable exactness, overlap max/union, disable byte-identity, seam
agreement, zero-probe cost derive, and both plate-merge policy rows), `Bake.MarkupMask_*` (5,
`Test_GroundNav_MarkupMask.cpp`), `Volume.Markup_*` (7, `Test_GroundNav_MarkupAdmission.cpp`),
`Bake.Fingerprint_EveryMarkupFieldPerturbsIt` +
`Bake.Fingerprint_MarkupOrderAndDisabledRecordsDoNotCount` (`Test_GroundNav_Fingerprint.cpp`), and
`CkNavigation.NavSurfaceAreaPolicy.*` (3, `Test_NavSurface_AreaPolicy.cpp`); the neutral AngelScript
contract test `CkAutoTest_NavSurface_AreaMarkupBecomesLive` is **byte-unchanged** and green on Recast.
Baseline pairs: `P4-4A-Final-Nav.log` 368/369, `P4-4A-Final-Crowd.log` 114/118 (baseline pair plus two
known flakes). **Two halves of the acceptance criteria above are deferred, not met.** "Layer 2:
avoidance-volume autotests green" is satisfied only on the **Recast** provider today — the
obstacle-suite crossover onto CkGroundNav is 4D. And walkability markup does **not** yet "trigger a
local repair (F1.35)": 4A re-bakes the whole volume, and the bounds-limited rebuild is 4C.

---

### F1.32 — Markup-live ground truth

**What it is.** `Get_IsMarkupLive(markup-handle)` → has this paint actually landed in the published
field yet? Plus the field epoch it landed in.

**Why we need it.** This is not a nicety: `CkCrowdAgent_PathRefresh_Processor.cpp:65-95` polls
`GetAreaID`/`GetPolyAreaID` to answer exactly this, and
`CkCrowdAvoidanceVolume_Processor.cpp:202-219` does the same for painted-area confirmation (R3 §C.7).
Without it, a system that paints an obstacle and immediately repaths races the bake and produces a
path straight through the obstacle it just painted.

**What WE build.** Each markup record carries the epoch at which it was applied; the query compares
that against the currently published epoch for the tiles the markup's bounds intersect. Because
epochs are derived at the read boundary and never stored as staleness flags (`R4 §1`), there is no
cache to go wrong.

**Acceptance criteria.** Layer 1: immediately after a markup request, `IsMarkupLive` is false; after
the covering tiles republish, it is true; a markup spanning two tiles is live only when **both**
have republished. Layer 2: the paint-then-repath race is pinned by an autotest that fails on the
pre-migration behaviour.

**Provenance.** Epoch/version-based visibility — our own proven pattern (`R4 §1`); no external claim.
**Design origin:** independently designed here as a **derived, strict** epoch comparison. Admission
stamps `_RequestedAtEpoch` (`Bake/CkGroundNav_MarkupTypes.h:85`) with the epoch published at that
moment; the record is live when every tile intersecting its world bounds is Built **and** carries an
epoch **strictly past** that stamp — an *equal* epoch is the publish that knew nothing of the record,
so `>=` would report a paint live one frame before it existed in the field. The answer is computed at
the read in `Facade/CkGroundNav_NavSurfaceAdapter.cpp:512` and nothing is stored, per the "derive at
the read boundary, never cache staleness" rule (`R4 §1`). A **disabled** record is not live on either
provider, matching Recast. The cost-only derive bumps a tile when its plate policy changed or when an
enabled record that tile has not yet observed reaches it, which is what makes a cost-only paint go
live without a geometry bake. Observability: `ck.GroundNav.MarkupAt`, markup outlines drawn in the
plate / `PathAt` / `FloodAt` views (`Debug/CkGroundNav_DebugDraw.cpp`), and the
`ck.GroundNav.Debug.MarkupLiveGate` bypass (`Debug/CkGroundNav_DebugGates.{h,cpp}`) — set to 0,
`Get_IsMarkupLive` answers true without asking the field, and says so in the log so a bypassed run can
never be mistaken for a passing one. No external code was copied, translated, or structurally
mirrored. **Landed** on `feature/ck-navigation` in CkFoundation `05a627f39`, tests in CkTests
`b88bdbe2`, superproject pointer bump `8654219` (2026-09-03). **Evidence:**
`Saved/Logs/P4-4A-Final-GroundNav.log` — 226/226, zero ensures; the Layer-1 half is
`CkTests.UnitTests.CkGroundNav.Volume.MarkupLive_*` (6, `Test_GroundNav_MarkupLive.cpp` — false before
the drain, false until the covering tiles republish, a two-tile record live only when **both**
republish, plus the attribute/multiplier and zero-probe-republish rows). The Layer-2 half — the
paint-then-repath race — is pinned in PIE by
`Ck_AutoTest_GroundNav_Markup_PaintThenRepathDoesNotCross` with the volume sliced to one tile a tick:
green **with** the live wait (`revisionAtPaint=46 revisionAtLive=54 framesWaited=17`, and the settled
route clears the painted region by 243 uu), and **red without it** — under
`ck.GroundNav.Debug.MarkupLiveGate 0` the test fails at step 14
(`Saved/Logs/P4-4A-PinBypass2.log`: `revisionAtPaint=8 revisionAtLive=8 framesWaited=1`), which is the
pre-migration behaviour the acceptance criterion asks the pin to fail on. Baseline pairs:
`P4-4A-Final-Nav.log` 368/369, `P4-4A-Final-Crowd.log` 114/118 (baseline pair plus two known flakes).
**Amendment ([S13-D9], session 13):** the epoch check alone was insufficient — a tile's epoch bumps on
mere reach, and a build already in flight when the paint drained publishes a higher epoch off the
record snapshot it took *before* the paint arrived. `Get_IsMarkupLive` now also requires the field to
have PRICED the record — the published `_Params._MarkupRecords` the plates were stamped from must
contain it, not just outlive its epoch — pinned by `Test_GroundNav_MarkupLive_RequiresPricing.cpp`.

---

### F1.33 — Surface revision and rebuild observability

**What it is.** `Get_SurfaceRevision(world)` — a monotone number that changes whenever any part of
the surface changes — and `BindTo_OnSurfaceRebuilt(bounds)` — a signal carrying the world-space
bounds that just changed. **One** implementation, per world.

**Why we need it.** Two *duplicate* revision observers exist today
(`Revision/CkNavigationRevision_Subsystem.cpp:21-66` and
`CkQueue/Navigation/CkQueue_NavigationRevisionSubsystem.cpp:30-64`), both watching Unreal's
generation-finished delegate. [NN-D8a] rules them **consolidated into the facade's revision API**.
`CkCrowdAgent_PathRefresh_Processor.cpp:140-152` and the avoidance-volume retirement logic build on
it.

**What WE build.** The aggregated chunk-epoch sum (F1.8) *is* the revision — no separate counter to
drift. The bounds signal is emitted by the publish step with the republished tiles' union bounds.
Signal defined with `CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE` and bound through the generated
`BindTo_OnSurfaceRebuilt`, per house doctrine.

**Acceptance criteria.** Layer 1: revision is strictly monotone across arbitrary interleavings of
tile rebuilds; the bounds signal fires exactly once per publish with bounds containing every changed
cell and no more than the union of republished tiles. Layer 2: both former subsystems' consumers
behave identically on the consolidated API. Layer 1 (contract): only one revision subsystem remains
(grep assertion).

**Provenance.** Monotone version counters + change-bounds notification — standard; our own epoch
lineage. **Design origin:** independently designed here as a **push** over the *existing* neutral
`OnSurfaceRebuilt` delegate — which was polled and bounds-less: the delegate now carries an `FBox`
of the world-space bounds that just changed, and a provider pushes it **once per publish** through
`ck::nav_surface::Request_NotifySurfaceRebuilt` (`NavSurface/CkNavSurface_Utils.{h,cpp}`) into
`FFragment_NavSurface_PendingRebuilds` on the world entity. `FProcessor_NavSurface_RevisionWatch`
(`NavSurface/CkNavSurface_Processor.{h,cpp}`) drains that queue in a pinned
`FGroup_Gameplay_TimeDelta` — one broadcast per queued box, in queue order — and the
revision-compare poll it used to be is kept **only as a fallback** for providers that never notify,
whose payload is an *invalid* box meaning "unknown". Recast is exactly such a provider in this
slice: its generation-finished delegate carries no bounds, so Recast fires none. CkGroundNav fires
from both of its publish sites — the build swap (`Volume/CkGroundNavVolume_Processor.cpp`) and the
cost-derive swap (`Field/CkGroundNav_FieldMarkupCost.{h,cpp}`) — with `Get_ChangedTileBounds`, the
union of the **BUILT** tiles carrying the new epoch. `Get_SurfaceRevision` on GroundNav is
`FCk_GroundNav_Field::Get_AggregatedTileEpochSum` summed over the world's fields
(`Facade/CkGroundNav_NavSurfaceAdapter.cpp`), monotone per publish because every publish assigns a
strictly greater epoch to at least one tile. Its two holes are **pinned, not reshaped**: the sum is
monotone only while tile count is invariant across rebuilds (it is), and a torn-down volume does not
lower the world revision, because the world-field registry retains that term until world cleanup —
both documented at `Do_SurfaceRevision`. No external code was copied, translated, or structurally
mirrored. **Landed** on `feature/ck-navigation` in CkFoundation `fa25d7de8`, tests in CkTests
`0c1e95d3`, superproject pointer bump `dff929d` (2026-09-03). **Evidence:**
`Saved/Logs/P4-4B-Final-GroundNav.log` — 242/242, zero ensures (the suite went 226 → 242 with this
slice); the signal half is
`CkTests.UnitTests.CkNavigation.NavSurface.{RebuiltFiresExactlyOncePerQueuedPublish,
RebuiltDrainsToEmptyAndRefiresNothing, FallbackFiresOnceWithUnknownBoundsWhenOnlyTheRevisionMoved}`
(`Test_NavSurface_RebuildSignal.cpp`, listener in `CkTest_RebuiltListener.h`); the revision half is
`CkGroundNav.Revision.{ChangedTileBoundsIsTheUnionOfRepublishedTilesOnly,
TileEpochSumRisesOnEveryInterleaving, TileCountIsInvariantAcrossRebuilds,
WorldRevisionDoesNotFallWhenAVolumeIsTornDown}` (`Test_GroundNav_Revision.cpp`,
`Test_GroundNav_RevisionWorld.cpp`) — the last two rows are the two holes, pinned as they stand. The
neutral AngelScript contract test `CkAutoTest_NavSurface_RebuildAdvancesRevisionAndSignals` is green
on Recast with only the payload widened. Only one revision subsystem remains: `OnNavigationGenerationFinished` is bound at a single site (`Revision/CkNavigationRevision_Subsystem.cpp`), re-verified by grep at PHASE_4 entry and again at this ship. Baseline pairs: `P4-4B-Final-Nav.log` 387/388,
`P4-4B-Final-Crowd.log` 114/118 (baseline four). **Three halves are deferred, not met.** Recast
fires no changed bounds (its delegate carries none), so push invalidation is GroundNav-only; a
walkability paint still re-bakes the whole volume in this slice, so its changed bounds are the whole
volume (4C narrows them); and a headless test cannot install a published field on a volume entity,
so "cost derive queues exactly one broadcast" is covered by the PIE pins rather than a hermetic one.

---

### F1.34 — Path invalidation on surface change

**What it is.** A path whose corridor intersects a changed region (inflated by a margin of order the
agent width) is flagged for repath. Implemented as a **push** (rebuild bounds → overlap test against
live paths) rather than per-path polling.

**Why we need it.** Today CkCrowd polls build state and revisions (`PathRefresh` `140-152`) and
re-searches broadly. Push-based invalidation is what makes a moving-obstacle world affordable.

**What WE build.** On the `OnSurfaceRebuilt` signal, an ECS processor tests changed bounds against
each active path's cached corridor bounds and sets a repath-required tag; the next `PathRefresh`
tick consumes the tag and runs plan repair (F1.30) rather than a cold search. Bounds are values;
no path holds a reference to the field.

**Acceptance criteria.** Layer 1: a rebuild adjacent to but not intersecting a corridor does not
flag it; one intersecting it does; the inflation margin is respected exactly. Layer 2: an obstacle
dropped onto a crowd's path causes a repath within a bounded number of frames (measured, recorded).

**Provenance.** Bounds-overlap change propagation; standard spatial invalidation. **Design origin:**
independently designed here as a corridor-bounds push. A GroundNav route caches its corridor's world
bounds on `FFragment_GroundNavPath_Current` (`Path/CkGroundNavPath_Fragment_Data.h` —
`_LastCorridorBounds`, inflated at store by the agent radius plus `kCorridorInflationMarginUu` = one
cell (25 uu), recorded as `_CorridorInflationUu`, alongside `_LastCorridorEpoch`).
`FCk_Request_GroundNavPath_FindPath` gained a `_PlanMode` (`Cold` | `Repair`): `Repair` reuses the
cached corridor keys through the P3 `Request_BeginRepair` seam — this is that seam's first caller —
and reports its verdict on the result as a reflected `ECk_GroundNav_RepairVerdict`.
`FProcessor_GroundNavPath_InvalidateOnRebuilt` (`Path/CkGroundNavPath_Invalidate_Processor.{h,cpp}`,
`FGroup_Gameplay_TimeDelta`, `RunBefore` the drain) reads the pending-rebuild queue, skips a
corridor already planned against the current epoch, and tags `FTag_GroundNavPath_RepathRequired`
wherever a changed box intersects the cached bounds — closed intersect; an unknown box flags every
path; GroundNav-gated, so a Recast world is untouched; and the `ck.GroundNav.Debug.RepathOnRebuild`
bypass announces itself in the log so a bypassed run can never be mistaken for a passing one. The
crowd's `PathRefresh` (`Agent/CkCrowdAgent_PathRefresh_Processor.cpp`) consumes that tag **first**,
ahead of its serial early-out, keyed on `_ActiveProvider == GroundNav` (shadow mode gives Recast
agents a live GroundNav corridor too), re-dispatches through the existing fork with the defaulted
plan-mode parameter set to `Repair`, keeps the agent `Walking` — the install swaps the polyline
mid-walk — and logs one `[REBUILD-REPLAN]` line. Observability: `ck.GroundNav.Invalidation`,
`ck.GroundNav.Debug.DrawInvalidation` (corridor and changed-bounds overlays), and an
`epoch [..] repair [..]` field on the path publish line. No external code was copied, translated, or
structurally mirrored. **Landed** on `feature/ck-navigation` in CkFoundation `fa25d7de8`, tests in
CkTests `0c1e95d3`, superproject pointer bump `dff929d` (2026-09-03). **Evidence:**
`Saved/Logs/P4-4B-Final-GroundNav.log` — 242/242, zero ensures (the suite went 226 → 242 with this
slice). Layer 1 is hermetic:
`CkGroundNav.Path.{CorridorBoundsCoverEveryWaypointInflatedByRadiusPlusMargin,
RepairPlanModeReusesTheCachedCorridor, RepairWithoutACachedCorridorPlansCold}` and
`CkGroundNav.Invalidation.{AdjacentBoundsDoesNotFlag, IntersectingBoundsFlags,
MarginIsRespectedExactly, OnePublishFlagsEachPathAtMostOnce, UnknownBoundsFlagsEveryPath,
RecastWorldIsUntouched, CorridorPlannedAgainstTheCurrentEpochIsNotFlagged}`
(`Test_GroundNav_PathPlanMode.cpp`, `Test_GroundNav_PathInvalidation.cpp`) — the first three
Invalidation rows are this cell's three Layer-1 criteria verbatim. Layer 2 is pinned in PIE by
`Ck_AutoTest_GroundNav_Rebuild_InvalidatesWalkingRouteExactlyOnce`
(`routeSwaps=1 framesFromLiveToNewRoute=3 nonWalkingFrames=0 repairVerdict=FullReplan`), which
**fails** under `ck.GroundNav.Debug.RepathOnRebuild 0` (`P4-4B-PinBypass2.log`:
`repairVerdict=None revisionAtPaint=15 revisionAtLive=30 framesFromLiveToNewRoute=1303`), and by
`Ck_AutoTest_GroundNav_Rebuild_AdjacentPaintDoesNotReplan` (`routeSwaps=0 changedBoundsLocal=true`).
The "bounded number of frames (measured, recorded)" is 3 frames of re-plan latency against the A6
Recast markup-box baseline of 2 — logged, not asserted. Baseline pairs: `P4-4B-Final-Nav.log`
387/388, `P4-4B-Final-Crowd.log` 114/118 (baseline four). **Two halves are deferred, not met.**
Recast fires no changed bounds (its delegate carries none), so push invalidation is GroundNav-only;
and a walkability paint still re-bakes the whole volume in this slice, so its changed bounds are the
whole volume (4C narrows them).

---

### F1.35 — Local repair (moved/added/removed geometry)

**What it is.** When geometry changes within known dirty bounds, re-derive **only** the affected
region: re-probe only the columns whose inflated probe box intersects the dirty bounds, re-run the
filters, distance transform, plate merge, and portal extraction for the touched tiles, and publish a
new field. Repair **never mutates the published structure**. A repaired field must be identical to a
full rebake of the same world state.

**Why we need it.** Doors, destructible cover, and moved props are gameplay; today they cost a Recast
tile rebuild we do not control. It is also the promotion gate: "moved-obstacle repair latency ≤ the
Recast-measured baseline on the same fixtures" (`MIGRATION_SEAM.md` §4 (promotion item 3)).

**What WE build.** Exactly the volumetric repair discipline (`R4 §1`): dirty-bounds fragment +
`NeedsRepair`/`RepairInProgress` tags, re-probe only intersecting cells, assemble a new structure and
swap. **Because the plate decomposition is derived and never patched, corruption is
unrepresentable** — repair is partial re-derivation, which is the property `REPRESENTATION.md`
selected candidate B for. The volumetric tests
(`RepairedOctreeMatchesAFullRebake`, `MovedObstacleFlipsOccupancyOnlyWhereItMoved`,
`SlicedRepairMatchesOneShotRepair`) transfer one-for-one.

**Acceptance criteria.** Layer 1: `RepairedFieldMatchesAFullRebake` (byte-identical);
`MovedObstacleChangesOnlyWhereItMoved` (cells outside the union of old+new bounds are untouched);
`SlicedRepairMatchesOneShotRepair`; probe count for a small repair is bounded well below a full bake
(tracked number). Layer 2: a door opening/closing in a gym flips reachability in the expected number
of frames.

**Provenance.** Localized re-derivation with dirty-bounds tracking — standard incremental
recomputation; our own proven, test-pinned pattern (`R4 §1`). **Design origin:** independently
designed here as a repair whose **tile set is fixed when the repair opens**, over a snapshotted dirty
box. `Field/CkGroundNav_FieldRepair.{h,cpp}` carries `FCk_GroundNav_FieldRepairState` and four entry
points. `Get_RepairTileIndices` inflates the dirty world box in XY by the halo —
`ceil(MaxClearance/cell)*cell`, 200 uu at defaults — and selects every tile whose bounds it meets,
the tiles placed from the field params so a *failed* tile still selects.
`Request_BeginRepair(state, source, box, epoch, currentMarkupRecords)` copies the published field,
fixes that tile set, and bakes under the **caller's** records. `Request_AdvanceRepair` mirrors the
build slice: per-slice and per-tile world-revision checks failing closed, the budget spent after the
first tile, an identical fetch shape per tile — then a closure pass over **every** tile in index
order with a fresh checked set, and the seam, boundary and reachability passes recomputed fully, so
the result is byte-identical to a full bake with tile epochs excluded and only re-baked tiles carry
the new epoch, which is what makes the changed bounds exact and keeps the tile-epoch sum monotone.
`Request_ReleaseRepairedField` closes it. On the volume:
`FCk_Request_GroundNavVolume_Repair{_DirtyBounds}` (a moved body = old ∪ new),
`FFragment_GroundNavVolume_RepairState`, the `NeedsRepair`/`RepairInProgress` tags, and
`FProcessor_GroundNavVolume_{HandleRepairRequests,StartRepair,Repair,CancelPendingRepairRequests}` —
dirty bounds coalesce into one box while pending and are **snapshotted at open**; a build's publish
supersedes pending repairs (Succeeded); a build arming cancels an in-flight repair; a stale-geometry
failure retries once then drops with an ensure; a repair that raced a publish is discarded and its
region re-raised. A walkability markup change now raises a repair over the record's old **and** new
footprint — the whole-volume rebuild F1.31 and F1.33 deferred against is retired — the cost derive
and the repair carry the current markup records, and a release keyed on a markup entity already in
teardown still finds its record (both of those 4A defects were found by the PIE pins).
Observability: `ck.GroundNav.RepairAt`, epoch tint in the tile view, dirty and repair boxes in the
invalidation overlay, `RepairHighlightSeconds`. The in-house lineage is CkVoxelNav's octree repair
(same dirty-bounds contract); no external code was copied, translated, or structurally mirrored.
**Landed** on `feature/ck-navigation` in CkFoundation `2ece50c53`, tests in CkTests `a80815e7`,
superproject pointer bump `8dfcb70` (2026-09-03). **Evidence:** `Saved/Logs/P4-4C-Gate3.log` —
255/255, zero ensures (the suite went 242 → 255 with this slice). Layer 1 is hermetic:
`CkTests.UnitTests.CkGroundNav.Repair.{LatticeArithmeticIsWhatTheFixtureAssumes,
RepairedFieldMatchesAFullRebake, MovedObstacleChangesOnlyWhereItMoved,
SlicedRepairMatchesOneShotRepair, HeldPreRepairFieldIsUnchanged,
ZeroTileDirtyBoxCompletesWithoutChange, StaleGeometryFailsClosed,
RepairBakesUnderTheCallersRecordsNotTheSourceFields}` (`Test_GroundNav_FieldRepair.cpp`) — the
moved-obstacle row changes 3 of 9 tiles and carries this cell's tracked probe number
(`[REPAIR-BUDGET] probes=134757 fullBakeProbes=397539`) — plus
`Bake.Markup_CostDeriveCarriesItsRecordsInTheParams`,
`Volume.Markup_ReleaseNamingADestroyedEntityStillRemovesTheRecord` and
`Volume.Markup_WalkabilityRecordMarksNothingUntilAFieldIsPublished`. Layer 2 is pinned in PIE by
`Ck_AutoTest_GroundNav_Repair_DoorToggleRepairsLocally` (close 9 frames / open 9 frames each round on
a 5x5 field sliced one tile a tick, changed bounds = the 9-tile union X -600..600, never the 2000 uu
volume) and `Ck_AutoTest_GroundNav_Repair_MovedMarkupBoxChangesOnlyWhereItMoved` (both halves 19
frames on an 8x3 field, changed bounds = columns 1..6, columns 0 and 7 never touched). The
comparator is now shared — `Test_GroundNav_FieldEquality.h`, with a first-difference diagnostic,
covering `_OpenBodies` and `_TileEdgeBoundary`. Baseline pairs: `P4-4C-Final-Nav.log` 400/401,
`P4-4C-Final-Crowd.log` 114/118 (baseline four). **Four halves are deferred, not met.** The
moved-obstacle latency against Recast's 2 + 2 frames is **recorded, not claimed ≤** — GroundNav's
19 is a one-tile-a-tick fixture and at the default budget a repair is a single slice, so the
comparable number is probes and scaling, not frames. "A repaired tile that then fails publishes its
failure" has no hermetic pin: the stub backend has no failure injection. CkJolt has no body-moved
event, so a moved **real** body's caller supplies the old ∪ new bounds. And the geometry
world-revision check is world-wide and static-only, so a geometry-driven multi-frame repair fails
closed on any static registration anywhere.

---

### F1.36 — Multi-world safety

**What it is.** All provider state — fields, markup, revisions, deferred requests, build scheduler —
is per world (or per volume entity within a world). No process-wide truth. Worlds tear down without
leaking and without ensures.

**Why we need it.** Explicit promotion gate ("Multi-PIE and teardown clean", `MIGRATION_SEAM.md`
§4, promotion item 6) and a *known current defect*: `GDeferredNavRequests` is a global, documented as not
multi-PIE-safe at `CkNav_Processor.cpp:35` and echoed in the module CLAUDE.md.

**What WE build.** Fields live on ECS volume entities; the deferred queue moves per-world behind the
facade ([NN-D8b]); nothing static holds a field, a handle, or a `UWorld`. Teardown follows the
house `EndPlay`/`Destructor` processor vocabulary, with pending requests completed
`Failed_Cancelled` via `ck::request::FireCancelledForPending`.

**Acceptance criteria.** Layer 2 (multi-PIE): two worlds with independent fields and markup show
zero cross-talk; ending one PIE session leaves the other's queries correct; a fresh startup log shows
zero new ensures and zero script errors after world teardown. Layer 1 (contract): grep assertion that
no global/static holds provider state.

**Provenance.** No algorithmic claim. **Landed** on `feature/ck-navigation` in CkFoundation `8b58149ef`,
tests in CkTests `89a828dc`, superproject pointer bump `92f4f9e` (2026-09-03). **Evidence:** `Saved/Logs/P4-4D-Gate4.log`
(271 / 271, 1 ensure = Iris `DataStreamChannel.cpp:244`). Layer 1: `CkTests.UnitTests.CkGroundNav.MultiWorld.*` (6 pins — per-world field
registries, the `TryGet_Field` bounds rule, and `DestroyedVolumeLeavesTheRegistry`: a volume leaves the
world-field registry at end-play and its tile epochs are carried as retired revision, so
`Revision.WorldRevisionDoesNotFallWhenAVolumeIsTornDown` holds exactly while a dead volume answers nothing),
`NavSurface.ProviderSwitchResyncsWithoutBroadcasting`, and the type-shaped grep over CkGroundNav + CkNavigation
whose every value-holding static is world-keyed and cleared at `OnWorldCleanup` (the world-field registry, the
debug field cache, the provider and shadow-mode mirrors) or code-shaped and seeded once (provider tables, area
policies, Recast tag tables). Layer 2: `Ck_AutoTest_Net_GroundNav_TwoWorldsDoNotShareFields` — server and
client each stage a field on their own band, a paint in one moves the other's revision by nothing, and each
hears only its own band's rebuilds (`rebuiltOnOtherBand=0`); teardown is ensure-free on GroundNav's side (the
one ensure every `Nav` run carries is Iris `DataStreamChannel.cpp:244`, pre-existing and named). The crossover
found the one leak this row had: a destroyed volume's field kept answering the world until world cleanup, so a
later test's mesh probe was satisfied by ground that no longer existed.

---

### F1.37 — Deterministic test rebuild hook

**What it is.** `Request_SurfaceRebuild_ForTesting(world)` — force a full, synchronous-to-completion
rebuild so a test can proceed from a known-settled field, plus a named "surface settled" condition
tests wait on.

**Why we need it.** [NN-D8d] rules the existing hook (`CkNav_Utils.cpp:191-198`, `NavSys->Build()`)
**kept as a neutral facade call** — both providers need a deterministic test hook. `R4 §7`'s settling
discipline forbids fixed hop counts; the named condition is the alternative.

**What WE build.** A facade call routed to the active provider; on CkGroundNav it drains the build
scheduler to completion under the slice budget and republishes.

**Acceptance criteria.** Layer 2: an autotest that paints markup, calls the hook, waits on the named
condition, and asserts markup-live — with no `WaitOneFrame` hop counting.

**Provenance.** No algorithmic claim. **Landed** in the same commits as F1.36. **What shipped:** the hook is
a KICK, never a blocking call — `Request_SurfaceRebuild_ForTesting` routes to the provider (Recast `Build()`;
GroundNav a `Request_Build` on every volume of the world) and the named condition is the fourteenth
provider-table capability `Get_IsSurfaceSettled(world)`: Recast = nav data present, health Ready and no dirty
areas queued; GroundNav = every volume `Get_IsSettled` (a published field, not building, no repair open or
pending, no cost derive owed, nothing queued) AND no markup request or release still riding the pipeline. Layer
1: `Test_NavSurface_Settled.cpp` (the table is incomplete without the entry; Recast is not settled without nav
data; pending markup work is not settled) and `Test_GroundNav_Settled.cpp` (5 pins over the volume markers and
queues). Layer 2: `Ck_AutoTest_GroundNav_Settle_PaintThenSettleThenLive` — stage, switch, paint, release,
rebuild, each waited on SETTLED only and sampled at the first settled poll: settled implies live and the hole
already cut (`[GROUNDNAV-SETTLE]` switch 1 / paint 2 / release 4 / rebuild 4 frames), and
`Ck_AutoTest_GroundNav_Markup_PaintThenForcedRebuildKeepsThePaint` — a paint and the kick in ONE step keep the
paint. The ten Recast obstacle fixtures now settle through the neutral pair (`Try_ProjectPoint` +
`Request_SurfaceRebuild_ForTesting`, the Queue one also `Get_IsSurfaceSettled`), delta-zero on Recast, and
on GroundNav stage their own field over the level's navmesh bounds through `Script/Common/CkAutoTest_GroundNavFixture.as`
(`[GROUNDNAV-CROSSOVER]` one line per site). The crossover's per-site results live in PROGRESS's 4D block.

---

## Part E — Seam, coexistence, and fixtures

### F1.38 — The provider-neutral ground-query facade

**What it is.** `UCk_Utils_NavSurface_UE` (working name) — one utils facade exposing the twelve
capabilities in `MIGRATION_SEAM.md` §2 (`Try_ProjectPoint`, `Try_MoveAlongSurface`,
`Try_SurfaceRaycast`, `Get_BoundarySegments`, `Get_IsReachable`, `Request_AreaMarkup`,
`Get_IsMarkupLive`, `Get_SurfaceRevision`, `BindTo_OnSurfaceRebuilt`, `Get_SurfaceBounds`,
`Get_ProviderHealth`, `Get_IsBuildInProgress`), plus point generation and surface attributes. It is
ECS-idiomatic (utils facade + fragments), **not** a `UObject` interface. It carries a documented
threading contract per function — `Get_BoundarySegments` is explicitly off-game-thread-callable
against an immutable snapshot; the Recast adapter keeps the existing stack-local-query discipline to
honour it.

**Why we need it.** It is the migration: consumers move to it once and never learn which provider
answered. R3's dependency table lists every migrating site: CkCrowd (ConstrainToNavmesh,
AvoidanceSample, Steering, OnPathResolved, OnRouteResolved, BlockDetect, PathRefresh,
AvoidanceVolume), CkPathNetwork, CkEqs, CkQueue, CkGameplayDebugger.

**What WE build.** The facade + a per-world provider fragment. Every function must work in **C++,
Blueprint and AngelScript** (non-negotiable #4), which for AngelScript means `EOrder::Late`
registration for any binding taking `FString`/UE types (a known trap) and generated `utils_*.as`
wrappers. Sites bucketed `[RETIRE]` are **deleted, not migrated**:
`CkCrowdAgent_DiagNavClip_Processor.cpp:213-329`, `CkCrowdAgent_DrawNavProjection_Processor.cpp:53-64`,
`CkCrowd_DebugSettings.cpp:124`, and the diagnostic re-probe block at `CkNav_Algorithm.cpp:163-196`.
[NN-D8h]: the stale `CkCrowd.Build.cs:21` comment is corrected in this mechanical pass.
[NN-D8g]: both EQS inline-projection sites migrate to the facade and are re-measured; the "keep the
two in sync" note dies with the duplication unless measurement vetoes.

**Acceptance criteria.** Layer 1: the facade returns identical results to direct provider calls for
every capability. Layer 2: every migrated consumer's existing autotests green on the Recast provider
*through the facade* (this is the no-behaviour-change gate for Step 2, before CkGroundNav exists).
Layer 1 (contract): zero Unreal-Navigation types in the facade's public headers; every function
callable from AngelScript (a binding-presence assertion). `[EDITOR-VERIFY]`: Blueprint nodes appear
with correct categories and pins.

**Provenance.** No algorithmic claim.

---

### F1.39 — Recast adapter and provider selection

**What it is.** The existing Recast behaviour, moved behind the facade and behind the provider fork,
kept selectable for the whole migration. Provider choice is a per-world/project setting plus the
existing per-agent fork; the default is unchanged until promotion; rollback is a one-setting revert.

**Why we need it.** No flag-day ([NN-D8] ground rules). Every phase leaves the Recast path selectable
and green, and Recast is the A/B oracle that produces the promotion evidence.

**What WE build.** An adapter compiling neutral filter definitions into `UNavigationQueryFilter`
instances and neutral area tags into the existing framework `UNavArea` classes
(`UCk_NavArea_Restricted`, `UCk_NavArea_CrowdAgent`, the three avoidance-volume areas), plus the
existing dispatch. CkCrowd's `RequestPathForActiveGoal` (`:488-557`) gains a fourth branch beside
VoxelNav (`503-528`) → PathNetwork (`530-554`) → CkNavigation (`556`); the episode lifecycle
(advance revision → abandon previous → mark pending → dispatch exactly one → install only a fresh
result → release terminal episodes) is **preserved verbatim**. [NN-D8c]:
`Request_SetActorNavigationRegistered` is kept through migration because the adapter needs it, and
retires with the adapter (CkGroundNav's geometry is physics-backend-sourced).

**Acceptance criteria.** Layer 2: full crowd/queue/pathnetwork suites green on the Recast provider
through the facade, delta-zero against the pre-migration baseline. Layer 1: switching providers at
runtime does not leak state between them.

**Provenance.** No algorithmic claim.

---

### F1.40 — Shadow / A-B parity mode

**What it is.** A setting that dispatches the same request to both providers; the Recast result
installs (authoritative), the CkGroundNav result is compared on waypoint count, length delta,
endpoint delta, success/failure agreement, and query time. Divergences are logged and counted into a
value-only diagnostics fragment the debugger renders.

**Why we need it.** It **is** the promotion evidence (`MIGRATION_SEAM.md` §4 (promotion item 1)) — same-fixture A/B
parity gathered on the existing crowd/queue/path-network test maps and gyms.

**What WE build.** A comparison processor plus `FCk_GroundNav_ShadowDiagnostics` — values only
(counts, deltas, histograms, ids), never handles or field shares, following the value-only debug
snapshot rule (`R4 §1`). Divergence counters are per fixture so VALIDATION.md can table them.

**Acceptance criteria.** Layer 2: shadow mode runs the full suites with zero crashes and produces a
divergence report; the report's schema is stable enough for VALIDATION.md to diff run over run.
The *content* thresholds (agreement rate, length-delta budget) are promotion gates, recorded in
VALIDATION.md as measured numbers, not estimated ones.

**Provenance.** Differential/shadow testing (standard practice). **Design origin:** independently
designed here as a per-world A/B mode. `ECk_NavSurface_ShadowMode {Off, GroundNavShadowsRecast}`
sits beside the provider selector in
`CkNavigation/Public/CkNavigation/NavSurface/CkNavSurface_Fragment_Data.h`, mirrored per world
behind an `FRWLock` (`Get_ShadowModeForWorld` / `Set_ShadowModeForWorld` in
`CkNavSurface_ProviderTable.{h,cpp}`), project default **Off** (`Settings/CkNav_ProjectSettings.h`),
reached through `UCk_Utils_NavSurface_UE::Request_SetShadowMode` / `Get_ShadowMode`. The shadow
query is appended after the authoritative Recast `Request_FindPath` in
`CkCrowd/.../Agent/CkCrowdAgent_HandleRequests_Processor.cpp` — every re-dispatch site shadows and
`_ActiveProvider` is untouched — carrying `_IsShadow` on `FCk_Request_GroundNavPath_FindPath` and on
`FCk_GroundNavPath_Result` alongside `_SearchDurationMs`; the ignore gate is the **first** statement
of `FProcessor_CrowdAgent_OnGroundNavPathResolved` (Gate 3D-1), so a shadow result is compared and
discarded, never installed. Diagnostics live in `CkGroundNav/Public/CkGroundNav/Shadow/` as the
values-only `FFragment_GroundNav_ShadowDiagnostics` on the world transient entity (per-fixture
counters, six 12-bucket histograms with interpolated p95, a 5×9 status-pair matrix, diverging query
ids), with `shadow::Accumulate` / `Get_Report` emitting a 29-column diff-stable schema (`schema=1`),
surfaced by `UCk_Utils_GroundNav_Shadow_UE` (`Request_Begin/EndShadowFixture`, `Get_ShadowReport`,
`Get_ShadowComparisonCount`, `Request_ResetShadowDiagnostics`) and `ck.GroundNav.ShadowReport`. The
comparison is `CkCrowd/Public/CkCrowd/Shadow/CkCrowd_ShadowCompare_Processor`, firing only on a
fresh + shadow + same-revision result in a terminal nav slot, summing each side's length over its
own waypoints from the agent's position and emitting one `[SHADOW-CMP]` Display line per comparison
keyed by `_QueryId` = label#rev; the `DrawShadowRoutes` overlay (`ck.Crowd.DrawShadowRoutes`) draws
both providers' routes. No external code was copied, translated, or structurally mirrored.
**Landed** on `feature/ck-navigation` in CkFoundation `60e06b084`; tests in CkTests `2f801582`.
**Evidence:** Gate 5a `Saved/Logs/P3-3D-Gate5a.log` — 198/198
(`Shadow_InstalledPathIsByteIdenticalToRecast`, `Shadow_CountersMoveOnAShadowRun`,
`Shadow_ReportSchemaIsStable`, plus three C++ `Shadow.*` rows in
`Test_GroundNav_ShadowDiagnostics.cpp`); final gates on the ship artifact `P3-3D-K-GroundNav.log`
198/198, `P3-3D-K-Nav.log` 335/337 (baseline pair), Gate 9 `Crowd` 115/118 (baseline pair plus a
known flake); scoped shadow sweeps `P3-3D-S2-Crowd.log` (1,350 comparisons, all Recast-only — the
crowd suite has no GroundNav field over its fixtures) and `P3-3D-S2-Nav.log` (fielded fixtures 6/6
both succeed, length Δ ≤ 28.3 uu, endpoint Δ 10 uu, GroundNav 0.010–0.015 ms vs Recast
0.011–0.025 ms per query); the values-only grep over the fragment and report files is 0 hits. The
full-suite shadow report — the Layer-2 half of the acceptance criteria above — is deferred to
campaign end per [NN-D54].

---

### F1.41 — Fallback and failure parity

**What it is.** Any CkGroundNav hard failure (no field, unbuilt region, budget exhaustion) fails the
episode **exactly as a Recast failure does today** — same fail reasons, same signals, same
waypoint-preservation behaviour — so CkCrowd's retry and escape machinery is provider-blind.

**Why we need it.** `MIGRATION_SEAM.md` Step 3 requires it; CkCrowd's escape path and corridor
splice (`PathRefresh` `461-486`, `799-825`) are tuned against the current failure vocabulary.

**What WE build.** A total mapping from CkGroundNav statuses onto the existing
`FCk_Nav_PathResult` fail-reason enum, with a Layer-1 exhaustiveness test — every new status maps to
exactly one existing reason, and adding a status without extending the map fails to compile or fails
the test.

**Acceptance criteria.** Layer 1: exhaustive status→reason mapping test. Layer 2: crowd failure-path
autotests (`NoRouteFailsClean`, `Stall_UnreachableGoalFailsBounded`) green on CkGroundNav with the
same signal sequence.

**Provenance.** No algorithmic claim. **Design origin:** independently designed here as a **total**
status→fail-reason table, `kGroundNavVerdicts`, living in CkCrowd: `Unreachable` → `FindPathNoPath`,
`NoStartSurface` → `StartProjectFailed`, `NoGoalSurface` → `EndProjectFailed`, `BudgetExceeded` →
`BudgetExceeded`, `Blocked` → `FindPathInvalid`, `Unbuilt` → `NoNavData` (only once the deferral
bound is spent), and `InProgress` → defer rather than fail. Totality is enforced at compile time by a
count check plus `consteval` identity `static_assert`s, so adding a status without extending the
table fails to compile — the acceptance criterion above is discharged by the compiler, not only by a
test. Waypoint preservation on failure is structural rather than tested-in: `FailPath` never writes
`_Waypoints`. No external code was copied, translated, or structurally mirrored. **Landed** on
`feature/ck-navigation` in CkFoundation `df65f4988` — the verdict table in
`CkCrowdAgent_GroundNavInstall_Algorithm.h`, the CkCrowd provider gate at the top of
`Request_NavigationPath`, `CkNav_Algorithm`'s `InInstallAs`, and
`CkCrowdAgent_OnGroundNavPathResolved_Processor`; the four AngelScript crowd autotests in CkTests
`e9edfcd1` are green with the same signal sequence as their Recast twins. **Evidence:**
`Saved/Logs/P3-3C-Gate4d.log` — 191/192; `P3-3C-Stall.log` — 4/4; `P3-3C-Nav.log` — 328/331
(baseline reds only); `P3-3C-Crowd.log` — 113/118 (baseline reds plus known flakes only).

---

### F1.42 — Test fixture vocabulary ("punch an impassable box at runtime")

**What it is.** The AngelScript test/gym vocabulary for creating and removing runtime obstacles must
keep working. Today that is `UNavArea_Null` used as a literal class to punch navmesh holes.

**Why we need it.** R3 §CkTests: 10+ AutoTests and gyms depend on it —
`NarrowGap_TraverseCalm`, `NarrowGap_NoRouteFailsClean`, `NarrowGap_BlockedDetours`,
`Stall_RepathsAroundLateObstacle`, `Stall_UnreachableGoalFailsBounded`,
`Steering_CornerRetirementKeepsAgentOnMesh`, `OffPath_TeleportRepaths`,
`Facing_CalmWhilePressingBlockedGap`, `CkQueueGym_PlayerController:1377`,
`Queue_NavigationChangeRetriesImpossibleFormation:89`, plus
`CkTestsAssets.as:794,7955` (`TSoftObjectPtr<ARecastNavMesh>`). **Explicit promotion gate**
(`MIGRATION_SEAM.md` §4 (promotion item 2)): "the `UNavArea_Null`-derived fixture vocabulary fully served by neutral
markup". Losing it means the crowd suite loses its obstacles.

**What WE build.** The neutral markup request (F1.31) with a well-known "impassable" area tag, and
`utils_navsurface`-level AngelScript wrappers so a fixture reads as one call. The
`TSoftObjectPtr<ARecastNavMesh>` asset references in `CkTestsAssets.as` are replaced by the neutral
provider handle. Migration happens in the **same phase** as Step 1's contract neutralization so no
test is red across a phase boundary.

**Acceptance criteria.** Layer 2: all ten-plus named tests green on the Recast provider through
neutral markup (no behaviour change), then green again on CkGroundNav. Layer 3: gym stations that
paint obstacles still work with reserved keys and control-panel rows unchanged.

**Provenance.** No algorithmic claim.

---

### F1.43 — Hermetic bake fixtures and the math-core boundary

**What it is.** The entire bake and query math must be exercisable with **no ECS, no `UWorld`, no
physics** — against hand-authored geometry lists — so Layer-1 tests are fast, deterministic, and
debuggable.

**Why we need it.** It is how CkVoxelNav achieves 68 automation tests (`R4 §1`, `§7`) and the reason
its repair is trustworthy. Without this boundary, every nav test becomes a PIE test and the suite
becomes unaffordable and flaky.

**What WE build.** A strict module split: `CkGroundNav`'s math core (rasterize → filter → layer →
clearance → plates → portals → search → funnel) as free functions and value types over a
`FCk_GroundNav_GeometryBatch`, with the ECS layer (fragments, processors, requests, utils) a thin
shell above it — the same shape as `CkPathNetwork_Build.h` ("pure math, runtime-callable, no
ECS/world dependency", `R4 §4`).

**Acceptance criteria.** Layer 1: the full bake→query→search pipeline runs in a test with the `_Stub`
geometry backend and zero engine subsystems; test naming `Ck.GroundNav.<Area>.<Scenario>`,
`IMPLEMENT_*AUTOMATION_TEST` only, zero `DEFINE_SPEC` (`R4 §7`); PIE-requiring tests isolated in
`*Pie.cpp` files.

**Provenance.** No algorithmic claim.

---

# TIER 2 — required for Recast retirement

> During Tier 2 the Recast adapter is still present. These features remove the *reasons* it is still
> present. Retirement is a separate phase with its own CTO sign-off (`MIGRATION_SEAM.md` §4).

### F2.1 — Manual nav links (authoring + graph integration)

**What it is.** An authored connection between two points on (or near) the surface that the search
may traverse: start/end positions, up vectors, per-direction traversal permission
(forward/backward/bidirectional), per-direction cost, an area/usage tag, a user type tag, and an
enabled flag. Links participate in A* as extra edges and in reachability as extra connectivity.

**Why we need it.** Nav links are named in the campaign's generation-1 scope ([NN-D2]: "floors,
ramps, stairs, nav links"). Recast cannot be retired while it is the only thing that can express a
ladder, a drop, or a jump-across.

**What WE build.** Links as ECS entities with a value record baked into the published field
(resolved to the plates their endpoints project onto), so the search graph sees them as portals of a
distinct kind. Link identity is a stable integer id scoped by the owning tile and its epoch, so a
link id from a stale field is **detectably stale by construction** rather than silently wrong.

**Acceptance criteria.** Layer 1: a path across a gap uses the link when its cost beats the detour
and not otherwise; a disabled link is invisible to search and to reachability; a one-directional
link is traversable in exactly one direction; a link whose endpoint no longer projects after a repair
is dropped from the rebuilt field with a counted diagnostic. Layer 3: gym station with a drop and a
ladder `[EDITOR-VERIFY]`.

**Provenance.** Off-mesh connections / nav links as published in the open-source `recastnavigation`
project (zlib) and standard in navigation-mesh literature. **Landed 2026-09-04 (session 9)** on `feature/ck-navigation`
under [NN-D71] (with F2a/F2b/F8a/F9a/F14a/F14b/F15a/F16a). **What shipped:** a reflected `FCk_GroundNav_LinkRecord`
(two world points, direction, two multipliers ≥ 1 on the span, authored clearance, tags, enable, projection mode + extents)
authored on the volume through `Request_Link` / `Request_ReleaseLink` (the link entity is the identity; ids monotone and
never reused), copied into `FCk_GroundNav_FieldParams::_Links` by every build, repair and derive; a derived field-level
`_ResolvedLinks` array (per-end surface, flat plate and status; an unbaked end HELD as `Unbuilt`, a groundless end a counted
status, never a warning) re-derived at the three composition points between the seam portals and the labels; a link change is
a zero-geometry-probe DERIVE (`Get_FieldWithLinks`) ordered after the cost derive and keeping its tag while a build runs or a
repair is armed; reachability unions every traversable link (undirected); the search sees a link as a crossing with a
`_LinkIndex` (one node, arrival at the entry, departure at the exit, the traverse charged on the incoming edge so the
Euclidean heuristic stays admissible at w = 1), the corridor pushes two degenerate funnel portals and the corner offset
leaves the endpoints where the author put them; `Get_IsLinkLive` (resolved AND enabled AND both endpoint tiles republished
past the record's epoch); draw mode 7 + the every-mode overlay, `ck.GroundNav.LinksAt`, the snapshot's `_Links`, the gym
station (a drop and a ladder). Evidence: `Saved/Logs/P5-5A-Final2.log` 309 / 309 (32 Layer-1 pins + 2 AS pins); the
per-step mapping is in PROGRESS's 5A exit block. Not shipped, by ruling: up vectors (5B), a neutral link capability on the
provider table ([P5-B1]), cross-volume links (P8 list), a link cheaper than its own span (Tier 3).

---

### F2.2 — Link runtime state and traversal handshake

**What it is.** Runtime mutation of links without a rebuild (enable/disable, cost update, add/remove
by id or by volume, batch update, per-end status reporting); path results carrying link metadata
(which waypoints are link-start/link-end, which link, expected entry direction); queries for "the
next link beyond distance d" and "all links on this path"; a traversal handshake — a consumer
announces it has started traversing a link with a correlator id and later reports completion; a
per-agent **traversal veto** so an agent may decline a link it cannot currently use.

**Why we need it.** Without the handshake, a link is just a teleport in the waypoint list and CkCrowd
has no way to run a ladder animation or gate a jump. Without runtime mutation, a closed door on a
link needs a rebuild.

**What WE build.** Link state as a mutable ECS fragment separate from the baked link geometry
(geometry is in the immutable field; enabled/cost live in a per-world overlay the search consults),
so state changes cost nothing.

**Handshake.** `FCk_Request_NavSurface_LinkTraversal` carries the **link id** plus a **correlator
id**, enqueued through the utils facade and **drained on the game thread like every Ck request**.
There is no any-thread entry: an off-thread caller marshals to the game thread first. Completion is
a paired `Request` carrying the same correlator, over the same transport, with the standard house
completion delegate (delegate always last, exactly-once, `Succeeded` for idempotent no-ops).

**Veto.** The per-agent link veto is **part of the per-query compiled filter** (F1.27), evaluated
when search expansion considers a link edge — the same point at which plate-edge filters run. It is
shaped as an **allow/deny plus cost-rewrite on link records**, not as a callback into agent code
from inside the search.

**Acceptance criteria.** Layer 1: disabling a link mid-path invalidates paths using it (F1.34) and
not others; the overlay is per world; a veto excludes the link for that agent only. Layer 2: an
agent traverses a link, the handshake fires exactly once, and cancelling mid-traversal completes
`Failed_Cancelled`.

**Provenance.** Off-mesh connection runtime as implemented in the open-source `recastnavigation`/
Detour project (zlib). **Landed 2026-09-04 (session 9)** on `feature/ck-navigation` under [NN-D73]
(+F7a/F8a/F6a/F8b/F14c). **What shipped:** runtime state through the 5A derive (no overlay): `Request_LinkBatch` (atomic
admission, one completion), `Request_ReleaseLink_ById`, `Request_ReleaseAllLinks`, `Get_LinkResolution`; per-waypoint link
metadata on the GroundNav path result (stable id, entry/exit, entry direction, distance) in a parallel array that leaves
`_Waypoints` byte-identical, with `Get_LinksOnPath` / `TryGet_NextLinkBeyond`; the neutral handshake on the traversing
entity (`UCk_Utils_NavSurface_LinkTraversal_UE`: Begin/Complete/Cancel with a correlator, exactly-once, the two signals,
EndPlay cancel; no provider-table capability — the table stays at 14); the crowd stamps spans at install, drives the
handshake from its waypoint cursor, cancels on every route drop, and while traversing applies the steered displacement
instead of the surface walk; a per-agent veto on the path request (denied ids, denied user-type tags with parent
matching, per-link multipliers ≥ 1) copied from the crowd's params onto both GroundNav dispatches; exact invalidation for
a link-only publish through a registry publish note describing the run of link-only publishes since the last geometry
publish. Evidence: `Saved/Logs/P5-5B-Final2.log` 503 / 501 and `P5-5B-Final2-Crowd.log` 125 / 121 (both baseline-only red).
Not shipped, by ruling: link metadata on the neutral `FCk_Nav_PathResult` (promotion), repath of detoured agents on a
re-enable (a consumer replans), a per-world overlay (the derive is the mechanism).

---

### F2.3 — CkPathNetwork migration off its Recast dependencies

**What it is.** Three distinct dependencies must move to the facade:
(a) `Resolve_OffPathLeg` (`CkPathNetwork_Processor.cpp:165-200`) uses Recast as a **connector-path
builder** for legs that leave the authored network; (b) `Get_DefaultRecastNavmesh` /
`Is_NavmeshSegmentDirectlyWalkable` / `Try_ResolveNavmeshSegment` (`:349-410`) use Recast as a
**safety oracle** — every compiled ribbon segment is proven walkable before install; (c) the filter
class threaded through the route compiler (`:461-797`) and ribbon containment via `ResolveQueryFilter`
(`:556-650`).

**Why we need it.** Explicit retirement gate: "PathNetwork fully migrated off its safety oracle"
(`MIGRATION_SEAM.md` §4).

**What WE build.** (a) becomes a CkGroundNav path query (F1.23/F1.24) through the facade. (b) becomes
`Try_SurfaceRaycast` (F1.16) with the cost cap, falling back to a bounded path query exactly as today.
(c) becomes neutral filter definitions (F1.27) threaded through `FRouteCostPolicy`.

**Acceptance criteria.** Layer 2: PathNetwork's existing autotests green through the facade on Recast
(no behaviour change), then green on CkGroundNav; segment-safety verdicts agree with the Recast
baseline on every compiled ribbon in the test maps (shadow-mode comparison).

**Provenance.** No new algorithmic claim (reuses F1.16, F1.23, F1.27).

---

### F2.4 — Editor authoring snap

**What it is.** Editor-time projection of authored nodes onto the walkable surface
(`CkPathNetworkEditor/CkPathNetwork_EditorUtils.cpp:27-35,176-186,356-366`), working against a field
built in the editor world, with a sensible answer when the field is unbuilt (report, do not snap to
nothing).

**Why we need it.** Explicit retirement gate ("editor authoring snap migrated").

**What WE build.** `Try_ProjectPoint` (F1.13) through the facade against the editor world's provider,
plus an editor-world build path so the field exists at authoring time.

**Acceptance criteria.** `[EDITOR-VERIFY]`: authoring a PathNetwork node snaps to the floor
identically to the current behaviour on a reference map; snapping in an unbuilt region reports a
clear status rather than silently leaving the node in the air.

**Provenance.** No new algorithmic claim. **Landed HALF (2026-09-04, [NN-D72]/[NN-D75]):** the snap sites go through `UCk_Utils_NavSurface_UE::Try_ProjectPoint` (landed in P0, `CkPathNetwork_EditorUtils.cpp:35`); an unbuilt region is reported as `Unbuilt`, distinct from `NoSurface`, on the conformance and failure records and in the Details panel, with the pure mapping pinned (`Ck.PathNetworkEditor.Conformance.*` ×4, `d07088f34`). **Historical P5 rationale (superseded as current status):** the record then reported no Jolt static world in an `EWorldType::Editor` world and deferred a CkJolt world-lifetime decision as [P5-B2]. **Current evidence boundary:** the campaign does not yet contain implementation-and-verification evidence for an editor-world GroundNav field or the maintainer's `[EDITOR-VERIFY]` leg. It therefore does not establish editor snap as GroundNav-complete; the historical statement does not establish that no editor Jolt world exists today. **This Recast dependency is scoped to the editor snap alone** — the runtime `CkPathNetwork` processor (F2.3, [P5-B1]) is fully off Recast since session 12 (zero Recast symbols in the module; `NavigationSystem` dropped from `CkPathNetwork.Build.cs`). `[EDITOR-VERIFY]` §4.3 owed to the maintainer.

---

### F2.5 — EQS migration

**What it is.** Both EQS-facing surfaces move to the facade: the inline projection post-pass
(`CkEqs/Query/CkEqs_Algorithm.cpp:266-280`, deliberately duplicated today with a documented "keep the
two in sync" note) and any generator/test that wants navigable/reachable/path-length/raycast/project
semantics. [NN-D8g] rules: migrate both sites, **re-measure**, and the sync note dies with the
duplication unless measurement vetoes.

**Why we need it.** It is one of two remaining projection duplications, and EQS is a primary consumer
of point generation (F1.21) and reachability (F1.19).

**What WE build.** Facade calls, with the batch variants (F1.13/F1.14/F1.20) doing the amortization
the inline duplication was written to get.

**Acceptance criteria.** Layer 1: batch-through-facade cost measured against the current inline path
on a representative query (the number decides whether the duplication is justified — recorded in
VALIDATION.md, not assumed). Layer 2: EQS autotests green.

**Provenance.** No new algorithmic claim. **Landed before P5** (`094f9984b`, P0/P2; verified 2026-09-04 [NN-D75]): `CkEqs_Algorithm.cpp:282` projects through the neutral facade, the "keep the two in sync" note exists nowhere in `Source`, and the generators/tests half names an empty set (`ECk_Eqs_TestType` refuses navigation semantics by design). **Re-measure verdict:** nothing is duplicated, so there is nothing to measure; the batch facade (F1.14) is NOT built on either side of the seam and is recorded unbuilt — GroundNav's own batch amortizes nothing but the call. Two stale comments (`CkEqs_ProjectSettings.h:32`, `CkAutoTest_Eqs_NavProjection.as:16-17`) corrected in P5.

---

### F2.6 — CkQueue migration and revision consolidation

**What it is.** `CkQueue_Formation_Processor.cpp:159-209` (project + raycast for slot placement) moves
to the facade, and `CkQueue/Navigation/CkQueue_NavigationRevisionSubsystem.cpp:30-64` is **deleted**
in favour of the consolidated revision API ([NN-D8a], F1.33).

**Why we need it.** Duplicate observers are a split-brain mirror with no reconciler — precisely the
shape the Resilience Tenets forbid.

**What WE build.** Facade calls plus deletion of the duplicate subsystem.

**Acceptance criteria.** Layer 2: queue formation and navigation-change autotests green. Layer 1
(contract): exactly one nav-revision subsystem in the codebase.

**Provenance.** No new algorithmic claim. **Landed before P5** (`094f9984b`; verified 2026-09-04 [NN-D75]): `CkQueue_Formation_Processor.cpp` places slots through the facade, `CkQueue_NavigationRevisionSubsystem` is deleted (one implementation of the revision observer remains, `rg NavigationRevisionSubsystem Source` = 0 in CkQueue), the Recast pattern greps zero under `Source/CkQueue`. Evidence: `Saved/Logs/P5-5F-Queue.log` delta-zero by name against the four baseline reds.

---

### F2.7 — Named markup regions with live cost update

**What it is.** A markup region registered by stable id whose **cost multiplier and usage tags can be
updated without any rebuild**, distinct from markup that changes walkability (which needs repair,
F1.35). Overlap semantics unchanged (usage OR, cost max).

**Why we need it.** Stationary-crowd cost markup (`UCk_NavArea_CrowdAgent`) updates constantly as
agents idle and move; paying a repair for each is unaffordable. This is the feature that makes
dynamic cost cheap enough to use aggressively.

**What WE build.** A per-world markup overlay consulted by the cost model (F1.26) at query time,
keyed by region id, layered over the baked plate policy. The baked policy answers walkability; the
overlay answers cost. Two tiers, one documented rule for which is which.

**Acceptance criteria.** Layer 1: updating a named region's multiplier changes subsequent path costs
with **zero** geometry probes and no epoch bump; walkability-changing markup still bumps the epoch;
overlap max-wins holds across baked and overlay sources.

**Provenance.** Two-tier baked/live area modifiers — the modifier-volume model in the open-source
`recastnavigation` project (zlib), split along a baked/live boundary of our own choosing. **Landed by 4A/4C** (`05a627f39`, `2ece50c53`; verified 2026-09-04 [NN-D75]): a markup record is keyed on its entity with a stable, never-reused per-volume id; a second request naming the same entity is an in-place update; a COST change is a derive (`Get_FieldWithMarkupCost`: zero probes, no re-bake) and a WALKABILITY change a local repair; overlap is tags OR / multiplier MAX without a plate split. **Amendments to this cell's acceptance:** the live cost update is a RE-TAG (the multiplier is the tag's registered policy — a per-record override is not authorable and Recast could not express it); "no epoch bump" is superseded as F2.2's was — the derive moves only the changed tiles' epochs, which is the invalidation channel; "query-time overlay" describes the shape [NN-D66] rejected in favour of the bake-time restamp. Not shipped: the crowd's own Recast-only painter (`UCk_NavAreaMarkup_UE`, four sites) — P8's B2 promotion condition ([NN-D70] F6).

---

### F2.8 — Per-agent-profile field variants

**What it is.** Support for **more than one baked profile layer** where slope/step/height genuinely
differ enough to change walkability (radius never justifies one — F1.5). One geometry collection
feeds all profiles; each profile gets its own filtered field.

**Why we need it.** A crouching or vaulting agent class, or a small critter that fits where a human
does not, changes the *walkable set*, not just the clearance threshold.

**What WE build.** The bake pipeline parameterized by profile from F1.3 onward (collection F1.1 and
rasterization F1.2 are shared), with a profile index in the field identity and in the content hash.
**Adding a profile is a CTO decision, not an executor's** ([NN-D7] fences) — the feature is the
mechanism, not a licence to multiply profiles.

**Acceptance criteria.** Layer 1: two profiles over one collection produce the expected differing
walkable sets with exactly one geometry collection (assert on the probe counter); queries select the
correct profile's field; profile identity participates in the content hash.

**Provenance.** Per-agent-type navmesh generation — standard practice, as in the open-source
`recastnavigation` project's agent parameters (zlib). **Landed 6a (2026-09-04, [NN-D75], `d07088f34`):** the sliced field build takes N params differing only in `_Profile` (reflection-compared, ensure on the first differing field), fetches each tile's geometry ONCE and bakes a tile per profile (the resumable unit stays the tile), composes each field on its own, reports open bodies once, counts `_GeometryFetches` on the build state — the honest form of "exactly one geometry collection" (the probe counter sums per profile by construction) — and terminates at Begin on an inadmissible profile; `ProfileVariants.*` ×6 (one fetch per tile, differing walkable sets across a 60 uu riser at step 40 vs 80, sliced == one-shot per field, mismatched params refused, N = 1 identical to the single entry point, per-profile fingerprints). Item 4 of the frozen fingerprint already hashes the profile — no new item; FINDING: `_StandingExtents` is read by the walkability filters but not hashed (follow-up). **Landed 6b (2026-09-04, [NN-D76]/[NN-D77]/[NN-D78], `9fff07269`):** the volume authors `_ProfileVariants` (tag + profile) beside its default; the build bakes one field per profile from a single geometry fetch and publishes the default and the variant map in ONE registry call under one lock; `TryGet_Field(world, location, profileTag)` (empty tag = default, unknown tag = no field, never a fallback); `_ProfileTag` on the five neutral queries and the GroundNav path request, threaded through the adapter and stamped on the path's current corridor so the invalidator resolves the field the plan was made on; the cost and link derives run over every field and publish when any moved (newest epoch, union bounds/link ids); a repair on a variant-holding volume rebuilds; variant fields count toward the world revision and dropped/replaced variant sums retire so it never falls; duplicate/empty tags and invalid variant profiles refused at admission. Pins: `ProfileVariants.*` ×12 (admission, registry selection, unknown tag, tag-carrying query, variant-planned path invalidated by a variant-only change, revision never falls, plus 6a's six) and the PIE pin `ProfileVariant_QuerySelectsTheProfilesField` (two projections select their own field; a repair on the variant volume completes Succeeded with a newer epoch and both profiles built). Filed: multi-profile local repair (must land before a second profile is authored — CTO, [NN-D7]); debug draw of variant fields; no headless pin for the variant-only derive publish. Adding a profile remains a CTO decision.

---

### F2.9 — CkGroundNav debug draw module

**What it is.** A runtime-tier draw module rendering the field: walkable cells, plates (by id, by
area tag, by clearance), portals, boundary segments, layer separation, reachability components, links,
and paths (corridor vs string-pulled). Sourced from **value-only snapshots**, with failure rendered
as a status rather than an empty scene.

**Why we need it.** [NN-D8f] rules `CkNavmeshDebugDraw` **retired, not rewritten** — CkGroundNav
ships its own draw module designed against value snapshots. Explicit retirement gate
("`CkNavmeshDebugDraw` deleted, its replacement is the CkGroundNav draw module").

**What WE build.** A `CkGroundNav_DebugSnapshot.h` in the runtime feature module (tier-1 of the
three-tier debugger data flow, `R4 §6`): boxes/segments/counts/ids/epochs/status only, never handles,
`UObject`s, or field shares; an `EDebugSnapshotStatus` in the proven vocabulary
(`MissingCook/StaleCook/Building/Current/Failed/RuntimeOnly`); a source enum distinguishing
LivePie/RetainedSnapshot/EditorPreview; a layer bitmask with deterministic caps; cache identity
checked before enumeration; whole-snapshot atomic replace. Drawing uses `CkPmg`'s retained
`Create_DebugLineSet` + `Append_Debug*_World` tier for plate outlines, portals, and paths — the tier
`R4 §3` identifies as right for exactly this — chunking indefinitely growing streams.

**Acceptance criteria.** Layer 1: snapshot contains no handle/`UObject`/shared-field member
(reflection or compile-time assertion); a failed build renders a status, never an empty scene; layer
caps are deterministic. `[EDITOR-VERIFY]`: each draw mode renders correctly in PIE **and** in
packaged Development/Test (the promotion gate names both).

**Provenance.** No algorithmic claim; our own three-tier debugger data-flow pattern (`R4 §6`). **Boundary ([NN-D80], 2026-09-04):** the tier-1 snapshot landed in P1 ([NN-D28]) in Runtime CkGroundNav with the status vocabulary `NeverBuilt / BackendUnavailable / NoGeometryInRegion / Failed / Current` (ratified; VoxelNav's six are not imported); draw modes 0–7 plus the query commands `ReachAt`/`PathAt` ("draw modes and query commands", F3); no new module or `.uplugin` row (F2). Owed by P6: the compile-time value-only assertion, the snapshot cache with whole-snapshot replace, per-status and torn-down-producer pins (6A); the CkPmg retained-tier draw and draw pins (6B); the draw stays unguarded, Shipping stripping is P8 (F8). **Landed (P6 batches 1+2, `127857f7f` + `58e4f0092`):** compile-time value-only trait + asserts (`CkGroundNav_DebugSnapshotTraits.h`), `FCk_GroundNav_DebugSnapshotCache` keyed on world name + entity number/version + newest tile epoch + surface revision with whole-value replace, status derived behind `ICk_GroundNav_GeometryBackend&` (`Make_DebugSnapshotFromBackend`), retained CkPmg draw rebuilt only on key/selection change (Field + Query groups, `ck.GroundNav.Debug.RetainedDraw`), per-status + draw + torn-down-producer pins. Packaged Development/Test verification (§4.4) is the maintainer's.

---

### F2.10 — CkCrowdDebugger adapter parity

**What it is.** `CkCrowdDebugger_DataCollector.cpp:169-482`, `Types.h:133` (GetBounds viewport fit),
and the NavmeshStatusPanel ("UNavigationSystemV1 OK") move to neutral surface bounds + provider
health, and the crowd debugger's nav-adjacent views keep working on either provider.

**Why we need it.** Explicit promotion gate: "Both debugger surfaces (runtime in-world draw +
CkCrowdDebugger adapter) at parity per VALIDATION.md's checklist, in PIE **and** packaged
Development/Test."

**What WE build.** Adapter changes only — the collector reads `Get_SurfaceBounds` /
`Get_ProviderHealth` / `Get_SurfaceRevision` from the facade. Snapshot boundary rules unchanged
(copies values; never `UWorld`/actor/handle/field/producer, `R4 §6`).

**Acceptance criteria.** `Ck.CrowdDebugger.Viewport3d.*` family green — the P6 entry set is **19/19 with zero
failures** (`P6-CrowdDebugger-Baseline.log`); the formerly "known inherited" `Nav.Filter.Customer` failure is
moot (the tag exists, its one consumer is green — [NN-D80]), so the family must stay exactly 19/19. The status
panel renders a provider-neutral header (provider · health · revision) and demotes the Recast rows to a
provider-specific section (F4). `[EDITOR-VERIFY]`: viewport fit and status panel correct on both
providers. **Landed (P6, `c2a3d58` + `81ac31e`):** neutral header + demoted Recast rows (`NavmeshStatus.*` ×3), the `GroundNavField` scene role fed from a shared immutable snapshot copy through the cache (`Viewport3d.GroundNavField*` ×2), the shadow-parity panel (`ShadowParity.*` ×2); family 26/26.

**Provenance.** No algorithmic claim.

---

### F2.11 — PerfLab seeding migration

**What it is.** `CkPerfLab_WorldSurvey_Builder.cpp:57-250` seeds spawn positions from the navmesh;
it moves to F1.21's point generators through the facade.

**Why we need it.** It is a listed `[ADAPTER]` site and PerfLab is how crowd performance numbers get
produced — including the ones this campaign's promotion gates depend on.

**What WE build.** Facade calls with a caller-supplied RNG seed so PerfLab runs stay reproducible.

**Acceptance criteria.** Layer 1: identical lattice output across runs (the survey is a fixed lattice by
contract, never random — [NN-D80] F6). `[EDITOR-VERIFY]`: a PerfLab survey produces a comparable agent
distribution on both providers.

**Provenance.** No new algorithmic claim. **DONE-ALREADY ([NN-D80] F6):** the survey projects through
`Try_ProjectPoint` and reads `Get_ProviderHealth` since 0C batch B; bounds stay `GetWorldBounds` under [NN-D19].
Routing through F1.21's generators needs a facade point-generation capability that does not exist — [P6-B1].

---

### F2.12 — Gym station for ground navigation

**What it is.** A gym station exercising bake, repair, markup, links, projection, containment,
raycast, boundary segments, search, and the debug draw modes, with steps as CkStateMachine graphs.

**Why we need it.** `R4 §7` Layer 3; it is how a human forms a verdict on the behaviours no automated
test can judge (does the path *look* right, do agents hug walls, does a door opening feel
responsive).

**What WE build.** A station in `Script/CkGroundNav/` registered in `CkTests_GymRegistry.as`, one
`UCk_Gym_StepState` per step with `UCk_Gym_Dwell` gating, using the shared
`CkGym_ControlPanel.as` (rows rebuilt per frame), reserved keys Tab and H, and the standard
`Ck_Gym_Restart/_Next/_Prev/_GoTo/_List` execs. One class per `.as` file; no test-class renames.

**Acceptance criteria.** `[EDITOR-VERIFY]`: every station placed, labelled in the panel and flyable-to
(`Ck_Gym_GoTo` travels between gyms, not stations — [NN-D80] F5); the station runs on both providers via the
provider setting; control-panel rows reflect live field status (readback where a getter exists, mirrored
only where none does and said so). The tuning range IS this gym; no step-state retrofit. **Landed (P6 batch 1, CkTests `70e1b223`):** seven rows on keys 1–7 (live provider setter, paint/clear, repair, walker cycle, path draw, status), readback audit, five stations (ramp, moved obstacle, painted markup, multi-tile crossing, no-route pocket) on a second range volume. §4.1 walkthrough owed.

**Provenance.** No algorithmic claim.

---

### F2.13 — Runtime gameplay-debugger category

**What it is.** A debugger category showing, per selected agent: active provider, agent profile,
current path status, waypoint index, last query time, current area tags, and whether the agent's
path is flagged for repath.

**Why we need it.** Field debugging of a crowd on a running build; the existing crowd debugger covers
steering/avoidance but not provider-level state.

**What WE build.** A category in the CkGameplayDebugger runtime tier reading value-only diagnostics
fragments (F1.40's shadow diagnostics reuse the same shape).

**Acceptance criteria.** `[EDITOR-VERIFY]`: category toggles and renders in PIE and in a packaged
Development build.

**Provenance.** No algorithmic claim. **Landed (P6, [NN-D80] F7/[NN-D81]/[NN-D84]):** a value-only `FFragment_GroundNavPath_Diagnostics` stamped per tick (provider, profile tag, status, waypoint count, corridor link ids/epoch, repath flag, `FCk_Time` of the last plan, `_HasBeenStamped`; gated by `ck.GroundNav.PathDiagnostics`), `Get_Diagnostics` + `DoCast/DoCastChecked`; the surface is an AS `UCk_GameplayDebugger_DebugSubmenu_UE` subclass (`UCk_GroundNav_DebugSubmenu`, key G) with an AS filter and profile (`CkTests_GroundNav_DebugProfile_Assets.as`), pinned by `PathDiagnostics.*` ×4 and the AS autotest; the maintainer selects the profile in Project Settings. No new category or module.

---

### F2.14 — Field serialization (value-only)

**What it is.** The published field serializes to and from bytes at three granularities: whole field,
a spatial subset (dropping boundary-crossing portals and links cleanly), and per tile. A stream
version gates compatibility; a mismatched version is a clean rejection with a status, never a
partial load.

**Why we need it.** Prerequisite for cook (F2.15) and streaming (F2.16). Today nav data is baked
level content and CkNavigation persists nothing (R3 §C.11) — that is exactly why removing Recast
requires us to supply the persistence it was giving us implicitly.

**What WE build.** Value-only serialization of the field's flat arrays and stable integer ids — the
representation was chosen partly for this ([NN-D7] requirement 12: "stable integer identity (no
pointer identity), chunkable, serializable as values"). Deliberately **not** persisted: the
process-relative pending clock (already contractually non-persistable, R3 §B) and any runtime overlay
state (markup overlays are re-applied after load, not serialized).

**Acceptance criteria.** Layer 1: round-trip a baked field and byte-compare; round-trip a spatial
subset and assert boundary portals/links are dropped rather than dangling; a version-mismatched blob
is rejected with a status and no partial state.

**Provenance.** No algorithmic claim.


**Landed (P7 batch 1, `05a81ac34`, [NN-D86] R1–R5 / [NN-D89]):** hand-rolled little-endian blob (`Field/CkGroundNav_FieldSerialize.*`): header {magic, format version, content byte, UTC cook seconds, lattice, fingerprint slot, sorted tag table}, `_Params` + tiles written, every derived array RE-DERIVED on load through the pure derives; `TIsPersistableValue` rejects `FName`/`FGameplayTag` (name table); whole-field / per-tile / spatial-subset forms on the fixed lattice; load statuses `WrongMagic/WrongVersion/Truncated/UnknownTag/LatticeMismatch/Corrupt` (finite + unit-quaternion guards, counts bounded per element) with the caller's field untouched on any failure; `Serialization.*` ×10 incl. `ParamsRoundTripEveryMember` and `NothingProcessRelativeIsPersisted`. Not persisted: inactive `FCk_AnyShape` slots (documented).
---

### F2.15 — Cook / offline bake

**What it is.** Fields bake offline and ship in the package, so a runtime build does not pay a cold
bake at level load. Includes a cook-time build entry point and a needs-resave signal when source
geometry changed after the last bake.

**Why we need it.** Recast bakes at cook today; removing it without an offline bake makes every level
load pay for a full field.

**What WE build.** A cook-time build path over the same math core (F1.43) writing serialized tiles
(F2.14) into cooked assets, plus a content-hash comparison (F1.11) as the needs-resave signal.

**Acceptance criteria.** Layer 1: a cooked field loaded at runtime is byte-identical to a runtime
bake of the same world. `[EDITOR-VERIFY]`: a packaged Development build loads cooked fields and the
crowd paths correctly with runtime baking disabled.

**Provenance.** No algorithmic claim.


**Scaffolded (P7 batch 1, `05a81ac34`, [NN-D86] R6/R7, [NN-D87]/[NN-D88]):** `CkGroundNavEditor` (UncookedOnly, the campaign's first added module), `Cook/CkGroundNav_CookedTile.*` + `CkGroundNav_CookedFieldIndex.*` (data assets: blob, format version, coord, lattice key, INPUT fingerprint, soft tile refs) with path free functions mirroring `ck::jolt`'s (PIE-prefix normalised), `_CookKey` on the volume params (None = runtime-only; duplicates refused at Setup and build request), the fingerprint's items 7–9 (`_MergeTunables`, `_MaxClearanceUu`, profile variants) + `Get_InputFingerprint` and the stored bake identity refreshed by every publish, `Get_IsBuildCurrent` (revision + input fingerprint). **Landed (P7 batch 2, `8b98001f8`, [NN-D90]/[NN-D91]):** `ECk_GroundNav_CookStatus {RuntimeOnly, MissingCook, StaleCook, Cooked}` on the built-field fragment (Setup answers it; a runtime build after a cooked publish demotes to `StaleCook`; repair/derives carry it); `Try_LoadCookedField` (pure: index identity, per-tile version/lattice/coord/fingerprint guards, one compose, no open bodies) + `Find_CookedFieldIndex`; the state-selected fallback in Setup (cooked → restamp every tile and publish without a build; else arm the runtime bake); the former one-profile cook-key restriction is superseded by S14: profile-aware identity is `{normalised source-level package, CookKey, ProfileTag}`, and the loader resolves every profile into temporary fields before atomic all-profile publication; `Get_CookStatus`; 13 `CookedAssets.*` pins + two PIE pins. **S14 current working-tree scope:** authored source-level identity, ordinary streaming-level package loading, profile-aware convention paths, atomic bundle publication, and descriptor-complete World Partition cook discovery are implemented. The pure 7F merger supports aligned transformed instances. Durable per-source World Partition manifests and automatic runtime cell/data-layer lifecycle binding remain open. Packaged Development/Test/Shipping evidence remains build-machine-only.
---

### F2.16 — Streaming: chunk load/unload and merge

**What it is.** Tiles load and unload with level streaming: a loaded chunk merges into the live field
(portals stitched to already-loaded neighbours, links re-resolved), an unloaded chunk is removed
cleanly (paths crossing it invalidated via F1.34, not left dangling), and a streamed-out-but-not-
unloaded chunk can be disabled and re-enabled without a rebuild.

**Why we need it.** Our projects stream. Without it, retirement means levels either bake everything
resident or lose navigation on streamed geometry.

**What WE build.** Chunk merge as *portal re-derivation at the seam only* — the cross-tile portal
extraction from F1.7 run for the newly adjacent boundary, which is a bounded, already-tested
operation. Chunk identity follows the CkVoxelNav `FChunkId` precedent (`R4 §1`) and partitioning is
decided at composition, not in a processor.

**Acceptance criteria.** Layer 1: load A, load adjacent B, assert cross-tile portals identical to a
single bake of A∪B; unload B and assert A's field is exactly A-alone's field; disable/re-enable
round-trips exactly. Layer 2: streaming a level in and out during an active crowd episode produces
path failures with correct reasons, not crashes or ensures.

**Provenance.** Tiled navigation data with runtime tile add/remove — the tile-cache model published in
the open-source `recastnavigation` project (zlib).


**IMPLEMENTED LOCALLY (S14 Gate63b/64d/64f/82):** positive authored volume identity, fixed-lattice source ownership, deterministic seam composition, unload/disable/re-enable, atomic all-profile publication, and active/in-flight path invalidation are covered by pure, registry, and production-volume tests. Automatic World Partition cell/data-layer lifecycle binding awaits the durable per-source manifest described under F2.18.
---

### F2.17 — Build invokers (streaming-driven build scope)

**What it is.** Runtime build scope driven by **invokers**: point invokers with an inner generation
radius and an outer removal radius (hysteresis prevents thrash at the boundary), plus invoker
volumes. The delta between the required set and the built set drives build and purge.

**Why we need it.** Open worlds cannot bake everything; Recast has invokers and content relies on
them. Retiring Recast without this regresses large-world memory and build cost.

**What WE build.** An ECS-native invoker: a fragment on any entity, aggregated per world into a
required-tile set each tick, differenced against the built set, feeding the build scheduler (F1.10)
in a deterministic order. Hysteresis is inner/outer radius, exactly as the published pattern.

**Acceptance criteria.** Layer 1: an invoker moving back and forth across the inner/outer boundary
does not thrash (bounded build count over N oscillations); the required set is a pure function of
invoker positions + radii; purge removes exactly the tiles outside the outer radius.

**Provenance.** Inner/outer-radius invoker hysteresis for on-demand navigation generation — the
nav-invoker pattern published in Unreal's own navigation system and in the open-source
`recastnavigation` tile-cache literature.


**IMPLEMENTED LOCALLY (S14 Gate65c/66b/82):** ECS point and box invokers produce deterministic generation/retention sets with hysteresis, targeted all-profile builds, retained zero-probe re-enable, purge, supersession, and per-world teardown.
---

### F2.18 — World Partition / data-layer awareness

**What it is.** Geometry collection respects data-layer filtering (a build knows which data layers it
is building for; geometry from excluded layers is not collected), and cooked chunks are World-
Partition-aware actors carrying their bounds and integer chunk coordinates.

**Why we need it.** Without it, a World-Partition project either bakes the union of all data-layer
variants (wrong) or cannot bake at all.

**What WE build.** A data-layer selector on the build request feeding F1.1's collection filter, and a
cooked chunk asset carrying bounds, chunk coordinates, and its content hash.

**Acceptance criteria.** Layer 1: collection with a layer filter returns exactly the expected
geometry subset; the layer selector participates in the content hash. `[EDITOR-VERIFY]`: a
data-layer-varying level bakes distinct fields.

**Provenance.** No algorithmic claim.


**LOCAL SOURCE SLICE IMPLEMENTED (S14 Gate67e/68/82):** the canonical selector filters production Jolt collection, participates in fingerprints and cooked lookup identity, tile/index assets carry volume/coordinate/bounds/profile/selector/hash metadata, and cook discovery visits unloaded World Partition actor descriptors safely. `[EDITOR-VERIFY]` remains open. Runtime cell/data-layer streaming additionally needs a durable all-profile source-partition manifest; the current full-volume indexes cannot be bound without overlapping the logical owner's reserved coordinates.
---

### F2.19 — Field merge (offset, rotation, tag remap)

**What it is.** Merging a separately baked field into a live one at an offset and an optional
90°-multiple rotation, remapping its area tags into the host's tag space.

**Why we need it.** Instanced/prefab level content — a room baked once and placed many times — is the
only affordable way to handle heavily repeated interiors, and streaming merge (F2.16) is a
degenerate case of the same operation.

**What WE build.** A pure value transform over the serialized field (positions offset/rotated, ids
rebased, tags remapped through a table) followed by seam portal re-derivation (F1.7). Restricted to
axis-aligned 90° rotations in generation 1 — arbitrary rotation would break the axis-aligned plate
invariant and is a Tier-3 question.

**Acceptance criteria.** Layer 1: merging a field at offset O equals baking the same geometry at
offset O (byte-compare after seam re-derivation); a tag remap is total (an unmapped tag is a
rejection, not a silent drop).

**Provenance.** No algorithmic claim.


**IMPLEMENTED LOCALLY (S14 Gate81/82):** the pure merge supports translation and 0/90/180/270-degree rotation into a declared host lattice, checked positive ID rebasing, total tag remap, authored world transforms, tile-local rotations, and complete seam/link/reachability re-derivation. Invalid descriptors, malformed fields, overlap, and out-of-bounds source footprints reject atomically.
---

### F2.20 — Adapter and dependency removal

**What it is.** Deletion of every `[ADAPTER]` site, removal of `NavigationSystem` and `AIModule`
from `CkNavigation.Build.cs:17-18`, `CkCrowd.Build.cs:21`, and any other affected `.Build.cs`;
deletion of `CkNavmeshDebugDraw` and of the five framework `UNavArea`/`UNavigationQueryFilter`
subclasses; deletion of `Request_SetActorNavigationRegistered` ([NN-D8c]).

**Why we need it.** It is the definition of retirement.

**What WE build.** Deletions only. **No back-compat shims** (CkFoundation doctrine).

**Acceptance criteria.** Grep assertion: zero occurrences of `UNavigationSystemV1`, `ARecastNavMesh`,
`UNavigationQueryFilter`, `UNavArea`, `INavRelevantInterface`, `FPathFindingQuery` across
CkFoundation/CkTests/CkGameplayDebugger (baseline counts from R3: 60/48/25/45/1/1). Full-suite
delta-zero gate **on the final artifact** (no stale-green: the gate re-runs after the last deletion).

**Provenance.** No algorithmic claim.

---

### F2.21 — Performance budgets met and recorded

**What it is.** Query-per-frame budget adherence, bake cost, and repair cost all within budgets
**measured and recorded in VALIDATION.md** — numbers, not estimates.

**Why we need it.** Promotion gate 4 (`MIGRATION_SEAM.md` §4). Non-negotiable #7: no performance
claim without a benchmark.

**What WE build.** Measurement harnesses (PerfLab surveys, Layer-1 probe/expansion counters) and the
VALIDATION.md tables they fill.

**Acceptance criteria.** Every budget in VALIDATION.md carries a measured baseline (Recast, same
fixture) and a measured CkGroundNav number, both dated, both reproducible.

**Provenance.** No algorithmic claim.

---

### F2.22 — Dynamic latency gates met

**What it is.** Markup-live latency (F1.32) and moved-obstacle repair latency (F1.35) ≤ the
Recast-measured baseline on the same fixtures.

**Why we need it.** Promotion gate 3.

**What WE build.** The measurement, on the existing crowd fixtures.

**Acceptance criteria.** Recorded pairs of numbers per fixture in VALIDATION.md.

**Provenance.** No algorithmic claim.

---

### F2.23 — Documentation and doctrine updates

**What it is.** `CkGroundNav/Claude.md`, an updated `CkNavigation/Claude.md` (flagged stale by
`Source/CLAUDE.md` today), the module tier table entry, and removal of stale references
(`CkAStar/Claude.md` describes only the CkGrid use case, `R4 §2`; `CkNavigation/Plan/` is history,
not live).

**Why we need it.** A retired dependency that still appears in the docs is a trap for the next
engineer.

**What WE build.** Docs.

**Acceptance criteria.** Every claim in the new module doc verified against code on the day it is
written and dated.

**Provenance.** No algorithmic claim.

---

# TIER 3 — post-retirement backlog

> Real capabilities, planned, none scheduled by this campaign. Each is listed so a later planner has
> the spec rather than rediscovering it.

### F3.1 — Automatic nav-link generation (linear)
Automatically generate traversal links across gaps, drops, and ledges between walkable areas,
validated against agent trajectory feasibility. Deferred: manual links ([F2.1](#f21)) reach
replacement parity, so auto-generation is a post-retirement enhancement.
*Provenance:* requires a design pass from cited public literature (e.g. Lozano-Pérez 1983
configuration-space obstacles; ballistic trajectory feasibility) before scheduling.

### F3.2 — Automatic nav-link generation (parabolic / jump arcs)
Automatically generate jump links whose traversal is a ballistic arc, validated against agent
trajectory feasibility. Requires F3.3. Deferred for the same reason as F3.1.
*Provenance:* requires a design pass from cited public literature (e.g. Lozano-Pérez 1983
configuration-space obstacles; ballistic trajectory feasibility) before scheduling.

### F3.3 — Parabola cast
Query capability: "does this ballistic arc clear the world, and where does it land, with
per-segment area/surface results" — feeds F3.2 and curve validation.
*Provenance:* requires a design pass from cited public literature (e.g. Lozano-Pérez 1983
configuration-space obstacles; ballistic trajectory feasibility) before scheduling.

### F3.4 — Convolved free-space geometry
Per-column Minkowski convolution of free/occluded space with the agent capsule, retained until the
neighbourhood needed for link generation is built. Only justified once F3.1/F3.2 exist.
*Provenance:* configuration-space obstacles via Minkowski convolution (Lozano-Pérez 1983).

### F3.5 — Sub-cell LOD / resolution reduction
Collapse N×N blocks to a coarser cell where the block forms a connected navigable surface, reducing
memory in large open areas. Interacts with F1.5 (clearance must be recomputed at the coarse tier) and
with F1.6 (which already collapses; measure before building this). *Provenance:* no algorithmic
claim.

### F3.6 — Boundary smoothing of plate outlines
An additive pass replacing staircase outlines along diagonal walls with simplified polylines.
`REPRESENTATION.md` lists it explicitly as the accepted-cost mitigation held in reserve. *Provenance:*
contour tracing with Douglas-Peucker simplification (Douglas & Peucker 1973); as in the open-source
`recastnavigation` project's contour stage (zlib). **Fence:** this is outline smoothing only — it must
not become a coincident polygon mesh, which would reopen the rejected candidate C.

### F3.7 — Non-circular agent footprints
Rectangular/oriented footprints: a per-cell valid-orientation mask replacing the scalar clearance
test, with a direction-mask erosion instead of a radial one. Changes the query contract (orientation
becomes an input) and is therefore a product-sized change, not an increment. *Provenance:*
configuration-space obstacles for a non-circular robot (Lozano-Pérez 1983).

### F3.8 — Surface types from physical materials
Per-cell surface-type id derived from the physical material, carrying friction and a cost multiplier,
remappable on field merge (F2.19). Feeds F3.9–F3.11. *Provenance:* no algorithmic claim.

### F3.9 — Water navigation modes
Littoral/shallow/deep as reserved usage-type bits with per-profile depth thresholds, cost multipliers,
and a deep-water mode (unnavigable / walk-the-floor / swim-the-surface); water-specific queries as
thin specializations of the generic area-tag queries (F1.20/F3.12). **See also the out-of-scope note
below — generation 1 has no water mode at all.**

### F3.10 — Points of interest
Registered named points queryable by radius or by path distance, and optionally returned "en route"
by the point generators (F1.21). Overlaps substantially with `CkPoi`; the first design question is
whether this belongs in CkGroundNav at all or as a CkPoi query using F1.19's flood fill.
*Provenance:* no algorithmic claim.

### F3.11 — Area-tag region queries
Edges of regions matching an area-tag spec (match modes none/any/all) and the closest position
carrying a given tag — the generalization F3.9's water queries would specialize. *Provenance:* no
algorithmic claim.

### F3.12 — Kinematic / turn-constrained search
Hybrid-A* over (cell, heading index, reversing flag) with a precomputed distance-to-goal field as the
heuristic (true remaining distance plus remaining turns, min over forward/reverse, with a
facing-alignment term for reverse manoeuvres). For vehicles and any agent that cannot turn in place.
*Provenance:* hybrid-A* / state-lattice planning (Dolgov, Thrun, Montemerlo & Diebel 2008, *Practical
Search Techniques in Path Planning for Autonomous Driving*); Dijkstra-computed heuristic fields.

### F3.13 — Speed-profiled curve output and trajectory sampling
Convert a waypoint list into a curvature-bounded curve (arc/spline chain), compute a max-energy speed
envelope per segment from gravity, friction, max lateral acceleration and max angular rate, run a
**backwards deceleration sweep** to make the profile feasible, and emit sampled trajectory states
(time, distance, speed) for animation and prediction. Incremental construction (build the smooth
section a short distance ahead, extend lazily, promote to fully built on demand).
*Provenance:* velocity-profile generation with backward-pass deceleration (standard trajectory-
generation literature, e.g. time-optimal path parameterization); clothoid/arc path smoothing.
**Note:** this is *path shaping*, not movement — see the out-of-scope section on CkCrowd.

### F3.14 — Clearance-thresholded reachability components
Component labels computed per clearance band, so `Get_IsReachable` can prove unreachability for a fat
agent where the coarse labels (F1.9) cannot. *Provenance:* connected components over a thresholded
graph.

### F3.15 — Memory: node dedup and hash-consing
Structural sharing of identical sub-blocks across tiles with a memory-thresholded dedup map, and
per-category memory attribution for the field's allocations. Only after F2.21 shows memory is a real
constraint. *Provenance:* hash consing (classic technique).

### F3.16 — Multi-variant editor build (data-layer build states)
An editor-time build-variant system producing distinct cooked chunks per ordered data-layer state,
with dirty-chunk source-control tracking. Extends F2.18. Only worth it for projects with heavy
data-layer variance.

### F3.17 — Latent Blueprint async query proxies
Latent BP nodes for projection, raycast, reachability and navigability so Blueprint can issue an
async query without a tick loop. Mechanical once F1.38 exists.

### F3.18 — Debug geometry export and query capture
OBJ export of the field, a dump-on-assertion hook, and capture of a search's cost/heuristic values
into a debug buffer for "why did it path there" visualization (compiled only in debug
configurations). Extends F2.9.

### F3.19 — Replicated debugger payload
A replicated byte payload so a client can render the server's view of the field/paths in a networked
session. Extends F2.13.

### F3.20 — Cross-field links
Links whose endpoints lie in two *different* fields (e.g. a static level field and a moving
platform's own field). Depends on out-of-scope moving volumes; listed for completeness.

---

# Explicitly out of scope (generation 1)

| Out of scope | Why | Where it goes |
|---|---|---|
| **Arbitrary-orientation surface navigation** (walls, ceilings, fully 3D walkable surfaces) | [NN-D2]: generation-1 scope is grounded navigation only. The chosen representation deliberately does not preclude it (per-cell quantized normals and a policy-keyed plate decomposition generalize), but a per-cell *valid-orientation* mask and an orientation-aware query contract are a product-sized change to every query signature. | A separate future campaign. [NN-D7] requirement 13 is the fence that keeps it possible. |
| **Replacing CkCrowd's movement or steering** | CkGroundNav supplies *where the ground is* and *which way to go*; CkCrowd owns local avoidance (its ORCA-family sampler), steering, block detection, and the single Transform write. Absorbing movement into the nav provider would merge two independently testable systems and destroy the A/B story — the shadow-mode gate compares *paths*, which is only meaningful while movement is shared. | Stays in CkCrowd. F1.17's boundary query and F1.15's containment are the entire interface. |
| **Water navigation modes** | Water requires surface-type classification (F3.8), depth semantics, and a per-profile deep-water mode; none of it is needed for parity with what Recast gives us today, and it would add a classification stage to the bake for zero current consumers. | Tier 3, F3.9 (with F3.8, F3.11). |
| **Moving / rotating navigation volumes** (a field attached to a moving platform, ship, or elevator car) | The published field is defined in world space with world-space stable ids; making it frame-relative changes the identity model, the query contract (every query gains a frame), and cross-field connectivity. This is the single largest structural change on the backlog. | Deferred; F3.20 (cross-field links) is its prerequisite and is itself Tier 3. |
| **Origin rebasing** | A world-origin shift invalidates every world-space position in the published field. Handling it means either rebasing serialized tiles (cheap but a full republish, invalidating every live path) or a frame-relative field (the moving-volume problem above). No current project rebases. | Deferred with moving volumes. If a project adopts rebasing, the interim answer is a full field republish plus mass path invalidation via F1.34 — correct, expensive, and adequate. |
| **Behaviour-tree / AI-task move nodes and movement adapters** (character/vehicle/3D movement drivers) | Ck agents move through CkCrowd and CkStateMachine, not through engine BT move tasks. Building a parallel movement stack would duplicate systems we already own. | Not planned. |
| **A navigation-owned agent component with move-to/track-actor semantics** | Same reason: the "issue a move and forget" surface is CkCrowd's goal/episode API, which already exists and is already provider-neutral. | Not planned. |
| **In-place mutable field with reader/writer locks** | Rejected by [NN-D7]: immutable publish is what makes off-thread boundary queries (F1.17) safe by construction and makes repair corruption-unrepresentable. A lock discipline would be a strictly worse trade for us. | Rejected, recorded here so it is not re-litigated. |
| **A coincident polygon mesh alongside the ground field** | [NN-D7] fence: this is candidate C, rejected without scoring. Two coupled structures must be built, repaired, and kept mutually consistent — every dynamic update transacts across both — for marginal query wins, and the plate decomposition already supplies the coarse search structure the mesh would add. F3.6 (outline smoothing) is the sanctioned answer to boundary quality. | Rejected. |
| **Per-agent-radius baked fields** | [NN-D7] fence: clearance is per cell and per portal; one bake serves all radii. | Rejected. |
