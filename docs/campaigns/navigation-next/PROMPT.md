# Campaign: navigation-next — CkGroundNav, an ECS-native grounded-navigation provider

> **Stable mission brief. Do not record moving state here — that lives in [PROGRESS.md](PROGRESS.md).**
> Freshness: authored at campaign open, after the P0 research fan-out and the representation /
> migration-seam decisions. Death condition: superseded when the campaign closes (every line in
> [VALIDATION.md](VALIDATION.md) green) — then this folder is history and
> `Source/CkGroundNav/Claude.md` is the living doc.

---

## 0. Provenance policy (read before anything else; binds every session and every PR)

**All navigation algorithms in this campaign are independently designed from Ck requirements,
original tests, and appropriately licensed public references.**

- Permitted sources: open-source code under permissive licenses (with the license honoured and
  attribution recorded), academic literature, published talks and papers, and public technical
  documentation.
- **Forbidden without exception:** copying, translating, transliterating, structurally mirroring, or
  otherwise deriving any part of this work from a proprietary source. "Structural mirroring" includes
  reproducing a proprietary implementation's file decomposition, stage sequence, data-structure
  layout, or naming, even when every line is retyped. A proprietary system may be used only as
  *behavioural/capability evidence* — "a shipping product does X, therefore X is a requirement we
  must satisfy" — and never as an implementation reference.
- **Every feature records its algorithmic provenance** in `FEATURE_MATRIX.md`'s **Provenance**
  cell: the public algorithm name(s) the feature is built from, plus the reference (paper, book,
  talk, or permissively licensed project + license) they come from. A feature with an empty
  Provenance cell is not done and cannot pass its phase gate.
- **Every PR that adds or changes a navigation algorithm names its public references** in the PR
  body. A reviewer who cannot trace an algorithm to a named public reference rejects the PR.
- **No campaign document, source comment, commit message, or PR text may name or allude to a
  proprietary reference codebase.** This is an open-source repository. Capability requirements are
  phrased as Ck requirements, full stop. (Recorded as [NN-D4].)
- Attribution obligations from permissively licensed sources (headers, LICENSE files, module doc
  rows) are satisfied in the same change that introduces the derived code — never deferred.

If an executor believes a required capability cannot be built without consulting a forbidden source,
that is a **STOP condition**: record it in PROGRESS.md § Blockers and return to the orchestrator.

---

## 1. Mission

Build **CkGroundNav** (working name; final name at CTO review): an original, ECS-native,
grounded-navigation provider inside CkFoundation, chartered as the **eventual replacement** for
Unreal Navigation / Recast across the Ck ecosystem.

Recast is **not** a permanent sibling. It is retained through the staged migration in exactly four
roles — **adapter** (so existing consumers keep working), **A/B oracle** (so parity is measured, not
asserted), **fallback** (so a CkGroundNav hard failure degrades exactly like a Recast failure does
today), and **rollback path** (so any phase can be reverted with a single setting) — and is retired
under its own gate once parity is proven. (Recorded as [NN-D1].)

**Generation-1 scope is grounded navigation only:** floors, ramps, stairs, multi-level overlap, and
nav links. (Recorded as [NN-D2].)

**Done means:** every item in [VALIDATION.md](VALIDATION.md) is green with evidence recorded in
PROGRESS.md, promotion to default provider has its CTO sign-off, and Recast retirement has its
own separate CTO sign-off.

## 2. Non-goals (fenced out — do not drift into these)

- **Arbitrary-orientation surface navigation** (walls, ceilings, curved manifolds). A future,
  product-sized campaign. The chosen representation must not *preclude* it; Generation 1 does not
  *implement* it.
- **Volumetric / free-space 3D navigation.** That is CkVoxelNav's domain and stays there. Flying and
  swimming agents keep their existing provider branch.
- **Replacing CkPathNetwork.** Authored ribbon routing stays a distinct layer; it migrates off its
  *Recast safety oracle* onto the neutral facade, and it is not absorbed.
- **A coincident polygon mesh beside the ground field** "for nicer paths" — that re-opens the
  rejected hybrid candidate and is fenced by REPRESENTATION.md.
