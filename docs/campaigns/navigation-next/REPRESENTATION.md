# navigation-next — Ground representation decision [NN-D7]

> **Status:** DECIDED (planning session 1, 2026-08-31). Executors do not revisit this.
> CTO may veto at package review; a veto reopens this file, nothing else does.

## The requirements the representation must satisfy

Derived from the capability freeze (FEATURE_MATRIX.md) and the coupling inventory
(`research/R3-coupling-inventory.md`). The representation must support, natively or cheaply:

1. **Projection** — point → nearest walkable surface, asymmetric search extents, batched.
2. **Constrained surface walk** — move a point along walkable ground without leaving it
   (today's `FindMoveAlongSurface` equivalent; the single Transform writer for grounded agents
   depends on it).
3. **Walkability raycast** — straight-line "can I walk from A to B" test with hit point.
4. **Boundary geometry** — nearest wall/edge segments around a point, **callable off the game
   thread** (crowd avoidance samples it from a parallel processor today).
5. **Path search + string-pulling** — partial paths, per-area costs, per-query filters.
6. **Overlapping floors** — multi-story buildings, bridges, stacked walkways.
7. **Slopes, ramps, stairs** — per-cell height and surface normal; slope/step/clearance
   filtering per agent profile.
8. **Dynamic areas** — runtime cost/exclusion painting with observable "your markup is live"
   ground truth, plus localized rebuild when geometry moves.
9. **Reachability** — near-O(1) unreachable rejection (component ids), flood-fill point
   generation (random/grid points for EQS-style consumers).
10. **Deterministic, testable builds** — a bake a hermetic C++ test can run against a
    hand-authored geometry list with an assertable probe/cost count.
11. **Multi-world safety** — per-world (or per-volume-entity) state; no process-wide truth.
12. **Streaming/persistence-ready** — stable integer identity (no pointer identity), chunkable,
    serializable as values.
13. **Future extension** — must not preclude arbitrary-orientation surface navigation
    (a later, product-sized campaign) or non-circular agent footprints.

## Candidates compared

### A — Tiled convex-polygon navmesh (heightfield → regions → contours → polygons)
The industry-standard pipeline, publicly documented end-to-end (the open-source
recastnavigation project, zlib license; Mononen's published talks; academic literature on
navigation meshes). Original implementation, tiled, with vertical layers.

- **For:** most compact runtime structure; smooth boundaries along diagonal walls; the funnel
  algorithm is the canonical fit; enormous public literature.
- **Against:**
  - It re-implements, feature for feature, the exact engine dependency we are retiring — the
    "why not just keep Recast" question has no crisp answer if our replacement is a
    from-scratch Recast.
  - Largest original-code surface in precisely the stages (region partitioning, contour
    simplification, polygonization) where independent implementations are hardest to keep
    structurally distinct from the references an author has seen.
  - Weakest differentiation: none of our proven in-house machinery (merged-cell decomposition,
    epochs, local repair, probe-deterministic budgeting) transfers; polygon meshes resist
    local repair (a moved obstacle re-triggers region+contour+polygonize for whole tiles).

### B — Layered ground field with merged-plate decomposition + portal graph  ← **CHOSEN**
Rasterize world geometry into per-column walkable spans; extract non-overlapping vertical
layers; store per finest cell: height, quantized normal, area/usage, and a **clearance value
from a distance transform**; then greedily merge near-coplanar, same-policy cells into
axis-aligned convex **plates** (hundreds, not tens of thousands — the same collapse our
volumetric field demonstrated, 91,752 → 359 on a 6400uu scene); plates connect through
**portals** (shared-edge intervals carrying the min clearance across the crossing). A* runs
over the plate-portal graph via CkAStar; string-pulling runs the funnel algorithm over portal
intervals (the funnel is defined on any convex-region/portal sequence, not just triangles).