- **Per-agent-radius baked fields.** Clearance is per-cell and per-portal; one bake serves all radii.
- **Back-compat shims.** CkFoundation doctrine: the old API is deleted in the same change that
  replaces it. No parallel deprecated surface, no adapter left behind "just in case".
- **Any implementation during planning sessions.** The planning package is documentation with
  illustrative snippets only; nothing lands in `Source/`, nothing compiles. (Recorded as [NN-D3].)
- **Speculative extension points.** No hooks, no virtuals, no settings for capabilities no consumer
  in the R3 dependency table asks for.

## 3. Chosen approach

The two authoritative design documents. **Executors implement them; they do not revisit them.**
A fork found while implementing returns to the orchestrator — it is not resolved locally.

### 3.1 Representation — [REPRESENTATION.md](REPRESENTATION.md) is authoritative ([NN-D7])

**Layered ground field + merged-plate decomposition + portal graph**, chunked in tiles:

- Rasterize world geometry into per-column walkable spans; extract non-overlapping vertical
  **layers** so overlapping floors, bridges, and stacked walkways are first-class.
- Store per finest cell: height, quantized surface normal, area/usage policy, and a **clearance**
  value produced by a distance transform. Clearance-per-cell replaces per-radius area erosion, and
  a **portal** carries the minimum clearance across the boundary it spans — which closes the
  transition-clearance hole that cell-size filtering alone leaves open.
- Greedily merge near-coplanar, same-policy cells into axis-aligned convex **plates** — the same
  collapse the volumetric field already demonstrates in-house (measured 91,752 cells → 359 merged
  on a 6400uu scene).
- Search runs over the plate/portal graph on **CkAStar**; string-pulling runs the **funnel
  algorithm** over portal intervals (the funnel is defined on any convex-region/portal sequence,
  not only on triangles).
- Field values are published immutably (`TSharedPtr<const T>`), versioned by **epoch**, chunked by
  stable integer id, repaired by *partial re-derivation* rather than in-place patching, and
  snapshotted as values only.

Public-literature provenance for the whole pipeline: span-based heightfield rasterization,
connected-component layer extraction, chamfer / two-pass distance transforms, greedy rectangle
decomposition, portal graphs, A*, funnel string-pulling, Dijkstra flood fill, DDA grid traversal.

### 3.2 Migration seam — [MIGRATION_SEAM.md](MIGRATION_SEAM.md) is authoritative ([NN-D8])

- The **request/result/revision/cancellation contract is already provider-neutral** and stays
  byte-compatible. `MarkPathPending` / `InstallExternalPath` / `AbandonPath` / `FailPath` plus the
  revision ring is a proven external-provider seam — CkVoxelNav and CkPathNetwork already ship paths
  through it. CkGroundNav plugs in the same way.
- **Two public Unreal-typed leaks are neutralized first** (no behaviour change): the excluded-area
  overlay becomes area **gameplay tags**, and the per-query filter-class override becomes a tag
  resolved through a **provider-neutral filter definition** that each adapter compiles into its
  native form.
- **Twelve engine-supplied capabilities beyond FindPath** move behind one neutral facade
  (`UCk_Utils_NavSurface_UE`, working name): projection, constrained surface walk, walkability
  raycast, boundary segments (with an explicit off-game-thread contract), reachability, area markup,
  markup-live ground truth, surface revision + rebuild observability, surface bounds, provider
  health, build-in-progress, and the deterministic test rebuild hook.
- **Coexistence, then promotion, then retirement.** Provider choice is data-driven; a shadow/A-B
  mode dispatches the same request to both providers, installs the authoritative one, and records
  the divergence. Promotion and retirement are separate, measured gates with separate CTO sign-offs.

### 3.3 Phase map ([NN-D9])

Each phase gets its own `PHASE_N.md` authored at its boundary, per `ck-methodology`.

| Phase | Scope |
|---|---|
| P0 | Contract neutralization (the two leaks + filter-definition assets) + Recast adapter behind the facade |
| P1 | Bake core: rasterization, layers, clearance, plate merge, portals — hermetic, ECS-free math |
| P2 | Query surface: projection, surface walk, raycast, boundary segments, reachability |
| P3 | Search + install + shadow mode: CkAStar plate/portal graph, funnel string-pull, A/B harness |
| P4 | Dynamics: area markup, markup-live probe, dirty bounds, local repair, epoch observability |
| P5 | Links + CkPathNetwork migration off the Recast oracle + editor authoring snap |
| P6 | Debugger module + gym |
| P7 | Persistence / cook / streaming of baked tiles |
| P8 | Parity measurement → promotion (CTO sign-off) → retirement (separate CTO sign-off) |

### 3.4 Rejected approaches (one-line kill reasons)

| Rejected | Kill reason |
|---|---|
| **Original tiled convex-polygon navmesh** (heightfield → regions → contours → polygons) | Re-implements feature-for-feature the exact dependency being retired, has the largest original-code surface precisely where independent implementations are hardest to keep distinct, and inherits none of the in-house repair/epoch/merge machinery. |
| **Hybrid layered grid + coincident polygon mesh** | Two coupled structures to build/repair/keep consistent; marginal query wins. Every dynamic update must transact across both, and the plate decomposition already supplies the coarse search structure the mesh would add. |
| **Curve/waypoint network only** (extend CkPathNetwork) | A sparse network supplies none of projection, containment, walkability raycast, boundary geometry, dynamic areas, or reachability; adding a pathfinder alone does not replace the provider. |
| **Keep Recast permanently as the walker stack** (the prior ruling) | Superseded by [NN-D1]; leaves the ecosystem's most load-bearing spatial service outside ECS ownership, off-thread discipline, epoch semantics, and hermetic testability. |
| **Per-agent-radius baked fields** | Multiplies bake cost and memory by the agent-profile count and still misses transition clearance; per-cell clearance + per-portal minimum solves both with one bake. |
| **Patch the published field in place on repair** | Makes corruption representable. Repair derives a new field/tile and swaps — the property the volumetric stack already test-pins. |

## 4. File inventory

### 4.1 Campaign documents (this folder)

| Doc | Role |
|---|---|
| `PROMPT.md` (this file) | Stable mission, provenance policy, approach, executor rules. No state. |
| `PROGRESS.md` | The living state of record: status board, numbered decisions, Done/In-flight/Blockers, session log. **The only place moving state lives.** |
| `REPRESENTATION.md` | Authoritative representation decision [NN-D7]: requirements, candidates, rejections, fences. |
| `MIGRATION_SEAM.md` | Authoritative seam design [NN-D8]: leak neutralization, the facade, coexistence, promotion/retirement gates, rulings [NN-D8a..h]. |
| `FEATURE_MATRIX.md` | Per-capability requirements freeze with the mandatory **Provenance** column. |
| `VALIDATION.md` | The acceptance protocol — the definition of done. |
| `PHASE_N.md` | Per-phase execution briefs, authored at each phase boundary. |
| `research/R3-coupling-inventory.md` | The Recast-coupling dependency table with file:line buckets. Executors consult it before touching any consumer. |
| `research/R4-ck-native-assets.md` | The in-house asset inventory: what already exists and must be reused rather than reinvented. |

### 4.2 Modules and files executors will touch

From R3's dependency table. Line references there are a snapshot — **re-verify before editing**.

**CkNavigation — the seam itself**