- **For:**
  - **Our own lineage.** Merged-cell decomposition, immutable publish, epochs, chunk portals,
    local repair, value-only snapshots, and probe-deterministic budgeting are proven, tested,
    in-house inventions (see `research/R4-ck-native-assets.md`); this representation is their
    natural 2.5D projection. The provenance story is the strongest of any candidate: public
    algorithms (span rasterization, distance transform, flood fill, A*, funnel) composed on an
    architecture we already own.
  - **Local repair is natural.** A moved obstacle re-probes only intersecting columns, then
    re-merges only the touched plates' neighborhood — the plate decomposition is derived, never
    patched, so repair equals partial re-derivation (the corruption-unrepresentable property we
    already test-pin volumetrically).
  - **Per-cell clearance beats area erosion.** One bake serves every agent radius
    (`clearance >= R` per plate/portal), gives wall-distance costing for free, and closes the
    transition-clearance hole that pure cell-size filters have (portal carries min clearance
    across the shared boundary — recorded as our improvement in the 2026-08-28 proposal, §1.1).
  - **Off-thread queries are trivial** — `TSharedPtr<const FGroundField>` immutable publish
    gives lock-free reads; requirement 4 falls out of the design instead of needing a
    lock-discipline contract.
  - **Deterministic and hermetically testable** exactly like the volumetric bake (probe-count
    budgeting, hand-authored box-list fixtures, no ECS required for the math core).
- **Against (accepted, with mitigations):**
  - Staircase boundaries at cell resolution along diagonal walls. Mitigated by: funnel over
    portals + clearance-biased costs (agents don't hug boundaries), boundary-segment queries
    returning cell-resolution edges is exactly what the avoidance consumer already handles
    (it consumes Recast's `FindEdges` wall segments today), and a boundary-smoothing pass on
    plate outlines remains an additive future option.
  - Memory: a per-cell layer field is bigger than a polygon mesh. Mitigated by plate-level
    storage of everything except height/clearance (per-cell arrays only in the finest tier),
    chunked tiles, and the measured collapse factor of merging.
  - Ramps/stairs need near-coplanarity merge criteria (height-plane fit tolerance + normal
    cone) so plates stay planar enough for funnel correctness. This is a real design task and
    is specified in PHASE docs, not left to executors.

### C — Hybrid layered grid + coincident polygon mesh
**Rejected without scoring.** Maintaining two coupled structures that must be built, repaired,
and kept mutually consistent doubles the bake and repair surface for marginal query wins; every
dynamic update must transact across both. Candidate B's plate decomposition already provides the
coarse search structure a polygon mesh would add.

### D — Curve/waypoint network only (extend CkPathNetwork)
**Rejected.** Ground navigation owns projection, containment, raycast, boundary queries,
dynamic areas, and reachability — none of which a sparse network supplies. Already ruled out
in the continuation brief ("adding another pathfinder alone is insufficient").

## Decision

**[NN-D7] Candidate B: layered ground field + merged plates + portal graph, chunked in tiles,
with per-cell clearance from a distance transform, immutable-publish + epoch semantics, local
repair, and CkAStar-driven search with portal-funnel string-pulling.**

Tentative module name **CkGroundNav** (final name at CTO review). All algorithm provenance is
public literature: span-based heightfield rasterization, connected-component layer extraction,
chamfer/two-pass distance transforms, greedy rectangle decomposition, portal graphs, A*,
funnel/string-pulling, Dijkstra flood fill, DDA grid traversal. A provenance ledger row per
feature is mandatory (see PROMPT.md provenance policy).

## Fences

- Do NOT add a coincident polygon mesh "for better paths" — that re-opens candidate C.
- Do NOT bake per-agent-radius fields — clearance is per-cell/per-portal; one bake serves all
  radii. Distinct *slope/step/height* profiles (not radius) that genuinely change walkability
  may justify additional profile layers; that is a CTO decision, not an executor's.
- Do NOT store raw pointers or engine-object references inside the field; stable integer ids
  only (tile id, layer index, plate id, portal id).
- Do NOT patch a published field in place. Repair derives a new field (or new tile) and swaps.
- The plate merge criteria (plane-fit tolerance, normal cone, policy equality) are frozen in
  PHASE_1 (where the plate merge lands, F1.6); executors implement, they do not tune the
  definition of "mergeable" beyond the exposed tunables.