| Path | Why it matters |
|---|---|
| `Nav/CkNav_Fragment_Data.h` | Holds the entire public request/result contract, including the only two Unreal-typed leaks (`_ExcludedAreaClasses`, `_QueryFilterClassOverride`) that P0 neutralizes. |
| `Nav/CkNav_Algorithm.h/.cpp` | The provider boundary: the Recast-typed statics on one side, the neutral `InstallExternalPath` / `MarkPathPending` / `AbandonPath` / `FailPath` seam on the other. |
| `Nav/CkNav_Processor.cpp` | Owns the revision ring, per-frame query budget, deferred-request queue (process-wide today — becomes per-world), authority, drain, and signals. |
| `Settings/CkNav_ProjectSettings.h` | The tag→filter table that becomes tag→neutral-filter-definition, plus budget and projection extents. |
| `NavAreaMarkup/CkNavAreaMarkup_Utils.h/.cpp` | The only `INavRelevantInterface` site; the dynamic cost/exclusion painter CkCrowd and CkQueue depend on. Must be reimplemented natively. |
| `Revision/CkNavigationRevision_Subsystem.*` | World-scoped "generation N finished" observability; consolidates with CkQueue's duplicate under the facade ([NN-D8a]). |
| `Utils/CkNav_Utils.cpp` | Public projection UFUNCTION plus the `_ForTesting` seams the whole test corpus leans on. |
| `CkNavigation.Build.cs` | Carries the `NavigationSystem` / `AIModule` dependencies whose removal is a retirement-gate criterion. |

**CkCrowd — the largest consumer**

| Path | Why it matters |
|---|---|
| `Agent/CkCrowdAgent_HandleRequests_Processor.cpp` | The provider fork (`RequestPathForActiveGoal`) and the episode acquire/release lifecycle CkGroundNav joins as a fourth branch — preserved verbatim. |
| `Agent/CkCrowdAgent_ConstrainToNavmesh_Processor.cpp` | **The single Transform writer for grounded agents.** Constrained surface walk plus off-mesh recovery projection; the highest-risk migration in the campaign. |
| `Agent/CkCrowdAgent_AvoidanceSample_Processor.*` | Boundary wall segments queried **off the game thread** from a parallel processor — the reason the facade needs an explicit immutable-snapshot thread contract. |
| `Agent/CkCrowdAgent_PathRefresh_Processor.cpp` | Rebuild observability, the "did my markup land" ground-truth probe, and synchronous replans on the escape/corridor-splice paths. |
| `Agent/CkCrowdAgent_Steering_Processor.cpp`, `_OnPathResolved_`, `_OnRouteResolved_`, `_BlockDetect_` | Walkability-raycast and surface-walk consumers; mechanical facade migrations with behavioural parity requirements. |
| `Agent/CkCrowdAgent_NavArea.h`, `_NavQueryFilter.h`, `AvoidanceVolume/*_NavArea.*` | Framework area/filter classes that become area tags + filter-definition assets. |
| `Agent/CkCrowdAgent_DiagNavClip_Processor.cpp`, `_DrawNavProjection_Processor.cpp`, `Settings/CkCrowd_DebugSettings.cpp` | Bucketed **[RETIRE]** — deleted, not migrated. |
| `CkCrowd.Build.cs` | Its nav-dependency comment is stale (claims one consumer; six processors use it) — corrected during the mechanical migration ([NN-D8h]). |

**CkPathNetwork (+ Editor)**

| Path | Why it matters |
|---|---|
| `Network/CkPathNetwork_Processor.cpp` | Uses Recast twice: as off-network connector-path builder, and as the **safety oracle** that proves every compiled ribbon segment walkable before install. Migrating the oracle is a retirement-gate criterion. |
| `CkPathNetworkEditor/CkPathNetwork_EditorUtils.cpp` | Authoring-time node snap onto the surface — needs an editor-time equivalent on the new provider. |

**Other consumers**

| Path | Why it matters |
|---|---|
| `CkEqs/Query/CkEqs_Algorithm.cpp` | Deliberately inlined projection post-pass; migrates to the facade and is re-measured ([NN-D8g]). |
| `CkQueue/Queue/CkQueue_Formation_Processor.cpp` | Slot placement via projection + raycast. |
| `CkQueue/Navigation/CkQueue_NavigationRevisionSubsystem.cpp` | The duplicate revision observer that the facade consolidates. |
| `CkVoxelNav/Path/CkVoxelNavPath_Utils.*` | An existing non-Recast provider already living behind the seam — the reference implementation for how CkGroundNav plugs in. |
| `CkGameplayDebugger/CkNavmeshDebugDraw_Subsystem.cpp` | Draws Recast tile geometry; **retired, not rewritten**, at Recast retirement ([NN-D8f]). |
| `CkGameplayDebugger/CkCrowdDebugger_DataCollector.cpp`, `Types.h`, `CkPerfLab_WorldSurvey_Builder.cpp` | Need neutral "surface bounds + provider health" and neutral seeded spawn positions. |
| `CkTests/Script/**` (`UNavArea_Null` fixture sites, `CkTestsAssets.as`) | 10+ AutoTests and gyms punch navigation holes with a Recast area class. **The fixture vocabulary "paint an impassable box at runtime" must survive**, or the crowd suite loses its obstacles. |

### 4.3 Ck assets to build on (reuse before inventing — R4)

| Asset | Use it for |
|---|---|
| **CkAStar** — `CkAStar_GraphConcept.h` (`Neighbors`/`Cost`/`Heuristic`/`IsGoal`, nothing geometric) | The plate/portal search. The concept is already proven over both a merged-box decomposition and an edge graph. |
| **CkAStar** — `TSearchState::ContinueSearch` (time-sliced, iteration **and** microsecond budget) | Budgeted searches that survive a frame boundary. |
| **CkAStar** — warm-start constructor `(Graph, Start, Goal, ExistingPath, WarmStartFromIndex, Capacity)` | Plan repair after a localized field change, instead of a full replan. |
| **CkAStar** — free `ValidateExistingPath(Graph, Path) -> int32` | Cheap "is my installed path still connected", returning the first disconnected step — the repair trigger. |
| **CkAStar** — `PathCache` (generation-counter invalidation), `PrecomputedTable` | Optional caching layers; treat their capability claims as doc-sourced until code-verified. |
| **Immutable publish** pattern (`TSharedPtr<const T>` field in a fragment slot, atomic swap) | Lock-free off-thread reads; the reason boundary queries need no lock discipline. |
| **Epoch** pattern (bump per completed rebuild; staleness **derived** at the read boundary, never stored; chunked fields use the epoch **sum** as a monotone fingerprint) | Rebuild observability, stale-plan detection, debugger cache identity. |
| **Chunk** pattern (stable integer ids = volume id + lattice index; portals + adjacency table; partitioning decided at composition, not in a processor) | Tiling the ground field with streaming-ready identity. |
| **Local repair** pattern (dirty-bounds fragment + NeedsRepair/RepairInProgress tags; re-probe only intersecting cells; **never mutate the published structure**) | Moved-obstacle repair. Test-pinned in-house: repaired == full rebake, moved obstacle flips occupancy only where it moved, sliced repair == one-shot repair. |
| **Value-only debug snapshot** pattern (boxes/counts/ids/epochs/status only — never handles, UObjects, or shared structures; **failure is a status, never an empty scene**; whole-snapshot atomic replace) | The debugger data contract for P6. |
| **Probe-count budgeting** (probe count is the primary, deterministic, test-assertable budget; wall-clock is only a guard) | Deterministic, hermetically assertable bake tests. |
| **Geometry backend seam** (a small JPH-free interface with a physics-backed impl and a stub impl; static-body domain only) | Bake input. Go through this seam, not through live entity-overlap probes. |
| **CkShapes** | Authored extents, radii, and clearance values — typed shape data, never bare floats. |
| **CkPmg** — `Create_DebugLineSet` + `Append_Debug*_World` (retained wireframe), `BasicShapes`, `DrawFilled*` | Debug meshes for plates, portals, paths, corridors, and boundary segments. Already a CkCrowdDebugger dependency. |
| **CkNavigation seam files** — `FCk_Nav_PathResult`, `MarkPathPending`, `InstallExternalPath`, `AbandonPath`, `FailPath`, revision ring | The install path. Do not invent a second one. |
| **CkPathNetwork** — world-free `RoutePlan` seam shared by runtime and editor preview; enumerated failure reasons; derived-only `FBuiltNetwork` | The precedent for "pure math, runtime-callable, no ECS/world dependency" build layers. |

## 5. Glossary

Every non-obvious term the doc set uses. Executors and reviewers share this vocabulary.

| Term | Meaning in this campaign |
|---|---|
| **Ground field** | The baked, per-tile, per-layer array of finest-resolution cells carrying height, quantized normal, area/usage policy, and clearance. The raw representation everything else is derived from. |
| **Cell** | One finest-resolution column sample in the ground field. Addressed by integer index, never by pointer or world position. |
| **Layer** | One non-overlapping vertical walkable surface within a tile. Multi-story buildings, bridges, and stacked walkways produce multiple layers over the same 2D footprint. |
| **Plate** | A merged, axis-aligned, convex region of near-coplanar same-policy cells. The node of the search graph. Plates are **derived** from the ground field — never authored, never patched. |
| **Portal** | A shared-edge interval between two adjacent plates, carrying the **minimum clearance** across the crossing. The edge of the search graph, and the interval the funnel string-pull walks. |
| **Clearance field** | The per-cell distance-to-nearest-blocked value produced by a distance transform. Serves every agent radius from one bake (`clearance >= R`) and gives wall-distance costing for free. |
| **Tile / chunk** | The spatial partition unit of the field: a bounded region with a stable integer id, its own epoch, its own portals to neighbours, independently bakeable, repairable, serializable, and streamable. ("Tile" and "chunk" are used interchangeably; prefer **tile** in ground-field prose.) |
| **Epoch** | A monotone counter bumped once per completed (re)build of a tile or field. Staleness is **derived** by comparing a held epoch against the current one at the read boundary — never stored as a flag. A chunked field's fingerprint is the sum of its chunk epochs. |
| **Provider episode** | One agent's complete path-request lifecycle: advance revision → abandon the previous provider → mark the shared result slot pending → dispatch to exactly one provider → install only a fresh result → release on terminal outcome. Owned by CkCrowd and preserved verbatim. |
| **Revision** | The caller-owned opaque integer stamped on a request and echoed on its result, compared on a ring over `[1, MAX_int32]` (shorter forward distance = newer). The stale-result protection for the whole seam. |
| **Shadow mode** | A/B operation in which the same request is dispatched to both the Recast adapter and CkGroundNav; the authoritative provider's result installs, and the shadow result is compared and counted rather than used. The mechanism that produces promotion evidence. |
| **Markup** | Runtime painting of cost or exclusion over a region — "make this box impassable / expensive until further notice". Replaces the engine's nav-relevant registration for consumers, and must serve the existing test-fixture vocabulary. |
| **Markup-live probe** | The ground-truth query "has my markup actually landed in the field yet". Consumers gate on this rather than assuming their paint took effect. |
| **Facade** | `UCk_Utils_NavSurface_UE` (working name) — the provider-neutral utils surface through which every consumer reaches ground-navigation capability. ECS-idiomatic (utils + fragments), **not** a UObject interface. |
| **Adapter** | A per-provider translation layer behind the facade. The Recast adapter compiles neutral filter definitions into engine filters and neutral area tags into engine area classes; the CkGroundNav adapter compiles them into a plate-policy mask and cost table. |
| **Filter definition** | A provider-neutral data asset naming allowed/excluded area tags and per-area cost multipliers. Replaces the engine filter-class references in the public contract. |
| **Area tag** | A `FGameplayTag` identifying a surface policy class (restricted, crowd-agent, avoidance-volume, …). Replaces the engine area-class references in the public contract. |
| **Promotion** | Making CkGroundNav the default provider while Recast remains present and selectable. Its own measured gate, its own CTO sign-off. |
| **Retirement** | Deleting the Recast adapter, its dependencies, and its debug module. A **separate** later gate with a **separate** CTO sign-off. |
| **Provenance** | The named public algorithm(s) and named public reference(s) a feature is built from, recorded per feature in `FEATURE_MATRIX.md`. Mandatory. |
| **`[EDITOR-VERIFY]`** | A check no agent can perform — requires a human running the editor, PIE, or a packaged build. Always accompanied by exact steps and exact expected observations. |
| **Delta-zero** | Test totals (pass/fail/skip counts and failing-test names) identical to the phase-entry baseline, plus the phase's new tests green. The only admissible form of a "no regressions" claim. |
| **Funnel / string-pulling** | The pass that turns a plate corridor into a taut polyline by walking the portal intervals and pivoting on the interval endpoints. Shortens the raw plate-sequence path without leaving the corridor. |
| **Containment escape** | An end position that lands outside the walkable set. The safety invariant every projection, move-along-surface, and post-processing pass must preserve; a single escape is a gate failure, not a tolerance. |
| **Safety oracle** | CkPathNetwork's "is this authored segment directly walkable" verdict, today answered by Recast. P5 moves it onto the neutral facade; verdict agreement over shipped networks is the migration evidence. |
| **Plane-fit tolerance** | The maximum residual, in unreal units, between a plate's fitted height plane and the cells it covers. Too tight shatters a staircase into one plate per tread; too loose over-merges and breaks funnel correctness. |
| **Normal cone** | The maximum angular spread, in degrees, of per-cell normals permitted inside one plate. The second half of the near-coplanarity merge criterion. |
| **Collapse factor** | The ratio of finest-resolution cells to merged plates on a given fixture. Tracked as a regression number, not a pass/fail: a poor ratio is a search-cost finding. |
| **DDA** | Digital Differential Analyzer — the incremental integer line-walk used to traverse a grid along a ray. The engine behind surface raycast and arc traversal. |
| **Flood fill** | The single-source expansion over plates/portals shared by reachability, path-distance queries, and point generation. One implementation, several consumers — never a second copy. |
| **PerfLab** | The Ck performance-harness gym used to seed and re-measure per-frame budgets. Its numbers feed the promotion budgets in PHASE_8. |
| **Corridor / ribbon / off-path leg** | *Corridor* = the ordered plate sequence a search returns. *Ribbon* = the swept walkable band the corridor covers, inside which the funnel is free to move. *Off-path leg* = a CkPathNetwork connector from an arbitrary position onto the authored network. |
| **Station / dwell / settle** | CkQueue vocabulary: a *station* is a queue slot position; *dwell* is the time an agent holds it; *settle* is the convergence of an agent onto it within tolerance. Nav supplies projection and raycast for all three; it owns none of them. |

## 6. Skills to load, and when

Executor sessions load these **before** the work they govern, not after.

| About to… | Load |
|---|---|
| write any fragment, processor, request, signal, tag, or typesafe handle — or read any `CK_` macro | `ck-macros-and-codegen` |
| touch the request/signal lifecycle, entity lifetimes, replication stance, or any cross-system invariant | `ckecs-architecture-contract` |
| write or run any test (C++ automation, PIE AS autotest, net autotest, gym, Gauntlet) | `ck-tests-authoring-and-running` |
| add or change any debugger view, overlay, inspector, or data collector | `ck-gameplaydebugger-extension` |
| write or modify any Slate UI — debugger widgets, list/tree rows, styling, viewport interaction | `ck-slate-tools` |
| claim any phase — or any change within it — is done | `ck-change-control` |
| build or run tests, ever | `build-test` |

Supporting, as the situation calls for them: `ck-angelscript-interop` (exposing anything to
AngelScript, or diagnosing a silent binding break), `ck-debugging-playbook` (build/UHT/linker/
packaged-only failures), `ck-performance-and-analysis` (any measurement that will appear in a
gate), `ckecs-domain-reference` (EnTT/registry internals), `ck-failure-archaeology` (before
resurrecting anything that looks previously attempted), `ck-methodology` (phase-doc discipline).

**Exemplars — mimicry is mandatory, invention is not.** Feature quartet shape: `CkTimer`. Provider/
volume/follower shape and the AStar graph adapter: `CkPathNetwork` and `CkVoxelNav`. Read the
target module's own `Claude.md` before coding in it.

### 6.1 Reference implementation to consult on hard problems ([S13-D3], maintainer 2026-09-07)

`D:\Repos\JoltNav` is a Mercuna-derived navigation + movement stack (`Jolt2DNavigation`: Recast-derived ground polymesh, `MerDriver`, `MerCommandPathTo`, ORCA and contextual-steering avoidance; `Jolt3DNavigation`: octree + `Pilot`; `JoltCore`). **It may always be consulted, read-only, on any hard problem where the solution is not clear** - before ruling a design fork, read how it resolves the same situation and record the comparison in the decision entry. Precedent: F9 (a crowd-cause `GoalBlocked` hold that never exits) - JoltNav has no occupancy inference at all; it cuts stationary agents out of the nav graph by footprint (`JoltObstacleComponent::StationaryThresholdTime`, `MerNavGraph2DData.cpp:182`), answers a goal inside one with a partial path or a failure (`MerPath2D.cpp:287`), and completes arrival geometrically (`MerCommandPathTo.cpp:118-138`). Nothing is copied from it; it is a second opinion with working code behind it.

## 7. Session-start ritual (verbatim — every executor session, every resumption)

1. **Read `PROGRESS.md` top to bottom, first, before anything else.** It outranks your memory, any
   summary you were handed, and any recollection of "where we were".
2. **Distrust it.** It is written by fallible sessions and decays between them.
3. **Spot-check two "Done" claims** against the actual code or the actual test names — pick the two
   whose failure would hurt most. If a claim does not reproduce, **stop**, correct PROGRESS.md, and
   report the drift before doing any new work.
4. **Read the current `PHASE_N.md` and verify its entry criteria** hold right now: the stated repo
   and submodule states, the stated prerequisites, the stated baseline. Do not begin a phase whose
   entry criteria you could not confirm.
5. **Capture a baseline before touching anything.** Run the full suite through the toolbox and
   record totals, failing-test names, and the artifact it ran against, in PROGRESS.md. A "no
   regressions" claim made without a baseline captured this session is void. A green run that
   predates your edits is not evidence — it is stale-green.
6. **Never edit source, `Script/`, or config while a build or test run is in flight.** Mid-run edits
   poison the run and mis-attribute the failure to whatever test happened to be executing. Freeze
   edits for the run's duration; batch your changes, then gate once.
7. Reconcile before you extend: if the code has moved since PROGRESS.md's last entry, fix the doc
   first, then work.

## 8. Executor rules (sub-agents read this section verbatim)

- **Executors never make design decisions.** The design lives in `REPRESENTATION.md`,
  `MIGRATION_SEAM.md`, `FEATURE_MATRIX.md`, and the phase docs. Implement what is written.
- **Anything else → STOP and record it in PROGRESS.md § Blockers**, then return to the orchestrator.
  "Anything else" means: a design fork; an ambiguity the docs do not resolve; an observation the
  research docs did not enumerate; a contradiction between two campaign documents; a contradiction
  between a campaign document and the code; two failed attempts at the same step; any capability
  that appears to require a forbidden source. Report verbatim evidence, not a paraphrase.
- **No implementation in planning sessions.** Planning produces documents with illustrative
  snippets. Nothing lands in `Source/`; nothing compiles.
- **Toolbox-only builds and tests.** Never invoke `Build.bat`, UnrealBuildTool, or
  `UnrealEditor-Cmd.exe` directly — the toolbox owns engine resolution, the machine-wide build lock,
  watchdogs, and structured results. Use `--parallel 1` for any run whose result you will cite;
  `--no-nullrhi` for full-suite gates; `--discover-fresh` when new AS autotests must appear; and
  remember a new C++ automation test needs a touch + relink, and a new **net** AS autotest needs a
  full C++ rebuild before it exists at all.
- **Commits only when the maintainer asks.** Never push, never bump a submodule pointer, never open
  a PR, never merge on your own initiative. Stage only paths you authored — never a blanket
  directory add; dirty files you did not write belong to other sessions and are left untouched and
  reported.
- **Report what you confirmed separately from what you inferred.** A gate you did not re-run on the
  final artifact is not a gate you passed.
- **Fences carried from the design docs:** no coincident polygon mesh; no per-radius baked fields;
  no raw pointers or engine-object references inside the field (stable integer ids only); never
  patch a published field in place; do not re-tune the plate-merge criteria beyond the exposed
  tunables; deferred `Request_*` APIs end with the completion delegate as the **last** parameter;
  no back-compat shims.
